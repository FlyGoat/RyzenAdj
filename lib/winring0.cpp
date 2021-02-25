// SPDX-License-Identifier: LGPL
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* Access PCI Config Space - winring0 */

extern "C" {
#include "nb_smu_ops.h"
}
#include "Windows.h"
#include "OlsApi.h"
#include "OlsDef.h"

#pragma comment(lib, "../prebuilt/WinRing0x64.lib")
#pragma comment(lib, "../prebuilt/inpoutx64.lib")

bool nb_pci_obj = true;
uint32_t nb_pci_address = 0x0;
HINSTANCE hInpOutDll;

typedef BOOL (__stdcall *lpIsInpOutDriverOpen)(void);
typedef PBYTE (__stdcall *lpMapPhysToLin)(uintptr_t pbPhysAddr, size_t dwPhysSize, HANDLE *pPhysicalMemoryHandle);
typedef BOOL (__stdcall *lpUnmapPhysicalMemory)(HANDLE PhysicalMemoryHandle, uintptr_t pbLinAddr);
typedef BOOL (__stdcall *lpGetPhysLong)(uintptr_t pbPhysAddr, u32 *pdwPhysVal);
lpIsInpOutDriverOpen gfpIsInpOutDriverOpen;
lpGetPhysLong gfpGetPhysLong;
lpMapPhysToLin gfpMapPhysToLin;
lpUnmapPhysicalMemory gfpUnmapPhysicalMemory;



extern "C" pci_obj_t init_pci_obj(){
    InitializeOls();
    if(GetDllStatus() == 0)
        return &nb_pci_obj;
    printf("WinRing0 Err: 0x%lx",GetDllStatus());
    return NULL;
}

extern "C" nb_t get_nb(pci_obj_t obj){
    return &nb_pci_address;
}

extern "C" void free_nb(nb_t){
    return;
}

extern "C" void free_pci_obj(pci_obj_t obj){
    DeinitializeOls();
}

extern "C" u32 smn_reg_read(nb_t nb, u32 addr)
{
    WritePciConfigDword(*nb, NB_PCI_REG_ADDR_ADDR, (addr & (~0x3)));
    return ReadPciConfigDword(*nb, NB_PCI_REG_DATA_ADDR);
}

extern "C" void smn_reg_write(nb_t nb, u32 addr, u32 data)
{
    WritePciConfigDword(*nb, NB_PCI_REG_ADDR_ADDR, addr);
    WritePciConfigDword(*nb, NB_PCI_REG_DATA_ADDR, data);
}

extern "C" mem_obj_t init_mem_obj(){
    hInpOutDll = LoadLibrary ( "inpoutx64.DLL" );

    if(hInpOutDll == NULL)
    {
        printf("Could not load inpoutx64.DLL\n");
        return NULL;
    }

    gfpIsInpOutDriverOpen = (lpIsInpOutDriverOpen)GetProcAddress(hInpOutDll, "IsInpOutDriverOpen");
    gfpGetPhysLong = (lpGetPhysLong)GetProcAddress(hInpOutDll, "GetPhysLong");
    gfpMapPhysToLin = (lpMapPhysToLin)GetProcAddress(hInpOutDll, "MapPhysToLin");
    gfpUnmapPhysicalMemory = (lpUnmapPhysicalMemory)GetProcAddress(hInpOutDll, "UnmapPhysicalMemory");

    if(!gfpIsInpOutDriverOpen())
    {
        printf("Could not open inpoutx64 driver\n");
        return NULL;
    }

    return &hInpOutDll;
}

extern "C" void free_mem_obj(mem_obj_t hInpOutDll)
{
    FreeLibrary((HINSTANCE)hInpOutDll);
}


extern "C" char physical_memory_read_u32(u32 physAddr, u32 *data)
{
    if(!gfpIsInpOutDriverOpen())
        return -1;
    return gfpGetPhysLong((uintptr_t)physAddr, data);
}

extern "C" char physical_memory_read_data(u32 physAddr, u32 *data, size_t size)
{
    u32 *pdwLinAddr;
    HANDLE physicalMemoryHandle;

    if(!gfpIsInpOutDriverOpen())
        return -1;

    pdwLinAddr = (u32*)gfpMapPhysToLin((uintptr_t)physAddr, size, &physicalMemoryHandle);
    if (pdwLinAddr == NULL)
        return -1;

    memcpy(data, pdwLinAddr, size);
    gfpUnmapPhysicalMemory(physicalMemoryHandle, *pdwLinAddr);
    return true;
}
