// SPDX-License-Identifier: LGPL
#pragma once

#include "../nb_smu_ops.h"

os_access_obj_t *init_os_access_obj_kmod();
int init_mem_obj_kmod(os_access_obj_t *os_access, uintptr_t physAddr);
int copy_pm_table_kmod(const os_access_obj_t *obj, void *buffer, size_t size);
int compare_pm_table_kmod(const void *buffer, size_t size);
void free_os_access_obj_kmod(os_access_obj_t *obj);

uint32_t smn_reg_read_kmod(const os_access_obj_t *obj, uint32_t addr);
void smn_reg_write_kmod(const os_access_obj_t *obj, uint32_t addr, uint32_t data);
