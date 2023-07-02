// SPDX-License-Identifier: LGPL
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* RyzenAdj API */

#include "ryzenadj.h"
#include "math.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#endif


EXP ryzen_access CALL init_ryzenadj()
{
	ryzen_access ry;
	enum ryzen_family family = cpuid_get_family();
	if (family == FAM_UNKNOWN)
		return NULL;

	ry = (ryzen_access)malloc((sizeof(*ry)));

	if (!ry){
		printf("Out of memory\n");
		return NULL;
	}
	memset(ry, 0, sizeof(*ry));

	ry->family = family;
	//init version and power metric table only on demand to avoid unnecessary SMU writes
	ry->bios_if_ver = 0;
	ry->table_values = NULL;

	ry->pci_obj = init_pci_obj();
	if(!ry->pci_obj){
		printf("Unable to get PCI Obj, check permission\n");
		return NULL;
	}

	ry->nb = get_nb(ry->pci_obj);
	if(!ry->nb){
		printf("Unable to get NB Obj\n");
		goto out_free_pci_obj;
	}

	ry->mp1_smu = get_smu(ry->nb, TYPE_MP1);
	if(!ry->mp1_smu){
		printf("Unable to get MP1 SMU Obj\n");
		goto out_free_nb;
	}

	ry->psmu = get_smu(ry->nb, TYPE_PSMU);
	if(!ry->psmu){
		printf("Unable to get RSMU Obj\n");
		goto out_free_mp1_smu;
	}

	return ry;

out_free_mp1_smu:
	free_smu(ry->mp1_smu);
out_free_nb:
	free_nb(ry->nb);
out_free_pci_obj:
	free_pci_obj(ry->pci_obj);
	free(ry);
	return NULL;
}

EXP void CALL cleanup_ryzenadj(ryzen_access ry){
	if (ry == NULL)
	    return;

	if (ry->table_values){
		free(ry->table_values);
	}
	if (ry->mem_obj){
		free_mem_obj(ry->mem_obj);
	}
	free_smu(ry->psmu);
	free_smu(ry->mp1_smu);
	free_nb(ry->nb);
	free_pci_obj(ry->pci_obj);
	free(ry);
}

EXP enum ryzen_family get_cpu_family(ryzen_access ry)
{
	return ry->family;
}

EXP int get_bios_if_ver(ryzen_access ry)
{
	if(ry->bios_if_ver)
		return ry->bios_if_ver;

	smu_service_args_t args = {0, 0, 0, 0, 0, 0};
	smu_service_req(ry->mp1_smu, 0x3, &args);
	ry->bios_if_ver = args.arg0;
	return ry->bios_if_ver;
}

#define _return_translated_smu_error(SMU_RESP)                              \
do {                                                                        \
	if (SMU_RESP == REP_MSG_UnknownCmd) {                                   \
		printf("%s is unsupported\n", __func__);                            \
		return ADJ_ERR_SMU_UNSUPPORTED;                                     \
	} else if (SMU_RESP == REP_MSG_CmdRejectedPrereq){                      \
		printf("%s was rejected\n", __func__);                              \
		return ADJ_ERR_SMU_REJECTED;                                        \
	} else if (SMU_RESP == REP_MSG_CmdRejectedBusy) {                       \
		printf("%s was rejected - busy\n", __func__);                       \
		return ADJ_ERR_SMU_REJECTED;                                        \
	} else if (SMU_RESP == REP_MSG_Failed) {                                \
		printf("%s failed\n", __func__);                                    \
		return ADJ_ERR_SMU_REJECTED;                                        \
	} else {                                                                \
		printf("%s failed with unknown response %x\n", __func__, SMU_RESP); \
		return ADJ_ERR_SMU_REJECTED;                                        \
	}                                                                       \
} while (0);

int request_table_ver_and_size(ryzen_access ry)
{
	unsigned int get_table_ver_msg;
	int resp;
	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		get_table_ver_msg = 0xC;
		break;
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
	case FAM_REMBRANDT:
	case FAM_PHOENIX:
		get_table_ver_msg = 0x6;
		break;
	default:
		printf("request_table_ver_and_size is not supported on this family\n");
		return ADJ_ERR_FAM_UNSUPPORTED;
	}

	smu_service_args_t args = {0, 0, 0, 0, 0, 0};
	resp = smu_service_req(ry->psmu, get_table_ver_msg, &args);
	ry->table_ver = args.arg0;

	switch (ry->table_ver)
	{
	case 0x1E0001: ry->table_size = 0x568; break;
	case 0x1E0002: ry->table_size = 0x580; break;
	case 0x1E0003: ry->table_size = 0x578; break;
	case 0x1E0004: ry->table_size = 0x608; break;
	case 0x1E0005: ry->table_size = 0x608; break;
	case 0x1E000A: ry->table_size = 0x608; break;
	case 0x1E0101: ry->table_size = 0x608; break;
	case 0x370000: ry->table_size = 0x794; break;
	case 0x370001: ry->table_size = 0x884; break;
	case 0x370002: ry->table_size = 0x88C; break;
	case 0x370003: ry->table_size = 0x8AC; break;
	case 0x370004: ry->table_size = 0x8AC; break;
	case 0x370005: ry->table_size = 0x8C8; break;
	case 0x3F0000: ry->table_size = 0x7AC; break;
	case 0x400001: ry->table_size = 0x910; break;
	case 0x400002: ry->table_size = 0x928; break;
	case 0x400003: ry->table_size = 0x94C; break;
	case 0x400004: ry->table_size = 0x944; break;
	case 0x400005: ry->table_size = 0x944; break;
	case 0x450004: ry->table_size = 0xA44; break;
	case 0x450005: ry->table_size = 0xA44; break;
	case 0x4C0006: ry->table_size = 0xAA0; break;
		default:
			//use a larger size then the largest known table to be able to test real table size of unknown tables
			ry->table_size = 0xA00;
	}

	if (resp != REP_MSG_OK) {
		_return_translated_smu_error(resp);
	}
	if(!ry->table_ver){
		printf("request_table_ver_and_size did not return anything\n");
		return ADJ_ERR_SMU_UNSUPPORTED;
	}
	return 0;
}

