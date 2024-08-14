#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    int show_config_message;
    int show_family_name;
    int show_bios_if_ver;
    int show_ryzenadj_ver;
    int show_table_ver;
    int show_stapm_limit;
    int show_stapm_limit_value;
    int show_fast_limit;
    int show_fast_limit_value;
    int show_slow_limit;
    int show_slow_limit_value;
    int show_stapm_time;
    int show_slow_time;
    int show_apu_slow_limit;
    int show_apu_slow_limit_value;
    int show_vrm_current;
    int show_vrm_current_value;
    int show_vrmsoc_current;
    int show_vrmsoc_current_value;
    int show_vrmsocmax_current;
    int show_vrmsocmax_current_value;
    int show_tctl_temp;
    int show_tctl_temp_value;
    int show_apu_skin_temp;
    int show_apu_skin_temp_value;
    int show_dgpu_skin_temp;
    int show_dgpu_skin_temp_value;
    int show_power_saving;
    int show_max_performance;
} config_t;

config_t parse_config(const char *config_file);

#endif