// SPDX-License-Identifier: LGPL
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* RyzenAdj API */

#include "ryzenadj.h"

ryzen_access init_ryzenadj(){
    smu_service_args_t args = {0, 0, 0, 0, 0, 0};
    ryzen_access ry;

    ry = (ryzen_access)malloc((sizeof(*ry)));

    ry->pci_obj = init_pci_obj();
    if(!ry->pci_obj){
        printf("Unable to get PCI Obj\n");
        return NULL;
    }

    ry->nb = get_nb(ry->pci_obj);
    if(!ry->nb){
        printf("Unable to get NB Obj\n");
        goto out_free_pci_obj;
    }

    ry->mp1_smu = get_smu(ry->nb, TYPE_MP1);
    if(!ry->mp1_smu){
        goto out_free_nb;
    }

    ry->psmu = get_smu(ry->nb, TYPE_PSMU);
    if(!ry->psmu){
        goto out_free_mp1_smu;
    }

    smu_service_req(ry->mp1_smu, 0x3, &args);
    if(args.arg0 != 0x5 && args.arg0 != 0x6){
        printf("Not a Ryzen NB SMU, BIOS Interface Ver: 0x%x",args.arg0);
        goto out_err;
    }

    return ry;

out_err:
    free_smu(ry->psmu);
out_free_mp1_smu:
    free_smu(ry->mp1_smu);
out_free_nb:
    free_nb(ry->nb);
out_free_pci_obj:   
    free_pci_obj(ry->pci_obj);
    free(ry);
    return NULL;
}

void cleanup_ryzenadj(ryzen_access ry){
    free_smu(ry->psmu);
    free_smu(ry->mp1_smu);
    free_nb(ry->nb);
    free_pci_obj(ry->pci_obj);
    free(ry);
}

#define _do_adjust(OPT) \
do{ \
        smu_service_args_t args = {0, 0, 0, 0, 0, 0};    \
        args.arg0 = value; \
        if(smu_service_req(ry->mp1_smu, OPT, &args) == 0x1){   \
            return 0;   \
        } else {    \
            return -1;  \
        } \
}while(0);

int set_stapm_limit(ryzen_access ry, uint32_t value){
    _do_adjust(0x1a);
}
int set_fast_limit(ryzen_access ry, uint32_t value){
    _do_adjust(0x1b);
}
int set_slow_limit(ryzen_access ry, uint32_t value){
    _do_adjust(0x1c);
}
int set_slow_time(ryzen_access ry, uint32_t value){
    _do_adjust(0x1d);
}
int set_stapm_time(ryzen_access ry, uint32_t value){
    _do_adjust(0x1e);
}
int set_tctl_temp(ryzen_access ry, uint32_t value){
    _do_adjust(0x1f);
}
int set_vrm_current(ryzen_access ry, uint32_t value){
    _do_adjust(0x20);
}
int set_vrmsoc_current(ryzen_access ry, uint32_t value){
    _do_adjust(0x21);
}
int set_vrmmax_current(ryzen_access ry, uint32_t value){
    _do_adjust(0x22);
}
int set_vrmsocmax_current(ryzen_access ry, uint32_t value){
    _do_adjust(0x23);
}
int set_psi0_current(ryzen_access ry, uint32_t value){
    _do_adjust(0x24);
}
int set_psi0soc_current(ryzen_access ry, uint32_t value){
    _do_adjust(0x25);
}