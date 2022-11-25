// SPDX-License-Identifier: LGPL
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* Access PCI Config Space - winring0 */

extern "C" {
#include "nb_smu_ops.h"
}
#include "Windows.h"
#include "OlsApi.h"
#include "OlsDef.h"

#pragma comment(lib, "../win32/WinRing0x64.lib")

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
u32 *pdwLinAddr;
HANDLE physicalMemoryHandle;



extern "C" pci_obj_t init_pci_obj(){
    InitializeOls();
    int dllStatus = GetDllStatus();
    switch(dllStatus)
    {
        case OLS_DLL_NO_ERROR: return &nb_pci_obj;
        case OLS_DLL_UNSUPPORTED_PLATFORM:
            printf("WinRing0 Err: Unsupported Plattform\n");
            break;
        case OLS_DLL_DRIVER_NOT_LOADED:
            printf("WinRing0 Err: Driver not loaded\n");
            break;
        case OLS_DLL_DRIVER_NOT_FOUND:
            printf("WinRing0 Err: Driver not found\n");
            break;
        case OLS_DLL_DRIVER_UNLOADED:
            printf("WinRing0 Err: Driver unloaded\n");
            break;
        case OLS_DLL_DRIVER_NOT_LOADED_ON_NETWORK:
            printf("WinRing0 Err: Driver not loaded on network\n");
            break;
        case OLS_DLL_UNKNOWN_ERROR:
            printf("WinRing0 Err: unknown error\n");
            break;
        default:
            printf("WinRing0 Err: unknown status code %d\n", dllStatus);
    }
    
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

extern "C" mem_obj_t init_mem_obj(uintptr_t physAddr){
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

    pdwLinAddr = (u32*)gfpMapPhysToLin(physAddr, 0x1000, &physicalMemoryHandle);
    if (pdwLinAddr == NULL){
        printf("failed to map memory\n");
        return NULL;
    }

    return &hInpOutDll;
}

extern "C" void free_mem_obj(mem_obj_t hInpOutDll)
{
    gfpUnmapPhysicalMemory(physicalMemoryHandle, *pdwLinAddr);
    FreeLibrary((HINSTANCE)hInpOutDll);
}

extern "C" int copy_pm_table(void *buffer, size_t size)
{
    memcpy(buffer, pdwLinAddr, size);
    return 0;
}

extern "C" int compare_pm_table(void *buffer, size_t size)
{
    return memcmp(buffer, pdwLinAddr, size);
}

extern "C" bool is_using_smu_driver()
{
    return false;
}