int request_table_addr(ryzen_access ry)
{
	unsigned int get_table_addr_msg;
	int resp;
	smu_service_args_t args = {0, 0, 0, 0, 0, 0};
	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		args.arg0 = 3;
		get_table_addr_msg = 0xB;
		break;
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
	case FAM_REMBRANDT:
	case FAM_PHOENIX:
		get_table_addr_msg = 0x66;
		break;
	default:
		printf("request_table_addr is not supported on this family\n");
		return ADJ_ERR_FAM_UNSUPPORTED;
	}

	resp = smu_service_req(ry->psmu, get_table_addr_msg, &args);

	switch (ry->family)
	{
	case FAM_REMBRANDT:
	case FAM_PHOENIX:
		ry->table_addr = (uint64_t) args.arg1 << 32 | args.arg0;
		break;
	default:
		ry->table_addr = args.arg0;
	}

	if(resp != REP_MSG_OK){
		_return_translated_smu_error(resp);
	}
	if(!ry->table_addr){
		printf("request_table_addr did not return anything\n");
		return ADJ_ERR_SMU_UNSUPPORTED;
	}
	return 0;
}

int request_transfer_table(ryzen_access ry)
{
	int resp;
	unsigned int transfer_table_msg;
	smu_service_args_t args = {0, 0, 0, 0, 0, 0};
	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		args.arg0 = 3;
		transfer_table_msg = 0x3D;
		break;
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
	case FAM_REMBRANDT:
	case FAM_PHOENIX:
		transfer_table_msg = 0x65;
		break;
	default:
		printf("request_transfer_table is not supported on this family\n");
		return ADJ_ERR_FAM_UNSUPPORTED;
	}

	resp = smu_service_req(ry->psmu, transfer_table_msg, &args);
	if (resp == REP_MSG_CmdRejectedPrereq) {
		//2nd try is needed for 2 usecase: if SMU got interrupted or first call after boot on Zen2
		//we need to wait because if we don't wait 2nd call will fail, too: similar to Raven and Picasso issue but with real reject instead of 0 data response
		//but because we don't have to check any physical memory values, don't waste CPU cycles and use sleep instead
		Sleep(10);
		resp = smu_service_req(ry->psmu, transfer_table_msg, &args);
		if(resp == REP_MSG_CmdRejectedPrereq){
			printf("request_transfer_table was rejected twice\n");
			Sleep(100);
			resp = smu_service_req(ry->psmu, transfer_table_msg, &args);
		}
	}
	if(resp != REP_MSG_OK){
		_return_translated_smu_error(resp);
	}
	return 0;
}

EXP int CALL init_table(ryzen_access ry)
{
	DBG("init_table\n");
	int errorcode = 0;

	errorcode = request_table_ver_and_size(ry);
	if(errorcode){
		return errorcode;
	}

	errorcode = request_table_addr(ry);
	if(errorcode){
		return errorcode;
	}

	//init memory object because it is prerequiremt to woring with physical memory address
	ry->mem_obj = init_mem_obj(ry->table_addr);
	if(!ry->mem_obj)
	{
		printf("Unable to get memory access\n");
		return ADJ_ERR_MEMORY_ACCESS;
	}

	//hold copy of table value in memory for our single value getters
	ry->table_values = calloc(ry->table_size / 4, 4);

	errorcode = refresh_table(ry);
	if(errorcode)
	{
		return errorcode;
	}

	if(!ry->table_values[0]){
		//Raven and Picasso don't get table refresh on the very first transfer call after boot, but respond with OK
		//if we detact 0 data, do an initial 2nd call after a small delay
		//transfer, transfer, wait, wait longer; don't work
		//transfer, wait, wait longer; don't work
		//transfer, wait, transfer; does work
		DBG("empty table detected, try again\n");
		Sleep(10);
		return refresh_table(ry);
	}

	return 0;
}

#define _lazy_init_table(RETURN_VAR)                                 \
do {                                                                 \
	if(!ry->table_values) {                                          \
		DBG("warning: %s was called before init_table\n", __func__); \
		errorcode = init_table(ry);                                  \
		if(errorcode) return RETURN_VAR;                             \
	}                                                                \
} while (0);

