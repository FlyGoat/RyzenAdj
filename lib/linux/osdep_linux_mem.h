// SPDX-License-Identifier: LGPL
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* Access PCI Config Space - libpci */
#pragma once

#include "../nb_smu_ops.h"

os_access_obj_t *init_os_access_obj_mem();
int init_mem_obj_mem(os_access_obj_t *os_access, uintptr_t physAddr);
int copy_pm_table_mem(const os_access_obj_t *obj, void *buffer, size_t size);
int compare_pm_table_mem(const void *buffer, size_t size);
void free_os_access_obj_mem(os_access_obj_t *obj);

uint32_t smn_reg_read_mem(const os_access_obj_t *obj, uint32_t addr);
void smn_reg_write_mem(const os_access_obj_t *obj, uint32_t addr, uint32_t data);
