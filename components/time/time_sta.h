#ifndef TIME_STA_H
#define TIME_STA_H

void time_sync_init(void);
void update_time_str(void);
bool is_time_synced(void);
char* get_time_str(void);

#endif