EXP uint32_t CALL get_table_ver(ryzen_access ry)
{
	int errorcode;
	_lazy_init_table(0);

	return ry->table_ver;
}

EXP size_t CALL get_table_size(ryzen_access ry)
{
	int errorcode;
	_lazy_init_table(0);

	return ry->table_size;
}

EXP float* CALL get_table_values(ryzen_access ry)
{
	return ry->table_values;
}

EXP int CALL refresh_table(ryzen_access ry)
{
	int errorcode = 0;
	_lazy_init_table(errorcode);

	//only execute request table if we don't use SMU driver
	if(!is_using_smu_driver()){
		//if other tools call tables transfer, we may already find new data inside the memory and can avoid calling transfer table twice
		//avoiding transfer table twice is important because SMU tend to reject transfer table calls if you repeat them too fast
		//transfer table rejection happens even if we did correctly wait for response register change
		//if multiple tools retry transfer table in a loop, both will get rejections, avoid this issue by checking if we need to transfer table
		//refresh table if this is the first call (table is empty) or if the first 6 table values in memory doesn't have new values (compare result = 0)
		if(ry->table_values[0] == 0 || compare_pm_table(ry->table_values, 6 * 4) == 0){
			errorcode = request_transfer_table(ry);
		}
	}

	if(errorcode){
		return errorcode;
	}

	if(copy_pm_table(ry->table_values, ry->table_size)){
		printf("refresh_table failed\n");
		return ADJ_ERR_MEMORY_ACCESS;
	}

	return 0;
}

#define _do_adjust(OPT) \
do {                                                 \
	smu_service_args_t args = {0, 0, 0, 0, 0, 0};    \
	int resp;										 \
	args.arg0 = value;                               \
	resp = smu_service_req(ry->mp1_smu, OPT, &args); \
	if (resp == REP_MSG_OK) {                        \
		err = 0;                                     \
	} else if (resp == REP_MSG_UnknownCmd) {         \
		err = ADJ_ERR_SMU_UNSUPPORTED;               \
	} else {                                         \
		err = ADJ_ERR_SMU_REJECTED;                  \
	}                                                \
} while (0);


#define _do_adjust_psmu(OPT) \
do {                                                 \
	smu_service_args_t args = {0, 0, 0, 0, 0, 0};    \
	int resp;										 \
	args.arg0 = value;                               \
	resp = smu_service_req(ry->psmu, OPT, &args);    \
	if (resp == REP_MSG_OK) {                        \
		err = 0;                                     \
	} else if (resp == REP_MSG_UnknownCmd) {         \
		err = ADJ_ERR_SMU_UNSUPPORTED;               \
	} else {                                         \
		err = ADJ_ERR_SMU_REJECTED;                  \
	}                                                \
} while (0);

#define _read_float_value(OFFSET)                   \
do {                                                \
	if(!ry->table_values)                           \
		return NAN;                                 \
	return ry->table_values[OFFSET / 4];            \
} while (0);

EXP int CALL set_stapm_limit(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x1a);
		break;
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
	case FAM_VANGOGH:
	case FAM_REMBRANDT:
	case FAM_MENDOCINO:
	case FAM_PHOENIX:
		_do_adjust(0x14);
        if (err) {
            printf("%s: Retry with PSMU\n", __func__);
		    _do_adjust_psmu(0x31);
        }
	}
	return err;
}

EXP int CALL set_fast_limit(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x1b);
		break;
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
	case FAM_VANGOGH:
	case FAM_REMBRANDT:
	case FAM_MENDOCINO:
	case FAM_PHOENIX:
		_do_adjust(0x15);
	}
	return err;
}

EXP int CALL set_slow_limit(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x1c);
		break;
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
	case FAM_VANGOGH:
	case FAM_REMBRANDT:
	case FAM_MENDOCINO:
	case FAM_PHOENIX:
		_do_adjust(0x16);
	}
	return err;
}

EXP int CALL set_slow_time(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x1d);
		break;
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
	case FAM_VANGOGH:
	case FAM_REMBRANDT:
	case FAM_MENDOCINO:
	case FAM_PHOENIX:
		_do_adjust(0x17);
	}
	return err;
}

EXP int CALL set_stapm_time(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x1e);
		break;
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
	case FAM_VANGOGH:
	case FAM_REMBRANDT:
	case FAM_MENDOCINO:
	case FAM_PHOENIX:
		_do_adjust(0x18);
	}
	return err;
}

EXP int CALL set_tctl_temp(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x1f);
		break;
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
	case FAM_VANGOGH:
	case FAM_REMBRANDT:
	case FAM_MENDOCINO:
	case FAM_PHOENIX:
		_do_adjust(0x19);
	}
	return err;
}

EXP int CALL set_vrm_current(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x20);
		break;
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
	case FAM_VANGOGH:
	case FAM_REMBRANDT:
	case FAM_MENDOCINO:
	case FAM_PHOENIX:
		_do_adjust(0x1a);
	}
	return err;
}

EXP int CALL set_vrmsoc_current(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x21);
		break;
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
	case FAM_VANGOGH:
	case FAM_REMBRANDT:
	case FAM_MENDOCINO:
	case FAM_PHOENIX:
		_do_adjust(0x1b);
	}
	return err;
}

