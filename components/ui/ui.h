#ifndef UI_H
#define UI_H

#include "lvgl.h"

void lvgl_init(void);
void create_ui(void);

lv_obj_t *get_label_time();

#endif