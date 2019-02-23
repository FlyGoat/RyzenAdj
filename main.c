// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* Ryzen NB SMU Service Request Tool */

#include <string.h>
#include "lib/ryzenadj.h"
#include "argparse.h"
#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)

#define _do_adjust(ARG) \
do{ \
    while(ARG != 0){    \
        if(!set_##ARG(ry, ARG)){   \
            printf("Sucessfully Set " STRINGIFY(ARG) " to %x\n", ARG);    \
            break;  \
        } else {    \
            printf("Faild to set" STRINGIFY(ARG) " \n");   \
            err = -1; \
            break;  \
        }   \
    } \
}while(0);

static const char *const usage[] = {
    "ryzenadj [options] [[--] args]",
    "ryzenadj [options]",
    NULL,
};

int main(int argc, const char **argv)
{
    ryzen_access ry;
    int err = 0;

    uint32_t info = 0, stapm_limit = 0, fast_limit = 0, slow_limit = 0, slow_time = 0, stapm_time = 0, tctl_temp = 0;
    uint32_t vrm_current = 0, vrmsoc_current = 0, vrmmax_current = 0, vrmsocmax_current = 0, psi0_current = 0, psi0soc_current = 0;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_GROUP("Options"),
        OPT_BOOLEAN('i', "info", &info, "Show information (W.I.P.)"),     
        OPT_GROUP("Settings"),   
        OPT_U32('a', "stapm-limit", &stapm_limit, "Sustained power limit (10e-3 W)"),
        OPT_U32('b', "fast-limit", &fast_limit, "Fast PPT power limit (10e-3 W)"),
        OPT_U32('c', "slow-limit", &slow_limit, "Slow PPT power limit (10e-3 W)"),
        OPT_U32('d', "slow-time", &slow_time, "Slow PPT constant time (ms)"),
        OPT_U32('e', "stapm-time", &stapm_time, "STAMP constant time (ms)"),
        OPT_U32('f', "tctl-temp", &tctl_temp, "Tctl temperature (℃)"),
        OPT_U32('g', "vrm-current", &vrm_current, "Vrm Current Limit (mA)"),
        OPT_U32('j', "vrmsoc-current", &vrmsoc_current, "Vrm SoC Current Limit (mA)"),
        OPT_U32('k', "vrmmax-current", &vrmmax_current, "Vrm Maximum Current Limit (mA)"),
        OPT_U32('l', "vrmsocmax-current", &vrmsocmax_current, "Vrm SoC Maximum Current Limit (mA)"),
        OPT_U32('m', "psi0-current", &psi0_current, "PSI0 Current Limit (mA)"),
        OPT_U32('n', "psi0soc-current", &psi0soc_current, "PSI0 SoC Current Limit (mA)"),
        OPT_END(),
    };


    struct argparse argparse;
    argparse_init(&argparse, options, usage, 0);
    argparse_describe(&argparse, "\n Ryzen Power Management adjust tool.", "\nWARNING: Use at your own risk!\nBy Jiaxun Yang <jiaxun.yang@flygoat.com>, Under LGPL.\nVersion: v0." STRINGIFY(RYZENADJ_VER));
    argc = argparse_parse(&argparse, argc, argv);

    ry = init_ryzenadj();
    if(!ry){
        printf("Unable to init ryzenadj, check permission\n");
        return -1;
    }

    _do_adjust(stapm_limit);
    _do_adjust(fast_limit);
    _do_adjust(slow_limit);
    _do_adjust(slow_time);
    _do_adjust(stapm_time);
    _do_adjust(tctl_temp);
    _do_adjust(vrm_current);
    _do_adjust(vrmsoc_current);
    _do_adjust(vrmmax_current);
    _do_adjust(vrmsocmax_current);
    _do_adjust(psi0_current);
    _do_adjust(psi0soc_current);

    cleanup_ryzenadj(ry);

    return err;
}
