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

static enum ryzen_family cpuid_family = WAIT_FOR_LOAD;

static enum ryzen_family cpuid_load_family()
{
    uint32_t regs[4];
    int family, model;
    char vendor[4 * 4];

    getcpuid(regs, 0);

    /* Hack Alert! Put into str buffer */
    *(uint32_t *) &vendor[0] = regs[1];
    *(uint32_t *) &vendor[4] = regs[3];
    *(uint32_t *) &vendor[8] = regs[2];

    if (strncmp(vendor, CPUID_VENDOR_AMD, strlen(CPUID_VENDOR_AMD))) {
        printf("Not AMD processor, must be kidding\n");
        return FAM_UNKNOWN;
    }

    getcpuid(regs, 1);

    family = ((regs[0] >> 8) & 0xf) + ((regs[0] >> 20) & 0xff);
    model = ((regs[0] >> 4) & 0xf) | ((regs[0] >> 12) & 0xf0);

    switch (family) {
    case 0x17: /* Zen, Zen+, Zen2 */
        switch (model) {
        case 17:
            return FAM_RAVEN;
        case 24:
            return FAM_PICASSO;
        case 32:
            return FAM_DALI;
        case 96:
            return FAM_RENOIR;
        case 104:
            return FAM_LUCIENNE;
        case 144:
            return FAM_VANGOGH;
        case 160:
            return FAM_MENDOCINO;
        default:
            printf("Fam%xh: unsupported model %d\n", family, model);
            break;
        };
        break;

    case 0x19: /* Zen3 */
        switch (model) {
        case 80:
            return FAM_CEZANNE;
        case 64:
        case 68:
            return FAM_REMBRANDT;
        case 116:
            return FAM_PHOENIX;
        default:
            printf("Fam%xh: unsupported model %d\n", family, model);
            break;
        };
        break;

    default:
        printf("Unsupported family: %xh\n", family);
        break;
    }

    printf("Only Ryzen Mobile Series are supported\n");
    return FAM_UNKNOWN;
}

/*
 * The function cpuid_load_family() processes a lot of
 * information. This information will be used several
 * times by acquiring the same data. Avoid this wasted
 * computation by entering cpuid_get_family(). This function
 * guarantees the load is done only 1 time.
 */
enum ryzen_family cpuid_get_family() {
    if (cpuid_family == WAIT_FOR_LOAD)
        cpuid_family = cpuid_load_family();

    return cpuid_family;
}