EXP int CALL set_vrmgfx_current(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_VANGOGH:
		_do_adjust(0x1c);
	}
	return err;
}

EXP int CALL set_vrmcvip_current(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_VANGOGH:
		_do_adjust(0x1d);
	}
	return err;
}

EXP int CALL set_vrmmax_current(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x22);
		break;
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
	case FAM_REMBRANDT:
	case FAM_MENDOCINO:
	case FAM_PHOENIX:
		_do_adjust(0x1c);
		break;
	case FAM_VANGOGH:
		_do_adjust(0x1e);
	}
	return err;
}

EXP int CALL set_vrmgfxmax_current(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_VANGOGH:
		_do_adjust(0x1f);
	}
	return err;
}

EXP int CALL set_vrmsocmax_current(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x23);
		break;
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
	case FAM_REMBRANDT:
	case FAM_MENDOCINO:
	case FAM_PHOENIX:
		_do_adjust(0x1d);
	}
	return err;
}

EXP int CALL set_psi0_current(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x24);
		break;
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
		_do_adjust(0x1e);
	}
	return err;
}

EXP int CALL set_psi3cpu_current(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_VANGOGH:
		_do_adjust(0x20);
	}
	return err;
}

EXP int CALL set_psi0soc_current(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x25);
		break;
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
		_do_adjust(0x1f);
	}
	return err;
}

EXP int CALL set_psi3gfx_current(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_VANGOGH:
		_do_adjust(0x21);
	}
	return err;
}

EXP int CALL set_max_gfxclk_freq(ryzen_access ry, uint32_t value) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x46);
		break;
	}
	return err;
}

EXP int CALL set_min_gfxclk_freq(ryzen_access ry, uint32_t value) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x47);
		break;
	}
	return err;
}

EXP int CALL set_max_socclk_freq(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x48);
		break;
	}
	return err;
}

EXP int CALL set_min_socclk_freq(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x49);
		break;
	}
	return err;
}

EXP int CALL set_max_fclk_freq(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x4A);
		break;
	}
	return err;
}

EXP int CALL set_min_fclk_freq(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x4B);
		break;
	}
	return err;
}

EXP int CALL set_max_vcn(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x4C);
		break;
	}
	return err;
}

EXP int CALL set_min_vcn(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x4D);
		break;
	}
	return err;
}

EXP int CALL set_max_lclk(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x4E);
		break;
	}
	return err;
}

EXP int CALL set_min_lclk(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x4F);
		break;
	}
	return err;
}

EXP int CALL set_prochot_deassertion_ramp(ryzen_access ry, uint32_t value) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x26);
		break;
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
		_do_adjust(0x20);
		break;
	case FAM_VANGOGH:
		_do_adjust(0x22);
		break;
	case FAM_REMBRANDT:
	case FAM_MENDOCINO:
	case FAM_PHOENIX:
		_do_adjust(0x1f);
	}
	return err;
}

EXP int CALL set_apu_skin_temp_limit(ryzen_access ry, uint32_t value) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	value *= 256;
	switch (ry->family)
	{
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
		_do_adjust(0x38);
		break;
	case FAM_VANGOGH:
	case FAM_REMBRANDT:
	case FAM_MENDOCINO:
	case FAM_PHOENIX:
		_do_adjust(0x33);
		break;
	}
	return err;
}

EXP int CALL set_dgpu_skin_temp_limit(ryzen_access ry, uint32_t value) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	value *= 256;
	switch (ry->family)
	{
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
		_do_adjust(0x39);
		break;
	case FAM_VANGOGH:
	case FAM_REMBRANDT:
	case FAM_MENDOCINO:
	case FAM_PHOENIX:
		_do_adjust(0x34);
		break;
	}
	return err;
}

EXP int CALL set_apu_slow_limit(ryzen_access ry, uint32_t value) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
		_do_adjust(0x21);
		break;
	case FAM_REMBRANDT:
	case FAM_PHOENIX:
		_do_adjust(0x23);
		break;
	}
	return err;
}

EXP int CALL set_skin_temp_power_limit(ryzen_access ry, uint32_t value) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
		_do_adjust(0x53);
		break;
	case FAM_VANGOGH:
	case FAM_REMBRANDT:
	case FAM_MENDOCINO:
	case FAM_PHOENIX:
		_do_adjust(0x4a);
		break;
	}
	return err;
}

EXP int CALL set_gfx_clk(ryzen_access ry, uint32_t value) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
	case FAM_VANGOGH:
	case FAM_REMBRANDT:
	case FAM_MENDOCINO:
	case FAM_PHOENIX:
		_do_adjust_psmu(0x89);
		break;
	}
	return err;
}

EXP int CALL set_power_saving(ryzen_access ry) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;
	uint32_t value = 0;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x19);
		break;
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
	case FAM_VANGOGH:
	case FAM_REMBRANDT:
	case FAM_MENDOCINO:
	case FAM_PHOENIX:
		_do_adjust(0x12);
		break;
	}
	return err;
}

