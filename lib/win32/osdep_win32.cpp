// SPDX-License-Identifier: LGPL
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* Access PCI Config Space - winring0 */
extern "C" {
#include "../nb_smu_ops.h"
}
#include <cstdlib>

#include "OlsApi.h"
#include "OlsDef.h"

typedef BOOL (__stdcall *lpIsInpOutDriverOpen)(void);
typedef PBYTE (__stdcall *lpMapPhysToLin)(uintptr_t pbPhysAddr, size_t dwPhysSize, HANDLE *pPhysicalMemoryHandle);
typedef BOOL (__stdcall *lpUnmapPhysicalMemory)(HANDLE PhysicalMemoryHandle, uintptr_t pbLinAddr);
typedef BOOL (__stdcall *lpGetPhysLong)(uintptr_t pbPhysAddr, uint32_t *pdwPhysVal);
lpIsInpOutDriverOpen gfpIsInpOutDriverOpen;
lpGetPhysLong gfpGetPhysLong;
lpMapPhysToLin gfpMapPhysToLin;
lpUnmapPhysicalMemory gfpUnmapPhysicalMemory;
uint32_t *pdwLinAddr;
HANDLE physicalMemoryHandle;

#ifdef __cplusplus
extern "C" {
#endif

os_access_obj_t *init_os_access_obj() {
    InitializeOls();

    const DWORD dllStatus = GetDllStatus();

    switch (dllStatus) {
        case OLS_DLL_NO_ERROR: {
            os_access_obj_t *obj = static_cast<os_access_obj_t *>(std::malloc(sizeof(os_access_obj_t)));

            if (obj == NULL)
                return NULL;

            memset(obj, 0, sizeof(os_access_obj_t));
            return obj;
        }
        case OLS_DLL_UNSUPPORTED_PLATFORM:
            DBG("WinRing0 Err: Unsupported Platform\n");
            break;
        case OLS_DLL_DRIVER_NOT_LOADED:
            DBG("WinRing0 Err: Driver not loaded\n");
            break;
        case OLS_DLL_DRIVER_NOT_FOUND:
            DBG("WinRing0 Err: Driver not found\n");
            break;
        case OLS_DLL_DRIVER_UNLOADED:
            DBG("WinRing0 Err: Driver unloaded\n");
            break;
        case OLS_DLL_DRIVER_NOT_LOADED_ON_NETWORK:
            DBG("WinRing0 Err: Driver not loaded on network\n");
            break;
        case OLS_DLL_UNKNOWN_ERROR:
            DBG("WinRing0 Err: unknown error\n");
            break;
        default:
            DBG("WinRing0 Err: unknown status code %lu\n", dllStatus);
            break;
    }

    return NULL;
}

void free_os_access_obj(os_access_obj_t *obj) {
    if (obj == NULL)
        return;

    DeinitializeOls();
    gfpUnmapPhysicalMemory(physicalMemoryHandle, *pdwLinAddr);
    FreeLibrary(obj->inpoutDll);
    free(obj);
}

uint32_t smn_reg_read(const os_access_obj_t *obj, uint32_t addr) {
    WritePciConfigDword(obj->pci_address, NB_PCI_REG_ADDR_ADDR, addr & (~0x3));
    return ReadPciConfigDword(obj->pci_address, NB_PCI_REG_DATA_ADDR);
}

void smn_reg_write(const os_access_obj_t *obj, uint32_t addr, uint32_t data) {
    WritePciConfigDword(obj->pci_address, NB_PCI_REG_ADDR_ADDR, addr);
    WritePciConfigDword(obj->pci_address, NB_PCI_REG_DATA_ADDR, data);
}

int init_mem_obj(os_access_obj_t *os_access, uintptr_t physAddr) {
    HINSTANCE hInpOutDll = LoadLibrary ("inpoutx64.DLL");

    if (hInpOutDll == NULL) {
        DBG("Could not load inpoutx64.DLL\n");
        return -1;
    }

    gfpIsInpOutDriverOpen = (lpIsInpOutDriverOpen)GetProcAddress(hInpOutDll, "IsInpOutDriverOpen");
    gfpGetPhysLong = (lpGetPhysLong)GetProcAddress(hInpOutDll, "GetPhysLong");
    gfpMapPhysToLin = (lpMapPhysToLin)GetProcAddress(hInpOutDll, "MapPhysToLin");
    gfpUnmapPhysicalMemory = (lpUnmapPhysicalMemory)GetProcAddress(hInpOutDll, "UnmapPhysicalMemory");

    if (!gfpIsInpOutDriverOpen()) {
        DBG("Could not open inpoutx64 driver\n");
        return -1;
    }

    pdwLinAddr = (uint32_t*)gfpMapPhysToLin(physAddr, 0x1000, &physicalMemoryHandle);
    if (pdwLinAddr == NULL) {
        DBG("failed to map memory\n");
        return -1;
    }

    os_access->inpoutDll = hInpOutDll;

    return 0;
}

int copy_pm_table(const os_access_obj_t *obj, void *buffer, const size_t size) {
    memcpy(buffer, pdwLinAddr, size);
    return 0;
}

int compare_pm_table(const void *buffer, const size_t size) {
    return memcmp(buffer, pdwLinAddr, size);
}

bool is_using_smu_driver() {
    return false;
}

#ifdef __cplusplus
}
#endif
