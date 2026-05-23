/**
 * @file http_server.c
 * @brief Simple HTTP server for board API (port 8080)
 */

#include "tal_api.h"
#include "tkl_output.h"
#include "cJSON.h"
#include "tal_socket.h"

#include "http_server.h"

// ==========================================
// CONFIGURATION
// ==========================================

#define HTTP_SERVER_PORT 8080
#define HTTP_SERVER_BACKLOG 5
#define HTTP_BUFFER_SIZE 4096

// ==========================================
// HELPER FUNCTIONS
// ==========================================

/**
 * @brief Create JSON response
 */
static char *create_json_response(const char *endpoint, const char *status)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "endpoint", endpoint);
    cJSON_AddStringToObject(root, "status", status);
    cJSON_AddNumberToObject(root, "timestamp", tal_time_get_posix());

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return json_str;
}

/**
 * @brief Send HTTP response
 */
static void send_http_response(int socket, int status_code, const char *content_type,
                               const char *body)
{
    char response[8192] = {0};

    // Build HTTP response header
    snprintf(response, sizeof(response),
             "HTTP/1.1 %d OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             status_code, content_type, strlen(body), body);

    tal_sock_send(socket, (uint8_t *)response, strlen(response));
}

/**
 * @brief Handle /api/status endpoint
 */
static void handle_status(int socket)
{
    PR_DEBUG("GET /api/status");

    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "device_id", "t5_clock_001");
    cJSON_AddStringToObject(root, "device_name", "HackStorm Clock");
    cJSON_AddStringToObject(root, "version", "1.0.0");
    cJSON_AddNumberToObject(root, "uptime_seconds", tal_system_get_tick() / 1000);
    cJSON_AddNumberToObject(root, "timestamp", tal_time_get_posix());
    cJSON_AddBoolToObject(root, "display_enabled", true);

    // Display info
    cJSON *display = cJSON_CreateObject();
    cJSON_AddNumberToObject(display, "width", 320);
    cJSON_AddNumberToObject(display, "height", 480);
    cJSON_AddNumberToObject(display, "brightness", 100);
    cJSON_AddItemToObject(root, "display", display);

    // Storage info
    cJSON *storage = cJSON_CreateObject();
    cJSON_AddNumberToObject(storage, "total_kb", 1024);
    cJSON_AddNumberToObject(storage, "available_kb", 512);
    cJSON_AddItemToObject(root, "storage", storage);

    char *json_str = cJSON_PrintUnformatted(root);
    send_http_response(socket, 200, "application/json", json_str);

    free(json_str);
    cJSON_Delete(root);
}

/**
 * @brief Handle /api/time endpoint
 */
static void handle_time(int socket)
{
    PR_DEBUG("GET /api/time");

    TIME_T current_time = tal_time_get_posix();
    POSIX_TM_S tm = {0};
    tal_time_gmtime_r(&current_time, &tm);

    cJSON *root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, "timestamp", current_time);
    cJSON_AddStringToObject(root, "iso_8601",
                            "2024-09-21T14:30:45Z"); // TODO: format properly

    // Time components
    cJSON *time_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(time_obj, "hour", tm.tm_hour);
    cJSON_AddNumberToObject(time_obj, "minute", tm.tm_min);
    cJSON_AddNumberToObject(time_obj, "second", tm.tm_sec);
    cJSON_AddNumberToObject(time_obj, "day", tm.tm_mday);
    cJSON_AddNumberToObject(time_obj, "month", tm.tm_mon + 1);
    cJSON_AddNumberToObject(time_obj, "year", tm.tm_year + 1900);
    cJSON_AddItemToObject(root, "time", time_obj);

    cJSON_AddBoolToObject(root, "ntp_synced", true);
    cJSON_AddStringToObject(root, "timezone", "UTC");

    char *json_str = cJSON_PrintUnformatted(root);
    send_http_response(socket, 200, "application/json", json_str);

    free(json_str);
    cJSON_Delete(root);
}

/**
 * @brief Handle unknown endpoint
 */
static void handle_not_found(int socket, const char *path)
{
    PR_DEBUG("404 Not Found: %s", path);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "error", "Not Found");
    cJSON_AddStringToObject(root, "path", path);
    cJSON_AddNumberToObject(root, "status_code", 404);

    char *json_str = cJSON_PrintUnformatted(root);
    send_http_response(socket, 404, "application/json", json_str);

    free(json_str);
    cJSON_Delete(root);
}

/**
 * @brief Parse HTTP request and route to handler
 */
static void handle_http_request(int socket, const char *request_data)
{
    PR_DEBUG("HTTP Request:\n%s", request_data);

    // Simple request parsing (not fully RFC-compliant, but good enough for MVP)
    char method[16] = {0};
    char path[256] = {0};

    sscanf(request_data, "%15s %255s", method, path);

    PR_NOTICE("HTTP %s %s", method, path);

    // Route to appropriate handler
    if (strcmp(method, "GET") == 0) {
        if (strcmp(path, "/api/status") == 0) {
            handle_status(socket);
        } else if (strcmp(path, "/api/time") == 0) {
            handle_time(socket);
        } else {
            handle_not_found(socket, path);
        }
    } else {
        handle_not_found(socket, path);
    }
}

// ==========================================
// HTTP SERVER THREAD
// ==========================================

/**
 * @brief HTTP server main loop (runs in separate thread)
 */
void http_server_thread(void *arg)
{
    OPERATE_RET rt = OPRT_OK;
    int server_socket = -1;
    int client_socket = -1;
    uint8_t recv_buffer[HTTP_BUFFER_SIZE];

    PR_NOTICE("HTTP Server Thread Starting on port %d...", HTTP_SERVER_PORT);

    // Create socket
    server_socket = tal_sock_create(SOCK_STREAM);
    if (server_socket < 0) {
        PR_ERR("Failed to create socket");
        return;
    }

    // Allow socket reuse (prevent "Address already in use" error)
    int reuse = 1;
    tal_sock_setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // Bind to port
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(HTTP_SERVER_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    rt = tal_sock_bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (rt != 0) {
        PR_ERR("Failed to bind socket to port %d: %d", HTTP_SERVER_PORT, rt);
        tal_sock_close(server_socket);
        return;
    }

    PR_NOTICE("Socket bound to port %d", HTTP_SERVER_PORT);

    // Listen for connections
    rt = tal_sock_listen(server_socket, HTTP_SERVER_BACKLOG);
    if (rt != 0) {
        PR_ERR("Failed to listen on socket: %d", rt);
        tal_sock_close(server_socket);
        return;
    }

    PR_NOTICE("HTTP Server listening on port %d", HTTP_SERVER_PORT);

    // Accept and handle connections
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        // Accept incoming connection
        client_socket = tal_sock_accept(server_socket, (struct sockaddr *)&client_addr,
                                        &client_addr_len);
        if (client_socket < 0) {
            PR_WARN("Failed to accept connection: %d", client_socket);
            tal_system_sleep(1000);
            continue;
        }

        PR_DEBUG("Client connected: %d", client_socket);

        // Receive request
        int bytes_recv = tal_sock_recv(client_socket, recv_buffer, HTTP_BUFFER_SIZE - 1, 5000);
        if (bytes_recv > 0) {
            recv_buffer[bytes_recv] = '\0';

            // Handle request
            handle_http_request(client_socket, (char *)recv_buffer);
        } else {
            PR_DEBUG("No data received from client");
        }

        // Close client socket
        tal_sock_close(client_socket);
    }

    // Cleanup (never reached in normal operation)
    tal_sock_close(server_socket);
}