EXP int CALL set_max_performance(ryzen_access ry) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;
	uint32_t value = 0;

	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x18);
		break;
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
	case FAM_VANGOGH:
	case FAM_REMBRANDT:
	case FAM_MENDOCINO:
	case FAM_PHOENIX:
		_do_adjust(0x11);
		break;
	}
	return err;
}

EXP int CALL set_oc_clk(ryzen_access ry, uint32_t value) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_LUCIENNE:
	case FAM_RENOIR:
	case FAM_CEZANNE:
	case FAM_REMBRANDT:
		_do_adjust(0x31);
        if (err) {
            printf("%s: Retry with PSMU\n", __func__);
		    _do_adjust_psmu(0x19);
        }
		break;
	}
	return err;
}

EXP int CALL set_per_core_oc_clk(ryzen_access ry, uint32_t value) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_LUCIENNE:
	case FAM_RENOIR:
	case FAM_CEZANNE:
	case FAM_REMBRANDT:
		_do_adjust(0x32);
        if (err) {
            printf("%s: Retry with PSMU\n", __func__);
		    _do_adjust_psmu(0x1a);
        }
		break;
	}
	return err;
}

EXP int CALL set_oc_volt(ryzen_access ry, uint32_t value) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_LUCIENNE:
	case FAM_RENOIR:
	case FAM_CEZANNE:
	case FAM_REMBRANDT:
		_do_adjust(0x33);
        if (err) {
            printf("%s: Retry with PSMU\n", __func__);
		    _do_adjust_psmu(0x1b);
        }
		break;
	}
	return err;
}

EXP int CALL set_disable_oc(ryzen_access ry) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;
	uint32_t value = 0x0;

	switch (ry->family)
	{
	case FAM_LUCIENNE:
	case FAM_RENOIR:
	case FAM_CEZANNE:
		_do_adjust(0x30);
        if (err) {
            printf("%s: Retry with PSMU\n", __func__);
		    _do_adjust_psmu(0x1d);
        }
		break;
	case FAM_REMBRANDT:
		_do_adjust_psmu(0x18);
		break;
	}
	return err;
}

EXP int CALL set_enable_oc(ryzen_access ry) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;
	uint32_t value = 0x0;

	switch (ry->family)
	{
	case FAM_LUCIENNE:
	case FAM_RENOIR:
	case FAM_CEZANNE:
		_do_adjust(0x2F);
		break;
	case FAM_REMBRANDT:
		_do_adjust_psmu(0x17);
		break;
	}
	return err;
}

EXP int CALL set_coall(ryzen_access ry, uint32_t value) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_CEZANNE:
	case FAM_RENOIR:
	case FAM_LUCIENNE:
		_do_adjust(0x55);
		break;
	case FAM_REMBRANDT:
	case FAM_VANGOGH:
		_do_adjust(0x4C);
		break;
	}
	return err;
}

EXP int CALL set_coper(ryzen_access ry, uint32_t value) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_CEZANNE:
	case FAM_RENOIR:
	case FAM_LUCIENNE:
		_do_adjust(0x54);
	case FAM_REMBRANDT:
		_do_adjust(0x4B);
		break;
	}
	return err;
}

EXP int CALL set_cogfx(ryzen_access ry, uint32_t value) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_CEZANNE:
	case FAM_RENOIR:
	case FAM_LUCIENNE:
		_do_adjust(0x64);
		break;
	case FAM_REMBRANDT:
	case FAM_VANGOGH:
		_do_adjust_psmu(0xB7);
		break;
	}
	return err;
}

//PM Table section, offset of first lines are stable across multiple PM Table versions
EXP float CALL get_stapm_limit(ryzen_access ry){_read_float_value(0x0);}
EXP float CALL get_stapm_value(ryzen_access ry){_read_float_value(0x4);}
EXP float CALL get_fast_limit(ryzen_access ry){_read_float_value(0x8);}
EXP float CALL get_fast_value(ryzen_access ry){_read_float_value(0xC);}
EXP float CALL get_slow_limit(ryzen_access ry){_read_float_value(0x10);}
EXP float CALL get_slow_value(ryzen_access ry){_read_float_value(0x14);}

