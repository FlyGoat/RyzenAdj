/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* Ryzen NB SMU Service Request Opreations */

#ifndef NB_SMU_OPS_H
#define NB_SMU_OPS_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint64_t u64;

#define DBG(...)

#define AMD_VENDOR_ID 0x1022
#define NB_DEVICE_ID 0x15d0

#define NB_PCI_REG_ADDR_ADDR 0xB8
#define NB_PCI_REG_DATA_ADDR 0xBC

#define MP1_C2PMSG_MESSAGE_ADDR          0x3B10528
#define MP1_C2PMSG_RESPONSE_ADDR         0x3B10564
#define MP1_C2PMSG_ARG_BASE              0x3B10998
#define MP1_C2PMSG_ARGx_ADDR(x)          (MP1_C2PMSG_ARG_BASE + 4 * x)


#define REP_MSG_OK                    0x1
#define REP_MSG_Failed                0xFF
#define REP_MSG_UnknownCmd            0xFE
#define REP_MSG_CmdRejectedPrereq     0xFD
#define REP_MSG_CmdRejectedBusy       0xFC

typedef struct {
    u32 arg0;
    u32 arg1;
    u32 arg2;
    u32 arg3;
    u32 arg4;
    u32 arg5;
} smu_service_args_t;


/* OS depdent part*/
#if defined _WIN32
typedef uint32_t *nb_t;
typedef bool *pci_obj_t;
#else
extern "C"
{
  #include <pci/pci.h>
}
typedef struct pci_dev *nb_t;
typedef struct pci_access *pci_obj_t;
#endif

pci_obj_t init_pci_obj();

int free_pci_obj(pci_obj_t obj);

nb_t get_nb(pci_obj_t obj);

u32 nb_reg_read(nb_t nb, u32 addr);

void nb_reg_write(nb_t nb, u32 addr, u32 data);

u32 smu_service_req(nb_t nb ,u32 id ,smu_service_args_t *args);


#endif