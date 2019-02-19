/* SPDX-License-Identifier: LGPL */
/* Copyright (C) 2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* RyzenAdj API */

#ifndef RYZENADJ_H
#define RYZENADJ_H
#ifdef __cplusplus
extern "C" {
#endif
#include  "nb_smu_ops.h"

typedef struct {
    nb_t nb;
    pci_obj_t pci_obj;
    smu_t mp1_smu;
    smu_t psmu;
} *ryzen_access;

ryzen_access init_ryzenadj();

void cleanup_ryzenadj(ryzen_access ry);

int set_stapm_limit(ryzen_access, uint32_t value);
int set_fast_limit(ryzen_access, uint32_t value);
int set_slow_limit(ryzen_access, uint32_t value);
int set_slow_time(ryzen_access, uint32_t value);
int set_stapm_time(ryzen_access, uint32_t value);
int set_tctl_temp(ryzen_access, uint32_t value);
int set_vrm_current(ryzen_access, uint32_t value);
int set_vrmsoc_current(ryzen_access, uint32_t value);
int set_vrmmax_current(ryzen_access, uint32_t value);
int set_vrmsocmax_current(ryzen_access, uint32_t value);
int set_psi0_current(ryzen_access, uint32_t value);
int set_psi0soc_current(ryzen_access, uint32_t value);



#ifdef __cplusplus
}
#endif
#endif