//custom section, offsets are depending on table version
EXP float CALL get_apu_slow_limit(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x003F0000:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
	case 0x004C0006:
		_read_float_value(0x18);
	}
	return NAN;
}
EXP float CALL get_apu_slow_value(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x003F0000:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
	case 0x004C0006:
		_read_float_value(0x1C);
	}
	return NAN;
}
EXP float CALL get_vrm_current(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x001E0001:
	case 0x001E0002:
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		_read_float_value(0x18);
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
	case 0x004C0006:
		_read_float_value(0x20);
	}
	return NAN;
}
EXP float CALL get_vrm_current_value(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x001E0001:
	case 0x001E0002:
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		_read_float_value(0x1C);
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
	case 0x004C0006:
		_read_float_value(0x24);
	}
	return NAN;
}
EXP float CALL get_vrmsoc_current(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x001E0001:
	case 0x001E0002:
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		_read_float_value(0x20);
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
	case 0x004C0006:
		_read_float_value(0x28);
	}
	return NAN;
}
EXP float CALL get_vrmsoc_current_value(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x001E0001:
	case 0x001E0002:
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		_read_float_value(0x24);
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
	case 0x004C0006:
		_read_float_value(0x2C);
	}
	return NAN;
}
EXP float CALL get_vrmmax_current(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x001E0001:
	case 0x001E0002:
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		_read_float_value(0x28);
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
		_read_float_value(0x30);
	}
	return NAN;
}
EXP float CALL get_vrmmax_current_value(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x001E0001:
	case 0x001E0002:
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		_read_float_value(0x2C);
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
		_read_float_value(0x34);
	}
	return NAN;
}
EXP float CALL get_vrmsocmax_current(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x001E0001:
	case 0x001E0002:
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		_read_float_value(0x34);
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
		_read_float_value(0x38);
	}
	return NAN;
}
EXP float CALL get_vrmsocmax_current_value(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x001E0001:
	case 0x001E0002:
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		_read_float_value(0x38);
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
		_read_float_value(0x3C);
	}
	return NAN;
}
EXP float CALL get_tctl_temp(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x001E0001:
	case 0x001E0002:
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		_read_float_value(0x58); //use core1 because core0 is not reported on dual core cpus
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x003F0000:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
	case 0x004C0006:
		_read_float_value(0x40);
	}
	return NAN;
}
EXP float CALL get_tctl_temp_value(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x001E0001:
	case 0x001E0002:
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		_read_float_value(0x5C); //use core1 because core0 is not reported on dual core cpus
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x003F0000:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
	case 0x004C0006:
		_read_float_value(0x44);
	}
	return NAN;
}
EXP float CALL get_apu_skin_temp_limit(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x003F0000:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
	case 0x004C0006:
		_read_float_value(0x58);
	}
	return NAN;
}
EXP float CALL get_apu_skin_temp_value(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x003F0000:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
	case 0x004C0006:
		_read_float_value(0x5C);
	}
	return NAN;
}
EXP float CALL get_dgpu_skin_temp_limit(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
	case 0x004C0006:
		_read_float_value(0x60);
	}
	return NAN;
}
EXP float CALL get_dgpu_skin_temp_value(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
	case 0x004C0006:
		_read_float_value(0x64);
	}
	return NAN;
}

EXP float CALL get_psi0_current(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x001E0001:
	case 0x001E0002:
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		_read_float_value(0x40);
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
		_read_float_value(0x78);
	}
	return NAN;
}

EXP float CALL get_psi0soc_current(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x001E0001:
	case 0x001E0002:
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		_read_float_value(0x48);
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
		_read_float_value(0x80);
	}
	return NAN;
}

EXP float CALL get_cclk_setpoint(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x001E0001:
	case 0x001E0002:
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		_read_float_value(0x98); //use core1 because core0 is not reported on dual core cpus;
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
		_read_float_value(0xFC);
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
		_read_float_value(0x100);
	}
	return NAN;
}

EXP float CALL get_cclk_busy_value(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x001E0001:
	case 0x001E0002:
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		_read_float_value(0x9C); //use core1 because core0 is not reported on dual core cpus
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
		_read_float_value(0x100);
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
		_read_float_value(0x104);
	}
	return NAN;
}

EXP float CALL get_stapm_time(ryzen_access ry)
{
	switch (ry->table_ver)
	{
	case 0x001E0002:
		_read_float_value(0x564);
	case 0x001E0003:
		_read_float_value(0x55C);
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		_read_float_value(0x5E0);
	case 0x00370000:
		_read_float_value(0x768);
	case 0x00370001:
		_read_float_value(0x858);
	case 0x00370002:
		_read_float_value(0x860);
	case 0x00370003:
	case 0x00370004:
		_read_float_value(0x880);
	case 0x00370005:
		_read_float_value(0x89C);
	case 0x00400001:
		_read_float_value(0x8E4);
	case 0x00400002:
		_read_float_value(0x8FC);
	case 0x00400003:
		_read_float_value(0x920);
	case 0x00400004:
	case 0x00400005:
		_read_float_value(0x918);
	}
	return NAN;
}

EXP float CALL get_slow_time(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x001E0002:
		_read_float_value(0x568);
	case 0x001E0003:
		_read_float_value(0x560);
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		_read_float_value(0x5E4);
	case 0x00370000:
		_read_float_value(0x76C);
	case 0x00370001:
		_read_float_value(0x85C);
	case 0x00370002:
		_read_float_value(0x864);
	case 0x00370003:
	case 0x00370004:
		_read_float_value(0x884);
	case 0x00370005:
		_read_float_value(0x8A0);
	case 0x00400001:
		_read_float_value(0x8E8);
	case 0x00400002:
		_read_float_value(0x900);
	case 0x00400003:
		_read_float_value(0x924);
	case 0x00400004:
	case 0x00400005:
		_read_float_value(0x91C);
	}
	return NAN;
}

