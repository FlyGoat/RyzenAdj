/* SPDX-License-Identifier: LGPL */
/* Copyright (C) 2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* RyzenAdj API */

#ifndef RYZENADJ_H
#define RYZENADJ_H
#ifdef __cplusplus
extern "C" {
#endif

#define RYZENADJ_REVISION_VER 0
#define RYZENADJ_MAJOR_VER 7
#define RYZENADJ_MINIOR_VER 1

enum ryzen_family {
        FAM_UNKNOWN = -1,
        FAM_RAVEN = 0,
        FAM_PICASSO,
        FAM_RENOIR,
        FAM_CEZANNE,
        FAM_DALI,
        FAM_LUCIENNE,
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

typedef struct _ryzen_access *ryzen_access;

EXP ryzen_access CALL init_ryzenadj();

EXP void CALL cleanup_ryzenadj(ryzen_access ry);

EXP enum ryzen_family get_cpu_family(ryzen_access ry);
EXP int get_bios_if_ver(ryzen_access ry);

EXP int CALL set_stapm_limit(ryzen_access, uint32_t value);
EXP int CALL set_fast_limit(ryzen_access, uint32_t value);
EXP int CALL set_slow_limit(ryzen_access, uint32_t value);
EXP int CALL set_slow_time(ryzen_access, uint32_t value);
EXP int CALL set_stapm_time(ryzen_access, uint32_t value);
EXP int CALL set_tctl_temp(ryzen_access, uint32_t value);
EXP int CALL set_vrm_current(ryzen_access, uint32_t value);
EXP int CALL set_vrmsoc_current(ryzen_access, uint32_t value);
EXP int CALL set_vrmmax_current(ryzen_access, uint32_t value);
EXP int CALL set_vrmsocmax_current(ryzen_access, uint32_t value);
EXP int CALL set_psi0_current(ryzen_access, uint32_t value);
EXP int CALL set_psi0soc_current(ryzen_access, uint32_t value);
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


#ifdef __cplusplus
}
#endif
#endif
