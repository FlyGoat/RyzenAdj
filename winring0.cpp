// SPDX-License-Identifier: LGPL
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* Access PCI Config Space - winring0 */

#include "nb_smu_ops.h"
#include "Windows.h"
#include "OlsApi.h"
#include "OlsDef.h"


bool nb_pci_obj = true;
uint32_t nb_pci_address = 0x0;

pci_obj_t init_pci_obj(){
    InitializeOls();
    if(GetDllStatus() == 0)
        return &nb_pci_obj;
    printf("WinRing0 Err: 0x%x",GetDllStatus());
    return NULL;
}

nb_t get_nb(pci_obj_t obj){
    return &nb_pci_address;
}

void free_nb(nb_t){
    return;
}

void free_pci_obj(pci_obj_t obj){
    DeinitializeOls();
}

u32 smn_reg_read(nb_t nb, u32 addr)
{
    WritePciConfigDword(*nb, NB_PCI_REG_ADDR_ADDR, (addr & (~0x3)));
    return ReadPciConfigDword(*nb, NB_PCI_REG_DATA_ADDR);
}

void smn_reg_write(nb_t nb, u32 addr, u32 data)
{
    WritePciConfigDword(*nb, NB_PCI_REG_ADDR_ADDR, addr);
    WritePciConfigDword(*nb, NB_PCI_REG_DATA_ADDR, data);
}
