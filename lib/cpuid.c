// SPDX-License-Identifier: LGPL
/* Copyright (C) 2020 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* RyzenAdj CPU ID stuff */

#include "ryzenadj.h"

#include <string.h>

#if defined(__GNUC__)
#include <cpuid.h>
#elif defined (_WIN32)
#include <intrin.h>
#endif

#define CPUID_VENDOR_AMD          "AuthenticAMD"

static void getcpuid(unsigned int CPUInfo[4], unsigned int InfoType)
{
#if defined(__GNUC__)
    __cpuid(InfoType, CPUInfo[0],CPUInfo[1],CPUInfo[2],CPUInfo[3]);
#elif defined(_WIN32)
    __cpuid((int*)(void*)CPUInfo, (int)InfoType);
#endif
}

enum ryzen_family cpuid_get_family()
{
    uint32_t regs[4];
    int family, model;
    char vendor[4 * 4];

    getcpuid(regs, 0);

    /* Hack Alert! Put into str buffer */
    *(uint32_t *) &vendor[0] = regs[1];
    *(uint32_t *) &vendor[4] = regs[3];
    *(uint32_t *) &vendor[8] = regs[2];

    if (strncmp((char *) &vendor, CPUID_VENDOR_AMD , sizeof(CPUID_VENDOR_AMD) - 1)) {
        printf("Not AMD processor, must be kidding\n");
        return FAM_UNKNOWN;
    }

    getcpuid(regs, 1);

    family = ((regs[0] >> 8) & 0xf) + ((regs[0] >> 20) & 0xff);
    model = ((regs[0] >> 4) & 0xf) | ((regs[0] >> 12) & 0xf0);

    if (family != 0x17) {
        printf("Not Zen processor, won't work\n");
        return FAM_UNKNOWN; 
    }

    switch(model) {
    case 17:
        printf("Detected Raven\n");
        return FAM_RAVEN;
        break;
    case 24:
        printf("Detected Picasso\n");
        return FAM_PICASSO;
        break;
    case 96:
        printf("Detected Renior\n");
        return FAM_RENIOR;
        break;
    default:
        printf("Unknown Ryzen processor, won't work\n");
        break;
    }

    return FAM_UNKNOWN;
}
