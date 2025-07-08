#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int watering_hour;
    int watering_minute;
    int duration_minutes;
} schedule_config_t;

bool load_schedule_config(schedule_config_t *cfg);
void save_schedule_config(const schedule_config_t *cfg);
void get_schedule_string(const schedule_config_t *cfg, char *out, size_t len);
