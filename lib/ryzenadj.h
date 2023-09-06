/* SPDX-License-Identifier: LGPL */
/* Copyright (C) 2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* RyzenAdj API */

#ifndef RYZENADJ_H
#define RYZENADJ_H
#ifdef __cplusplus
extern "C" {
#endif

#define RYZENADJ_REVISION_VER 0
#define RYZENADJ_MAJOR_VER 14
#define RYZENADJ_MINIOR_VER 0

enum ryzen_family {
        WAIT_FOR_LOAD = -2,
        FAM_UNKNOWN = -1,
        FAM_RAVEN = 0,
        FAM_PICASSO,
        FAM_RENOIR,
        FAM_CEZANNE,
        FAM_DALI,
        FAM_LUCIENNE,
        FAM_VANGOGH,
        FAM_REMBRANDT,
        FAM_MENDOCINO,
        FAM_PHOENIX,
        FAM_END
};

#ifdef _LIBRYZENADJ_INTERNAL
#include  "ryzenadj_priv.h"

#ifdef _WIN32
#define EXP __declspec(dllexport)
#define CALL __stdcall
#else
#define EXP __attribute__((visibility("default")))
#define CALL
#endif

#else

#ifdef _WIN32
#define EXP __declspec(dllimport)
#define CALL __stdcall
#else
#define EXP
#define CALL
#endif
struct _ryzen_access;

#endif

#define ADJ_ERR_FAM_UNSUPPORTED      -1
#define ADJ_ERR_SMU_TIMEOUT          -2
#define ADJ_ERR_SMU_UNSUPPORTED      -3
#define ADJ_ERR_SMU_REJECTED         -4
#define ADJ_ERR_MEMORY_ACCESS        -5

typedef struct _ryzen_access *ryzen_access;

EXP ryzen_access CALL init_ryzenadj();

EXP void CALL cleanup_ryzenadj(ryzen_access ry);

EXP enum ryzen_family get_cpu_family(ryzen_access ry);
EXP int get_bios_if_ver(ryzen_access ry);

EXP int CALL init_table(ryzen_access ry);
EXP uint32_t CALL get_table_ver(ryzen_access ry);
EXP size_t CALL get_table_size(ryzen_access ry);
EXP float* CALL get_table_values(ryzen_access ry);
EXP int CALL refresh_table(ryzen_access ry);

EXP int CALL set_stapm_limit(ryzen_access, uint32_t value);
EXP int CALL set_fast_limit(ryzen_access, uint32_t value);
EXP int CALL set_slow_limit(ryzen_access, uint32_t value);
EXP int CALL set_slow_time(ryzen_access, uint32_t value);
EXP int CALL set_stapm_time(ryzen_access, uint32_t value);
EXP int CALL set_tctl_temp(ryzen_access, uint32_t value);
EXP int CALL set_vrm_current(ryzen_access, uint32_t value);
EXP int CALL set_vrmsoc_current(ryzen_access, uint32_t value);
EXP int CALL set_vrmgfx_current(ryzen_access, uint32_t value);
EXP int CALL set_vrmcvip_current(ryzen_access, uint32_t value);
EXP int CALL set_vrmmax_current(ryzen_access, uint32_t value);
EXP int CALL set_vrmgfxmax_current(ryzen_access, uint32_t value);
EXP int CALL set_vrmsocmax_current(ryzen_access, uint32_t value);
EXP int CALL set_psi0_current(ryzen_access, uint32_t value);
EXP int CALL set_psi3cpu_current(ryzen_access, uint32_t value);
EXP int CALL set_psi0soc_current(ryzen_access, uint32_t value);
EXP int CALL set_psi3gfx_current(ryzen_access, uint32_t value);
EXP int CALL set_max_gfxclk_freq(ryzen_access, uint32_t value);
EXP int CALL set_min_gfxclk_freq(ryzen_access, uint32_t value);
EXP int CALL set_max_socclk_freq(ryzen_access, uint32_t value);
EXP int CALL set_min_socclk_freq(ryzen_access, uint32_t value);
EXP int CALL set_max_fclk_freq(ryzen_access, uint32_t value);
EXP int CALL set_min_fclk_freq(ryzen_access, uint32_t value);
EXP int CALL set_max_vcn(ryzen_access, uint32_t value);
EXP int CALL set_min_vcn(ryzen_access, uint32_t value);
EXP int CALL set_max_lclk(ryzen_access, uint32_t value);
EXP int CALL set_min_lclk(ryzen_access, uint32_t value);
EXP int CALL set_prochot_deassertion_ramp(ryzen_access ry, uint32_t value);
EXP int CALL set_apu_skin_temp_limit(ryzen_access, uint32_t value);
EXP int CALL set_dgpu_skin_temp_limit(ryzen_access, uint32_t value);
EXP int CALL set_apu_slow_limit(ryzen_access, uint32_t value);
EXP int CALL set_skin_temp_power_limit(ryzen_access ry, uint32_t value);
EXP int CALL set_gfx_clk(ryzen_access ry, uint32_t value);
EXP int CALL set_oc_clk(ryzen_access ry, uint32_t value);
EXP int CALL set_per_core_oc_clk(ryzen_access ry, uint32_t value);
EXP int CALL set_oc_volt(ryzen_access ry, uint32_t value);
EXP int CALL set_disable_oc(ryzen_access ry);
EXP int CALL set_enable_oc(ryzen_access ry);
EXP int CALL set_power_saving(ryzen_access ry);
EXP int CALL set_max_performance(ryzen_access ry);
EXP int CALL set_coall(ryzen_access ry, uint32_t value);
EXP int CALL set_coper(ryzen_access ry, uint32_t value);
EXP int CALL set_cogfx(ryzen_access ry, uint32_t value);

EXP float CALL get_stapm_limit(ryzen_access ry);
EXP float CALL get_stapm_value(ryzen_access ry);
EXP float CALL get_fast_limit(ryzen_access ry);
EXP float CALL get_fast_value(ryzen_access ry);
EXP float CALL get_slow_limit(ryzen_access ry);
EXP float CALL get_slow_value(ryzen_access ry);
EXP float CALL get_apu_slow_limit(ryzen_access ry);
EXP float CALL get_apu_slow_value(ryzen_access ry);
EXP float CALL get_vrm_current(ryzen_access ry);
EXP float CALL get_vrm_current_value(ryzen_access ry);
EXP float CALL get_vrmsoc_current(ryzen_access ry);
EXP float CALL get_vrmsoc_current_value(ryzen_access ry);
EXP float CALL get_vrmmax_current(ryzen_access ry);
EXP float CALL get_vrmmax_current_value(ryzen_access ry);
EXP float CALL get_vrmsocmax_current(ryzen_access ry);
EXP float CALL get_vrmsocmax_current_value(ryzen_access ry);
EXP float CALL get_tctl_temp(ryzen_access ry);
EXP float CALL get_tctl_temp_value(ryzen_access ry);
EXP float CALL get_apu_skin_temp_limit(ryzen_access ry);
EXP float CALL get_apu_skin_temp_value(ryzen_access ry);
EXP float CALL get_dgpu_skin_temp_limit(ryzen_access ry);
EXP float CALL get_dgpu_skin_temp_value(ryzen_access ry);
EXP float CALL get_psi0_current(ryzen_access ry);
EXP float CALL get_psi0soc_current(ryzen_access ry);
EXP float CALL get_stapm_time(ryzen_access ry);
EXP float CALL get_slow_time(ryzen_access ry);
EXP float CALL get_cclk_setpoint(ryzen_access ry);
EXP float CALL get_cclk_busy_value(ryzen_access ry);

EXP float CALL get_core_clk(ryzen_access ry, uint32_t value);
EXP float CALL get_core_volt(ryzen_access ry, uint32_t value);
EXP float CALL get_core_power(ryzen_access ry, uint32_t value);
EXP float CALL get_core_temp(ryzen_access ry, uint32_t value);

EXP float CALL get_l3_clk(ryzen_access ry);
EXP float CALL get_l3_logic(ryzen_access ry);
EXP float CALL get_l3_vddm(ryzen_access ry);
EXP float CALL get_l3_temp(ryzen_access ry);

EXP float CALL get_gfx_clk(ryzen_access ry);
EXP float CALL get_gfx_temp(ryzen_access ry);
EXP float CALL get_gfx_volt(ryzen_access ry);

EXP float CALL get_mem_clk(ryzen_access ry);
EXP float CALL get_fclk(ryzen_access ry);

EXP float CALL get_soc_power(ryzen_access ry);
EXP float CALL get_soc_volt(ryzen_access ry);

EXP float CALL get_socket_power(ryzen_access ry);

#ifdef __cplusplus
}
#endif
#endif
