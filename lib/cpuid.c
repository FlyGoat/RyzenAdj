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
