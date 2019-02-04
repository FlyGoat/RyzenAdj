// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* Ryzen NB SMU Service Request Opreations */

#include "nb_smu_ops.h"

u32 smu_service_req(nb_t nb ,u32 id ,smu_service_args_t *args)
{
    u32 response = 0x0;
    DBG("SMU_SERVICE REQ_ID:0x%x\n", id);
    DBG("SMU_SERVICE REQ: arg0: 0x%x, arg1:0x%x, arg2:0x%x, arg3:0x%x, arg4: 0x%x, arg5: 0x%x\n",  \
        args->arg0, args->arg1, args->arg2, args->arg3, args->arg4, args->arg5);

    /* Clear the response */
    nb_reg_write(nb, MP1_C2PMSG_RESPONSE_ADDR, 0x0);
    /* Pass arguments */
    nb_reg_write(nb, MP1_C2PMSG_ARGx_ADDR(0), args->arg0);
    nb_reg_write(nb, MP1_C2PMSG_ARGx_ADDR(1), args->arg1);
    nb_reg_write(nb, MP1_C2PMSG_ARGx_ADDR(2), args->arg2);
    nb_reg_write(nb, MP1_C2PMSG_ARGx_ADDR(3), args->arg3);
    nb_reg_write(nb, MP1_C2PMSG_ARGx_ADDR(4), args->arg4);
    nb_reg_write(nb, MP1_C2PMSG_ARGx_ADDR(5), args->arg5);
    /* Send message ID */
    nb_reg_write(nb, MP1_C2PMSG_MESSAGE_ADDR, id);
    /* Wait until reponse changed */
    while(response == 0x0) {
        response = nb_reg_read(nb, MP1_C2PMSG_RESPONSE_ADDR);
    }
    /* Read back arguments */
    args->arg0 = nb_reg_read(nb, MP1_C2PMSG_ARGx_ADDR(0));
    args->arg1 = nb_reg_read(nb, MP1_C2PMSG_ARGx_ADDR(1));
    args->arg2 = nb_reg_read(nb, MP1_C2PMSG_ARGx_ADDR(2));
    args->arg3 = nb_reg_read(nb, MP1_C2PMSG_ARGx_ADDR(3));
    args->arg4 = nb_reg_read(nb, MP1_C2PMSG_ARGx_ADDR(4));
    args->arg5 = nb_reg_read(nb, MP1_C2PMSG_ARGx_ADDR(5));

    DBG("SMU_SERVICE REP: REP: 0x%x, arg0: 0x%x, arg1:0x%x, arg2:0x%x, arg3:0x%x, arg4: 0x%x, arg5: 0x%x\n",  \
        response, args->arg0, args->arg1, args->arg2, args->arg3, args->arg4, args->arg5);

    return response;
}