EXP float CALL get_core_power(ryzen_access ry, uint32_t core) {
	switch (core)
	{
	case 0:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x300);
		case 0x00370005:
			_read_float_value(0x31C);
		case 0x003F0000: // Van Gogh
			_read_float_value(0x238); //568
		case 0x00400001:
			_read_float_value(0x304); //772
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x320); //800
		}
	case 1:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x304);
		case 0x00370005:
			_read_float_value(0x320);
		case 0x003F0000: // Van Gogh
			_read_float_value(0x23C); //572
		case 0x00400001:
			_read_float_value(0x308); //776
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x324); //804
		}
	case 2:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x308);
		case 0x00370005:
			_read_float_value(0x324);
		case 0x003F0000: // Van Gogh
			_read_float_value(0x240); //576
		case 0x00400001:
			_read_float_value(0x30c); //780
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x328); //808
		}
	case 3:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x30c);
		case 0x00370005:
			_read_float_value(0x328);
		case 0x003F0000: // Van Gogh
			_read_float_value(0x244); //580
		case 0x00400001:
			_read_float_value(0x310); //784
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x32c); //812
		}
	case 4:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x310);
		case 0x00370005:
			_read_float_value(0x32c);
		case 0x00400001:
			_read_float_value(0x314); //788
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x330); //816
		}

	case 5:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x314);
		case 0x00370005:
			_read_float_value(0x330);
		case 0x00400001:
			_read_float_value(0x318); //792
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x334); //820
		}
	case 6:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x318);
		case 0x00370005:
			_read_float_value(0x334);
		case 0x00400001:
			_read_float_value(0x31c); //796
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x338); //824
		}
	case 7:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x31c);
		case 0x00370005:
			_read_float_value(0x338);
		case 0x00400001:
			_read_float_value(0x320); //800
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x33c); //828
		}
	}
	return NAN;
}

EXP float CALL get_core_volt(ryzen_access ry, uint32_t core) {
	switch (core)
	{
	case 0:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x320);
		case 0x00370005:
			_read_float_value(0x33C);
		case 0x003F0000: // Van Gogh
			_read_float_value(0x248); //584
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x340); //832
		}
	case 1:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x324);
		case 0x00370005:
			_read_float_value(0x340);
		case 0x003F0000: // Van Gogh
			_read_float_value(0x24C); //588
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x344); //836
		}
	case 2:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x328);
		case 0x00370005:
			_read_float_value(0x344);
		case 0x003F0000: // Van Gogh
			_read_float_value(0x250); //592
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x348); //840
		}
	case 3:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x32C);
		case 0x00370005:
			_read_float_value(0x348);
		case 0x003F0000: // Van Gogh
			_read_float_value(0x254); //596
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x34c); //844
		}
	case 4:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x330);
		case 0x00370005:
			_read_float_value(0x34C);
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x350); //848

		}
	case 5:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x334);
		case 0x00370005:
			_read_float_value(0x350);
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x354); //852
		}
	case 6:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x338);
		case 0x00370005:
			_read_float_value(0x354);
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x358); //856
		}
	case 7:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x33C);
		case 0x00370005:
			_read_float_value(0x358);
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x35c); //860
		}
	}
	return NAN;
}

EXP float CALL get_core_temp(ryzen_access ry, uint32_t core) {
	switch (core)
	{
	case 0:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x340);
		case 0x00370005:
			_read_float_value(0x35C);
		case 0x003F0000: // Van Gogh
			_read_float_value(0x258); //600
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x360); //864
		}
	case 1:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x344);
		case 0x00370005:
			_read_float_value(0x360);
		case 0x003F0000: // Van Gogh
			_read_float_value(0x25C); //604
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x364); //868
		}
	case 2:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x348);
		case 0x00370005:
			_read_float_value(0x364);
		case 0x003F0000: // Van Gogh
			_read_float_value(0x260); //608
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x368); //872
		}
	case 3:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x34C);
		case 0x00370005:
			_read_float_value(0x368);
		case 0x003F0000: // Van Gogh
			_read_float_value(0x264); //612
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x36c); //876
		}
	case 4:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x350);
		case 0x00370005:
			_read_float_value(0x36C);
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x370); //880
		}
	case 5:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x354);
		case 0x00370005:
			_read_float_value(0x370);
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x374); //884
		}
	case 6:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x358);
		case 0x00370005:
			_read_float_value(0x374);
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x378); //888
		}
	case 7:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x35C);
		case 0x00370005:
			_read_float_value(0x378);
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x37C); //892
		}
	}
	return NAN;
}

EXP float CALL get_core_clk(ryzen_access ry, uint32_t core) {
	switch (core)
	{
	case 0:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x3A0);
		case 0x00370005:
			_read_float_value(0x3BC);
		case 0x003F0000: // Van Gogh
			_read_float_value(0x288); //648
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x3c0); //960
		}
	case 1:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x3A4);
		case 0x00370005:
			_read_float_value(0x3C0);
		case 0x003F0000: // Van Gogh
			_read_float_value(0x2C8); //652
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x3c4); //964
		}
	case 2:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x3A8);
		case 0x00370005:
			_read_float_value(0x3C4);
		case 0x003F0000: // Van Gogh
			_read_float_value(0x290); //656
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x3c8); //968
		}
	case 3:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x3AC);
		case 0x00370005:
			_read_float_value(0x3C8);
		case 0x003F0000: // Van Gogh
			_read_float_value(0x294); //660
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x3cc); //972
		}
	case 4:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x3B0);
		case 0x00370005:
			_read_float_value(0x3CC);
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x3d0); //976
		}
	case 5:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x3B4);
		case 0x00370005:
			_read_float_value(0x3D0);
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x3d4); //980
		}
	case 6:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x3B8);
		case 0x00370005:
			_read_float_value(0x3D4);
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x3d8); //984
		}
	case 7:
		switch (ry->table_ver)
		{
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			_read_float_value(0x3BC);
		case 0x00370005:
			_read_float_value(0x3D8);
		case 0x00400004:
		case 0x00400005:
			_read_float_value(0x3dc); //988
		}
	}
	return NAN;
}

