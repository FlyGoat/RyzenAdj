/* SPDX-License-Identifier: LGPL */
/* Copyright (C) 2020 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* RyzenAdj Private stuff */
/* Do not include this file! */

#ifndef RYZENADJ_PRIV_H
#define RYZENADJ_PRIV_H

#include <stdint.h>

#include  "nb_smu_ops.h"

struct _ryzen_access {
	os_access_obj_t *os_access;
	smu_t mp1_smu;
	smu_t psmu;
	enum ryzen_family family;
	int bios_if_ver;
	uintptr_t table_addr;
	uint32_t table_ver;
	size_t table_size;
	float *table_values;
};

enum ryzen_family cpuid_get_family();

#endif
