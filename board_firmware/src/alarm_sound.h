#ifndef __ALARM_SOUND_H__
#define __ALARM_SOUND_H__

void alarm_sound_init(void);   // call once at boot, BEFORE display_init()
void alarm_sound_start(void);
void alarm_sound_stop(void);

#endif
