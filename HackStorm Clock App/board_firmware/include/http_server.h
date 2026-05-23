/**
 * @file http_server.h
 * @brief HTTP server header
 */

#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

/**
 * @brief HTTP server thread (runs in separate thread)
 *
 * Listens on port 8080 and handles API requests
 */
void http_server_thread(void *arg);

#endif /* __HTTP_SERVER_H__ */
