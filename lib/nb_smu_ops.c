// SPDX-License-Identifier: LGPL
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* Ryzen NB SMU Service Request Operations */
#include <stdlib.h>

#include "ryzenadj.h"

#define MP1_C2PMSG_MESSAGE_ADDR_1        0x3B10528
#define MP1_C2PMSG_RESPONSE_ADDR_1       0x3B10564
#define MP1_C2PMSG_ARG_BASE_1            0x3B10998

/* For Vangogh and Rembrandt */
#define MP1_C2PMSG_MESSAGE_ADDR_2        0x3B10528
#define MP1_C2PMSG_RESPONSE_ADDR_2       0x3B10578
#define MP1_C2PMSG_ARG_BASE_2            0x3B10998

/* For Strix Point */
#define MP1_C2PMSG_MESSAGE_ADDR_3	0x3b10928
#define MP1_C2PMSG_RESPONSE_ADDR_3	0x3b10978
#define MP1_C2PMSG_ARG_BASE_3		0x3b10998

#define PSMU_C2PMSG_MESSAGE_ADDR_1          0x3B10a20
#define PSMU_C2PMSG_RESPONSE_ADDR_1         0x3B10a80
#define PSMU_C2PMSG_ARG_BASE_1              0x3B10a88

/* For Dragon and Fire Range */
#define MP1_C2PMSG_MESSAGE_ADDR_4	0x3B10530
#define MP1_C2PMSG_RESPONSE_ADDR_4	0x3B1057C
#define MP1_C2PMSG_ARG_BASE_4		0x3B109C4

#define PSMU_C2PMSG_MESSAGE_ADDR_2          0x03B10524
#define PSMU_C2PMSG_RESPONSE_ADDR_2         0x03B10570
#define PSMU_C2PMSG_ARG_BASE_2              0x03B10A40

/*
* All the SMU have the same TestMessage as for now
* Correct me if they don't
*/
#define SMU_TEST_MSG 0x1

static uint32_t c2pmsg_argX_addr(const uint32_t base, const uint32_t offt) {
	return base + 4 * offt;
}

uint32_t smu_service_req(smu_t smu, const uint32_t id, smu_service_args_t *args) {
	uint32_t response = 0x0;
	DBG("SMU_SERVICE REQ_ID:0x%x\n", id);
	DBG("SMU_SERVICE REQ: arg0: 0x%x, arg1:0x%x, arg2:0x%x, arg3:0x%x, arg4: 0x%x, arg5: 0x%x\n",  \
		args->arg0, args->arg1, args->arg2, args->arg3, args->arg4, args->arg5);

	/* Clear the response */
	smn_reg_write(smu->os_access, smu->rep, 0x0);
	/* Pass arguments */
	smn_reg_write(smu->os_access, c2pmsg_argX_addr(smu->arg_base, 0), args->arg0);
	smn_reg_write(smu->os_access, c2pmsg_argX_addr(smu->arg_base, 1), args->arg1);
	smn_reg_write(smu->os_access, c2pmsg_argX_addr(smu->arg_base, 2), args->arg2);
	smn_reg_write(smu->os_access, c2pmsg_argX_addr(smu->arg_base, 3), args->arg3);
	smn_reg_write(smu->os_access, c2pmsg_argX_addr(smu->arg_base, 4), args->arg4);
	smn_reg_write(smu->os_access, c2pmsg_argX_addr(smu->arg_base, 5), args->arg5);
	/* Send message ID */
	smn_reg_write(smu->os_access, smu->msg, id);
	/* Wait until response changed */
	while(response == 0x0) {
		response = smn_reg_read(smu->os_access, smu->rep);
	}
	/* Read back arguments */
	args->arg0 = smn_reg_read(smu->os_access, c2pmsg_argX_addr(smu->arg_base, 0));
	args->arg1 = smn_reg_read(smu->os_access, c2pmsg_argX_addr(smu->arg_base, 1));
	args->arg2 = smn_reg_read(smu->os_access, c2pmsg_argX_addr(smu->arg_base, 2));
	args->arg3 = smn_reg_read(smu->os_access, c2pmsg_argX_addr(smu->arg_base, 3));
	args->arg4 = smn_reg_read(smu->os_access, c2pmsg_argX_addr(smu->arg_base, 4));
	args->arg5 = smn_reg_read(smu->os_access, c2pmsg_argX_addr(smu->arg_base, 5));

	DBG("SMU_SERVICE REP: REP: 0x%x, arg0: 0x%x, arg1:0x%x, arg2:0x%x, arg3:0x%x, arg4: 0x%x, arg5: 0x%x\n",  \
		response, args->arg0, args->arg1, args->arg2, args->arg3, args->arg4, args->arg5);

	return response;
}

