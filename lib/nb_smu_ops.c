// SPDX-License-Identifier: LGPL
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* Ryzen NB SMU Service Request Opreations */
#include <stdlib.h>

#include "ryzenadj.h"

static uint32_t c2pmsg_argX_addr(const uint32_t base, const uint32_t offt) {
	return (base + 4 * offt);
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
	/* Wait until reponse changed */
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

static int smu_service_test(smu_t smu) {
	uint32_t response = 0x0;

	/* Clear the response */
	smn_reg_write(smu->os_access, smu->rep, 0x0);
	/* Test message with unique argument */
	smn_reg_write(smu->os_access, smu->arg_base, 0x47);
	if(smn_reg_read(smu->os_access, smu->arg_base) != 0x47){
		printf("PCI Bus is not writeable, check secure boot\n");
		return 0;
	}

	/* Send message ID (all the SMU have the same TestMessage as of now, correct me if they don't) */
	smn_reg_write(smu->os_access, smu->msg, 0x1);

	/* Wait until reponse changed */
	while(response == 0x0)
		response = smn_reg_read(smu->os_access, smu->rep);

	return response == REP_MSG_OK;
}

static uint32_t getMP1C2PMsgMessageAddress(const RYZEN_FAMILY family) {
	switch (family) {
		case FAM_KRACKANPOINT:
		case FAM_STRIXPOINT:
		case FAM_STRIXHALO:
			return 0x3B10928;
		case FAM_DRAGONRANGE:
		case FAM_FIRERANGE:
			return 0x3B10530;
		default:
			break;
	}

	return 0x3B10528;
}

static uint32_t getMP1C2PMsgResponseAddress(const RYZEN_FAMILY family) {
	switch (family) {
		case FAM_REMBRANDT:
		case FAM_VANGOGH:
		case FAM_MENDOCINO:
		case FAM_PHOENIX:
		case FAM_HAWKPOINT:
			return 0x3B10578;
		case FAM_KRACKANPOINT:
		case FAM_STRIXPOINT:
		case FAM_STRIXHALO:
			return 0x3B10978;
		case FAM_DRAGONRANGE:
		case FAM_FIRERANGE:
			return 0x3B1057C;
		default:
			break;
	}

	return 0x3B10564;
}

static uint32_t getMP1C2PMsgArgBaseAddress(const RYZEN_FAMILY family) {
	switch (family) {
		case FAM_DRAGONRANGE:
		case FAM_FIRERANGE:
			return 0x3B109C4;
		default:
			break;
	}

	return 0x3B10998;
}

static uint32_t getPSMUC2PMsgMessageAddress(const RYZEN_FAMILY family) {
	switch (family) {
		case FAM_DRAGONRANGE:
		case FAM_FIRERANGE:
			return 0x03B10524;
		default:
			break;
	}

	return 0x3B10A20;
}

static uint32_t getPSMUC2PMsgResponseAddress(const RYZEN_FAMILY family) {
	switch (family) {
		case FAM_DRAGONRANGE:
		case FAM_FIRERANGE:
			return 0x03B10570;
		default:
			break;
	}

	return 0x3B10A80;
}

static uint32_t getPSMUC2PMsgArgBaseAddress(const RYZEN_FAMILY family) {
	switch (family) {
		case FAM_DRAGONRANGE:
		case FAM_FIRERANGE:
			return 0x03B10A40;
		default:
			break;
	}

	return 0x3B10A88;
}

smu_t get_smu(os_access_obj_t *obj, const int smu_type) {
	const RYZEN_FAMILY family = cpuid_get_family();
	smu_t smu = malloc(sizeof(*smu));

	if (smu == NULL)
		return NULL;

	smu->os_access = obj;

	/* Fill SMU information */
	switch (smu_type) {
		case TYPE_MP1: {
			smu->msg = getMP1C2PMsgMessageAddress(family);
			smu->rep = getMP1C2PMsgResponseAddress(family);
			smu->arg_base = getMP1C2PMsgArgBaseAddress(family);
		}
			break;
		case TYPE_PSMU: {
			smu->msg = getPSMUC2PMsgMessageAddress(family);
			smu->rep = getPSMUC2PMsgResponseAddress(family);
			smu->arg_base = getPSMUC2PMsgArgBaseAddress(family);
		}
			break;
		default: {
			DBG("Failed to get SMU, unknown SMU_TYPE: %i\n", smu_type);
			goto err;
		}
	}

	if (smu_service_test(smu))
		return smu;

	DBG("Faild to get SMU, SMU_TYPE: %i\n", smu_type);

err:
	free(smu);
	return NULL;
}
