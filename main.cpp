// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* Ryzen NB SMU Service Request Tool */

#include <string.h>
#include "nb_smu_ops.h"
#include "argparse.h"
#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)

#define VER_STR "v0.0.2"

#define _do_adjust(ARG, OPT) \
do{ \
    while(ARG != 0){    \
        memset(args, 0, sizeof(*args));\
        args->arg0 = ARG; \
        if(smu_service_req(nb, OPT, args) == 0x1){   \
            printf("Sucessfully Set " STRINGIFY(ARG) " to %x\n", args->arg0);    \
            break;  \
        } else {    \
            printf("Faild to set" STRINGIFY(ARG) " \n");   \
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
    pci_obj_t pci_obj;
    nb_t nb;
    int info = 0;
    smu_service_args_t *args;
    int err = 0;

    uint32_t stapm_limit = 0, fast_limit = 0, slow_limit = 0, slow_time = 0, stapm_time = 0, tctl_temp = 0;
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
    argparse_describe(&argparse, "\n Ryzen Power Management adjust tool.", "\nWARNING: Use at your own risk!\nBy Jiaxun Yang <jiaxun.yang@flygoat.com>, Under LGPL.\nVersion: " VER_STR);
    argc = argparse_parse(&argparse, argc, argv);

    pci_obj = init_pci_obj();
    if(!pci_obj){
        printf("Unable to get PCI Obj\n");
        return -1;
    }

    nb = get_nb(pci_obj);
    if(!nb){
        printf("Unable to get NB Obj\n");
        err = -1;
        goto out_free_pci_obj;
    }

    args = (smu_service_args_t *)malloc(sizeof(*args));
    memset(args, 0, sizeof(*args));

    smu_service_req(nb, 0x3, args);
    if(args->arg0 != 0x5 && args->arg0 != 0x5){
        printf("Not a Ryzen NB SMU, BIOS Interface Ver: 0x%x",args->arg0);
        err = -1;
        goto out_err;
    }
    memset(args, 0, sizeof(*args));

    _do_adjust(stapm_limit, 0x1a);
    _do_adjust(fast_limit, 0x1b);
    _do_adjust(slow_limit, 0x1c);
    _do_adjust(slow_time, 0x1d);
    _do_adjust(stapm_time, 0x1e);
    _do_adjust(tctl_temp, 0x1f);
    _do_adjust(vrm_current, 0x20);
    _do_adjust(vrmsoc_current, 0x21);
    _do_adjust(vrmmax_current, 0x22);
    _do_adjust(vrmsocmax_current, 0x23);
    _do_adjust(psi0_current, 0x24);
    _do_adjust(psi0soc_current, 0x25);

out_err:
    free(args);
out_free_pci_obj:   
    free_pci_obj(pci_obj);
    return err;
}
