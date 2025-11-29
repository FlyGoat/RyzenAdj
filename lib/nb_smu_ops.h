/* SPDX-License-Identifier: LGPL */
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* Ryzen NB SMU Service Request Operations */

#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef NDEBUG
#define DBG(...)
#else
#define DBG(...) fprintf(stderr, __VA_ARGS__)
#endif

enum SMU_TYPE{
	TYPE_MP1,
	TYPE_PSMU,
	TYPE_COUNT,
};

#define NB_PCI_REG_ADDR_ADDR			0xB8
#define NB_PCI_REG_DATA_ADDR			0xBC

#define REP_MSG_OK                    0x1
#define REP_MSG_Failed                0xFF
#define REP_MSG_UnknownCmd            0xFE
#define REP_MSG_CmdRejectedPrereq     0xFD
#define REP_MSG_CmdRejectedBusy       0xFC

typedef struct _smu_service_args_t {
		uint32_t arg0;
		uint32_t arg1;
		uint32_t arg2;
		uint32_t arg3;
		uint32_t arg4;
		uint32_t arg5;
} smu_service_args_t;

typedef struct {
#ifdef _WIN32
	uint32_t pci_address;
	HINSTANCE inpoutDll;
#else
	union {
		struct {
			struct pci_access *pci_acc;
			struct pci_dev *pci_dev;
		} mem;
		struct {
			int smn_fd;
			int pm_table_fd;
			size_t pm_table_size;
		} kmod;
	} access;
#endif
} os_access_obj_t;

typedef struct _smu_t {
	os_access_obj_t *os_access;
	uint32_t msg;
	uint32_t rep;
	uint32_t arg_base;
} *smu_t;

os_access_obj_t *init_os_access_obj();
int init_mem_obj(os_access_obj_t *os_access, uintptr_t physAddr);
int copy_pm_table(const os_access_obj_t *obj, void *buffer, size_t size);
int compare_pm_table(const void *buffer, size_t size);
void free_os_access_obj(os_access_obj_t *obj);

uint32_t smn_reg_read(const os_access_obj_t *obj, uint32_t addr);
void smn_reg_write(const os_access_obj_t *obj, uint32_t addr, uint32_t data);
bool is_using_smu_driver();

smu_t get_smu(os_access_obj_t *obj, int smu_type);
uint32_t smu_service_req(smu_t smu, uint32_t id, smu_service_args_t *args);
