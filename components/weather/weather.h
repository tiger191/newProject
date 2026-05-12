#ifndef WEATHER_H
#define WEATHER_H


#include <stdbool.h>

void weather_task(void *p);
bool get_weather_ok(void);
void set_weather_state(bool state);
char* get_weather_str(void);
char* get_temp_str(void);

#endif