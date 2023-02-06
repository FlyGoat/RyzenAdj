/* SPDX-License-Identifier: LGPL */
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* Ryzen NB SMU Service Request Opreations */

#ifndef NB_SMU_OPS_H
#define NB_SMU_OPS_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint64_t u64;

#ifdef NDEBUG
#define DBG(...)
#else
#define DBG(...) fprintf(stderr, __VA_ARGS__)
#endif

#define NB_PCI_REG_ADDR_ADDR 0xB8
#define NB_PCI_REG_DATA_ADDR 0xBC

#define C2PMSG_ARGx_ADDR(y, x)          (y + 4 * x)

enum SMU_TYPE{
	TYPE_MP1,
	TYPE_PSMU,
	TYPE_COUNT,
};

#define MP1_C2PMSG_MESSAGE_ADDR_1        0x3B10528
#define MP1_C2PMSG_RESPONSE_ADDR_1       0x3B10564
#define MP1_C2PMSG_ARG_BASE_1            0x3B10998

/* For Vangogh and Rembrandt */
#define MP1_C2PMSG_MESSAGE_ADDR_2        0x3B10528
#define MP1_C2PMSG_RESPONSE_ADDR_2       0x3B10578
#define MP1_C2PMSG_ARG_BASE_2            0x3B10998

#define PSMU_C2PMSG_MESSAGE_ADDR          0x3B10a20
#define PSMU_C2PMSG_RESPONSE_ADDR         0x3B10a80
#define PSMU_C2PMSG_ARG_BASE              0x3B10a88

#define REP_MSG_OK                    0x1
#define REP_MSG_Failed                0xFF
#define REP_MSG_UnknownCmd            0xFE
#define REP_MSG_CmdRejectedPrereq     0xFD
#define REP_MSG_CmdRejectedBusy       0xFC

/*
* All the SMU have the same TestMessage as for now
* Correct me if they don't
*/
#define SMU_TEST_MSG 0x1

typedef struct _smu_service_args_t {
		u32 arg0;
		u32 arg1;
		u32 arg2;
		u32 arg3;
		u32 arg4;
		u32 arg5;
} smu_service_args_t;


/* OS depdent part*/
#if defined _WIN32
#include <Windows.h>
typedef uint32_t *nb_t;
typedef bool *pci_obj_t;
typedef HINSTANCE *mem_obj_t;
#else
#include <pci/pci.h>
typedef struct pci_dev *nb_t;
typedef struct pci_access *pci_obj_t;
typedef bool *mem_obj_t;
#endif

typedef struct _smu_t {
	nb_t nb;
	u32 msg;
	u32 rep;
	u32 arg_base;
} *smu_t;


pci_obj_t init_pci_obj();

void free_pci_obj(pci_obj_t obj);

nb_t get_nb(pci_obj_t obj);

void free_nb(nb_t nb);

u32 smn_reg_read(nb_t nb, u32 addr);

void smn_reg_write(nb_t nb, u32 addr, u32 data);


smu_t get_smu(nb_t nb, int smu_type);
void free_smu(smu_t smu);
u32 smu_service_req(smu_t smu ,u32 id ,smu_service_args_t *args);

mem_obj_t init_mem_obj(uintptr_t physAddr);

void free_mem_obj(mem_obj_t obj);

int copy_pm_table(void *buffer, size_t size);

int compare_pm_table(void *buffer, size_t size);

bool is_using_smu_driver();
#endif
