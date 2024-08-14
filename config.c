#include "config.h"
#include <unistd.h>
#include <stdio.h>
config_t parse_config(const char *config_file) {
    config_t config;
    memset(&config, 0, sizeof(config_t));
    if (access(config_file, F_OK) == -1) {
        FILE *fp = fopen(config_file, "w");
        if (fp) {
            fprintf(fp, 
                "#You can toggle the display of individual lines of \"'ryzenadj --info\" by setting either 0(invisible) or 1(visible) to the options presented below\n"
                "show_config_message=1\n"
                "show_family_name=1\n"
                "show_bios_if_ver=1\n"
                "show_ryzenadj_ver=1\n"
                "show_table_ver=1\n"
                "show_stapm_limit=1\n"
                "show_stapm_limit_value=1\n"
                "show_fast_limit=1\n"
                "show_fast_limit_value=1\n"
                "show_slow_limit=1\n"
                "show_slow_limit_value=1\n"
                "show_stapm_time=1\n"
                "show_slow_time=1\n"
                "show_apu_slow_limit=1\n"
                "show_apu_slow_limit_value=1\n"
                "show_vrm_current=1\n"
                "show_vrm_current_value=1\n"
                "show_vrmsoc_current=1\n"
                "show_vrmsoc_current_value=1\n"
                "show_vrmsocmax_current=1\n"
                "show_vrmsocmax_current_value=1\n"
                "show_tctl_temp=1\n"
                "show_tctl_temp_value=1\n"
                "show_apu_skin_temp=1\n"
                "show_apu_skin_temp_value=1\n"
                "show_dgpu_skin_temp=1\n"
                "show_dgpu_skin_temp_value=1\n"
                "show_power_saving=1\n"
                "show_max_performance=1\n");
            fclose(fp);
        } else {
            printf("Error creating configuration file: %s\n", config_file);
            return config;
        }
    }
    
    FILE *fp = fopen(config_file, "r");
    if (!fp) {
        printf("Error opening configuration file: %s\n", config_file);
        return config;
    }

    char line[1024];
    while (fgets(line, 1024, fp)) {
        char *token = strtok(line, "=");
        if (token) {
            char *value = strtok(NULL, "\n");
            if (value) {
                if (strcmp(token, "show_config_message") == 0) {
                    config.show_config_message = atoi(value);
                    if (config.show_config_message == 1) {
                        printf("Configuration file is located at %s. You can disable this message by setting 'show_config_message=0' in it.\n", config_file);
                    }
                } else if (strcmp(token, "show_family_name") == 0) {
                    config.show_family_name = atoi(value);
                } else if (strcmp(token, "show_bios_if_ver") == 0) {
                    config.show_bios_if_ver = atoi(value);
                } else if (strcmp(token, "show_ryzenadj_ver") == 0) {
                    config.show_ryzenadj_ver = atoi(value);
                } else if (strcmp(token, "show_table_ver") == 0) {
                    config.show_table_ver = atoi(value);
                } else if (strcmp(token, "show_stapm_limit") == 0) {
                    config.show_stapm_limit = atoi(value);
                } else if (strcmp(token, "show_stapm_limit_value") == 0) {
                    config.show_stapm_limit_value = atoi(value);
                } else if (strcmp(token, "show_fast_limit") == 0) {
                    config.show_fast_limit = atoi(value);
                } else if (strcmp(token, "show_fast_limit_value") == 0) {
                   config.show_fast_limit_value = atoi(value);
                } else if (strcmp(token, "show_slow_limit") == 0) {
                   config.show_slow_limit = atoi(value);
                } else if (strcmp(token, "show_slow_limit_value") == 0) {
                   config.show_slow_limit_value = atoi(value);
                } else if (strcmp(token, "show_stapm_time") == 0) {
                   config.show_stapm_time = atoi(value);
                } else if (strcmp(token, "show_slow_time") == 0) {
                   config.show_slow_time = atoi(value);
                } else if (strcmp(token, "show_apu_slow_limit") == 0) {
                   config.show_apu_slow_limit = atoi(value);
                } else if (strcmp(token, "show_apu_slow_limit_value") == 0) {
                   config.show_apu_slow_limit_value = atoi(value);
                } else if (strcmp(token, "show_vrm_current") == 0) {
                   config.show_vrm_current = atoi(value);
                } else if (strcmp(token, "show_vrm_current_value") == 0) {
                   config.show_vrm_current_value = atoi(value);
                } else if (strcmp(token, "show_vrmsoc_current") == 0) {
                   config.show_vrmsoc_current = atoi(value);
                } else if (strcmp(token, "show_vrmsoc_current_value") == 0) {
                   config.show_vrmsoc_current_value = atoi(value);
                } else if (strcmp(token, "show_vrmsocmax_current") == 0) {
                   config.show_vrmsocmax_current = atoi(value);
                } else if (strcmp(token, "show_vrmsocmax_current_value") == 0) {
                   config.show_vrmsocmax_current_value = atoi(value);
                } else if (strcmp(token, "show_tctl_temp") == 0) {
                   config.show_tctl_temp = atoi(value);
                } else if (strcmp(token, "show_tctl_temp_value") == 0) {
                   config.show_tctl_temp_value = atoi(value);
                } else if (strcmp(token, "show_apu_skin_temp") == 0) {
                   config.show_apu_skin_temp = atoi(value);
                } else if (strcmp(token, "show_apu_skin_temp_value") == 0) {
                   config.show_apu_skin_temp_value = atoi(value);
                } else if (strcmp(token, "show_dgpu_skin_temp") == 0) {
                   config.show_dgpu_skin_temp = atoi(value);
                } else if (strcmp(token, "show_dgpu_skin_temp_value") == 0) {
                   config.show_dgpu_skin_temp_value = atoi(value);
                } else if (strcmp(token, "show_power_saving") == 0) {
                   config.show_power_saving = atoi(value);
                } else if (strcmp(token, "show_max_performance") == 0) {
                   config.show_max_performance = atoi(value);
                }
            }
        }
    }

    fclose(fp);
    return config;
}