EXP float CALL get_l3_clk(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
		_read_float_value(0x568);
	case 0x00370005:
		_read_float_value(0x584);
	case 0x003F0000: //Van Gogh
		_read_float_value(0x35C); //860
	case 0x00400004:
	case 0x00400005:
		_read_float_value(0x614); //1556
	}
	return NAN;
}

EXP float CALL get_l3_logic(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
		_read_float_value(0x540);
	case 0x00370005:
		_read_float_value(0x55C);
	case 0x003F0000: //Van Gogh
		_read_float_value(0x348); //840
	case 0x00400004:
	case 0x00400005:
		_read_float_value(0x600); //1536
	}
	return NAN;
}

EXP float CALL get_l3_vddm(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
		_read_float_value(0x548);
	case 0x00370005:
		_read_float_value(0x564);
	case 0x003F0000: //Van Gogh
		_read_float_value(0x34C); //844
	case 0x00400004:
	case 0x00400005:
		_read_float_value(0x604); //1540
	}
	return NAN;
}

EXP float CALL get_l3_temp(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
		_read_float_value(0x550);
	case 0x00370005:
		_read_float_value(0x56C);
	case 0x003F0000: //Van Gogh
		_read_float_value(0x350); //848
	case 0x00400004:
	case 0x00400005:
		_read_float_value(0x608); //1544
	}
	return NAN;
}

EXP float CALL get_gfx_clk(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
		_read_float_value(0x5B4);
	case 0x00370005:
		_read_float_value(0x5D0);
	case 0x00400001:
		_read_float_value(0x60C); //1548
	case 0x00400002:
		_read_float_value(0x624); //1572
	case 0x00400003:
		_read_float_value(0x644); //1604
	case 0x00400004:
	case 0x00400005:
		_read_float_value(0x648); //1608
	case 0x003F0000: //Van Gogh
		_read_float_value(0x388); //904
	}
	return NAN;
}

EXP float CALL get_gfx_volt(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
		_read_float_value(0x5A8);
	case 0x00370005:
		_read_float_value(0x5C4);
	case 0x00400001:
		_read_float_value(0x600); //1536
	case 0x00400002:
		_read_float_value(0x618); //1560
	case 0x00400003:
		_read_float_value(0x638); //1592
	case 0x00400004:
	case 0x00400005:
		_read_float_value(0x63C); //1596
	case 0x003F0000: //Van Gogh
		_read_float_value(0x37C); //896
	}
	return NAN;
}

EXP float CALL get_gfx_temp(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
		_read_float_value(0x5AC);
	case 0x00370005:
		_read_float_value(0x5C8);
	case 0x00400001:
		_read_float_value(0x604); //1540
	case 0x00400002:
		_read_float_value(0x61C); //1564
	case 0x00400003:
		_read_float_value(0x63C); //1596
	case 0x00400004:
	case 0x00400005:
		_read_float_value(0x640); //1600
	case 0x003F0000: //Van Gogh
		_read_float_value(0x380); //896
	}
	return NAN;
}

EXP float CALL get_fclk(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
		_read_float_value(0x5CC);
	case 0x00370005:
		_read_float_value(0x5E8);
	case 0x003F0000: //Van Gogh
		_read_float_value(0x3C5); //956
	case 0x00400004:
	case 0x00400005:
		_read_float_value(0x664); //1636
	}
	return NAN;
}

EXP float CALL get_mem_clk(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
		_read_float_value(0x5D4);
	case 0x00370005:
		_read_float_value(0x5F0);
	case 0x003F0000: //Van Gogh
		_read_float_value(0x3C4); //964
	case 0x00400004:
	case 0x00400005:
		_read_float_value(0x66c); //1644
	}
	return NAN;
}

EXP float CALL get_soc_volt(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
		_read_float_value(0x198);
	case 0x00370005:
		_read_float_value(0x198);
	case 0x003F0000: //Van Gogh
		_read_float_value(0x1A0); //416
	case 0x00400004:
	case 0x00400005:
		_read_float_value(0x19c); //412

	}
	return NAN;
}

EXP float CALL get_soc_power(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
		_read_float_value(0x1A0);
	case 0x00370005:
		_read_float_value(0x1A0);
	case 0x003F0000: //Van Gogh
		_read_float_value(0x1A8); //424
	case 0x00400004:
	case 0x00400005:
		_read_float_value(0x1a4); //420
	}
	return NAN;
}

EXP float CALL get_socket_power(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
		_read_float_value(0x98);
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
		_read_float_value(0x98); //152
	case 0x003F0000: //Van Gogh
		_read_float_value(0xA8); //168
	}
	return NAN;
}
