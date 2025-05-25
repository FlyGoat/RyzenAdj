/* SPDX-License-Identifier: LGPL */
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* Ryzen NB SMU Service Request Opreations */

#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

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


/* OS depdent part*/
#if defined _WIN32
#include <Windows.h>
typedef uint32_t *nb_t;
typedef bool *pci_obj_t;
typedef HINSTANCE *mem_obj_t;
#elif defined LINUX_USE_RYZEN_SMU_MODULE
typedef struct smu_kernel_module smu_kernel_module_t;
typedef smu_kernel_module_t *pci_obj_t;
typedef smu_kernel_module_t *nb_t;
typedef void *mem_obj_t;
#else
#include <pci/pci.h>
typedef struct pci_dev *nb_t;
typedef struct pci_access *pci_obj_t;
typedef bool *mem_obj_t;
#endif

typedef struct _smu_t {
	nb_t nb;
	uint32_t msg;
	uint32_t rep;
	uint32_t arg_base;
} *smu_t;


pci_obj_t init_pci_obj();

void free_pci_obj(pci_obj_t obj);

nb_t get_nb(pci_obj_t obj);

void free_nb(nb_t nb);

uint32_t smn_reg_read(nb_t nb, uint32_t addr);

void smn_reg_write(nb_t nb, uint32_t addr, uint32_t data);


smu_t get_smu(nb_t nb, int smu_type);
void free_smu(smu_t smu);
uint32_t smu_service_req(smu_t smu ,uint32_t id ,smu_service_args_t *args);

mem_obj_t init_mem_obj(uintptr_t physAddr);

void free_mem_obj(mem_obj_t obj);

int copy_pm_table(nb_t nb, void *buffer, size_t size);

int compare_pm_table(void *buffer, size_t size);

bool is_using_smu_driver();