static int smu_service_test(smu_t smu)
{
	uint32_t response = 0x0;

	/* Clear the response */
	smn_reg_write(smu->os_access, smu->rep, 0x0);
	/* Test message with unique argument */
	smn_reg_write(smu->os_access, smu->arg_base, 0x47);
	if(smn_reg_read(smu->os_access, smu->arg_base) != 0x47){
		printf("PCI Bus is not writeable, check secure boot\n");
		return 0;
	}

	/* Send message ID */
	smn_reg_write(smu->os_access, smu->msg, SMU_TEST_MSG);
	/* Wait until response changed */
	while(response == 0x0) {
		response = smn_reg_read(smu->os_access, smu->rep);
	}

	return response == REP_MSG_OK;
}

smu_t get_smu(os_access_obj_t *obj, const int smu_type) {
	smu_t smu = malloc(sizeof(*smu));

	if (smu == NULL)
		return NULL;

	smu->os_access = obj;

	/* Fill SMU information */
	switch(smu_type){
		case TYPE_MP1:
			switch (cpuid_get_family()) {
			case FAM_REMBRANDT:
			case FAM_VANGOGH:
			case FAM_MENDOCINO:
			case FAM_PHOENIX:
			case FAM_HAWKPOINT:
				smu->msg = MP1_C2PMSG_MESSAGE_ADDR_2;
				smu->rep = MP1_C2PMSG_RESPONSE_ADDR_2;
				smu->arg_base = MP1_C2PMSG_ARG_BASE_2;
				break;
			case FAM_KRACKANPOINT:
			case FAM_STRIXPOINT:
			case FAM_STRIXHALO:
				smu->msg = MP1_C2PMSG_MESSAGE_ADDR_3;
				smu->rep = MP1_C2PMSG_RESPONSE_ADDR_3;
				smu->arg_base = MP1_C2PMSG_ARG_BASE_3;
				break;
			case FAM_DRAGONRANGE:
			case FAM_FIRERANGE:
				smu->msg = MP1_C2PMSG_MESSAGE_ADDR_4;
				smu->rep = MP1_C2PMSG_RESPONSE_ADDR_4;
				smu->arg_base = MP1_C2PMSG_ARG_BASE_4;
				break;
			default:
				smu->msg = MP1_C2PMSG_MESSAGE_ADDR_1;
				smu->rep = MP1_C2PMSG_RESPONSE_ADDR_1;
				smu->arg_base = MP1_C2PMSG_ARG_BASE_1;
				break;
			}
			break;
		case TYPE_PSMU:
			switch (cpuid_get_family()) {
			case FAM_DRAGONRANGE:
			case FAM_FIRERANGE:
				smu->msg = PSMU_C2PMSG_MESSAGE_ADDR_2;
				smu->rep = PSMU_C2PMSG_RESPONSE_ADDR_2;
				smu->arg_base = PSMU_C2PMSG_ARG_BASE_2;
				break;
			default:
				smu->msg = PSMU_C2PMSG_MESSAGE_ADDR_1;
				smu->rep = PSMU_C2PMSG_RESPONSE_ADDR_1;
				smu->arg_base = PSMU_C2PMSG_ARG_BASE_1;
				break;
			}
			break;
		default:
			DBG("Failed to get SMU, unknown SMU_TYPE: %i\n", smu_type);
			goto err;
			break;
	}

	if(smu_service_test(smu)){
		return smu;
	} else {
		DBG("Failed to get SMU, SMU_TYPE: %i\n", smu_type);
		goto err;
	}
err:
	free(smu);
	return NULL;
}
