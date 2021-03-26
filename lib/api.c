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

	ry->family = family;
	//init version and power table only on demand to avoid unnecessary SMU writes
	ry->bios_if_ver = 0;
	ry->table_ver = 0;
	ry->table_addr = 0;
	ry->table_size = 0;
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
		goto out_free_nb;
	}

	ry->psmu = get_smu(ry->nb, TYPE_PSMU);
	if(!ry->psmu){
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
	free_mem_obj(ry->mem_obj);
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

int request_table_ver(ryzen_access ry)
{
	unsigned int get_table_ver_msg;
	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
		get_table_ver_msg = 0xC;
		break;
	case FAM_RENOIR:
	case FAM_CEZANNE:
		get_table_ver_msg = 0x6;
		break;
	default:
		printf("Get table version is not supported on this family\n");
		return PMTABLE_ERR_FAM_UNSUPPORTED;
	}

	smu_service_args_t args = {0, 0, 0, 0, 0, 0};
	int resp = smu_service_req(ry->psmu, get_table_ver_msg, &args);
	if (resp == REP_MSG_OK) {
		ry->table_ver = args.arg0;
		if(!ry->table_ver)
		{
			printf("Get table version did not return anything\n");
		}
		return 0;
	} else if (resp == REP_MSG_UnknownCmd) {
		printf("Get table version is unsupported\n");
		return PMTABLE_ERR_SMU_UNSUPPORTED;
	} else {
		printf("Get table version was rejected\n");
		return PMTABLE_ERR_SMU_REJECTED;
	}
}

EXP uint32_t get_table_ver(ryzen_access ry)
{
	if(!ry->table_ver)
		request_table_ver(ry);

	return ry->table_ver;
}

EXP size_t get_table_size(ryzen_access ry)
{
	if(ry->table_size)
		return ry->table_size;

	switch (get_table_ver(ry))
	{
		case 0x1E0004: ry->table_size = 0x6AC; break;
		case 0x1E0005: ry->table_size = 0x6AC; break;
		case 0x1E0101: ry->table_size = 0x6AC; break;
		case 0x240802: ry->table_size = 0x7E0; break;
		case 0x240803: ry->table_size = 0x7E4; break;
		case 0x240902: ry->table_size = 0x514; break;
		case 0x240903: ry->table_size = 0x518; break;
		case 0x2D0803: ry->table_size = 0x894; break;
		case 0x380804: ry->table_size = 0x8A4; break;
		case 0x2D0903: ry->table_size = 0x594; break;
		case 0x380904: ry->table_size = 0x5A4; break;
		case 0x370000: ry->table_size = 0x794; break;
		case 0x370001: ry->table_size = 0x884; break;
		case 0x370002: ry->table_size = 0x88C; break;
		case 0x370003: ry->table_size = 0x8AC; break;
		case 0x370004: ry->table_size = 0x8AC; break;
		case 0x370005: ry->table_size = 0x8C8; break;
		default:
			//use smallest size for unknown version to support dump of unknown tables
			ry->table_size = 0x514;
	}
	return ry->table_size;
}

int request_table_addr(ryzen_access ry)
{
	unsigned int get_table_addr_msg;
	smu_service_args_t args = {0, 0, 0, 0, 0, 0};
	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
		args.arg0 = 3;
		get_table_addr_msg = 0xB;
		break;
	case FAM_RENOIR:
	case FAM_CEZANNE:
		get_table_addr_msg = 0x66;
		break;
	default:
		printf("Get table addr is not supported on this family\n");
		return PMTABLE_ERR_FAM_UNSUPPORTED;
	}

	int resp = smu_service_req(ry->psmu, get_table_addr_msg, &args);
	if (resp == REP_MSG_OK) {
		ry->table_addr = args.arg0;
		if(!ry->table_addr)
		{
			printf("Get table addr did not return anything\n");
			return PMTABLE_ERR_SMU_UNSUPPORTED;
		}
	} else if (resp == REP_MSG_UnknownCmd) {
		printf("Get table addr is unsupported\n");
		return PMTABLE_ERR_SMU_UNSUPPORTED;
	} else {
		printf("Get table addr was rejected\n");
		return PMTABLE_ERR_SMU_REJECTED;
	}

	return 0;
}

EXP uint32_t get_table_addr(ryzen_access ry)
{
	if(!ry->table_addr)
		request_table_addr(ry);

	return ry->table_addr;
}

EXP int CALL init_pm_table(ryzen_access ry)
{
	DBG("init_pm_table\n");
	int errorcode = 0;

	errorcode = request_table_ver(ry);
	if(errorcode){
		return errorcode;
	}

	errorcode = request_table_addr(ry);
	if(errorcode){
		return errorcode;
	}

	//init memory object because it is prerequiremt to woring with physical memory address
	ry->mem_obj = init_mem_obj();
	if(!ry->mem_obj)
	{
		printf("Unable to get MEM Obj\n");
		return PMTABLE_ERR_MEMORY;
	}

	//hold copy of table value in memory for our single value getters
	ry->table_values = malloc(get_table_size(ry));

	errorcode = refresh_pm_table(ry);
	if(errorcode)
	{
		return errorcode;
	}

	if(!ry->table_values[0]){
		//Raven and Picasso don't get table refresh on the very first transfer call after boot, but respond with OK
		//if we detact 0 data, do an initial 2nd call after a small delay (copy_from_phyaddr is enough delay)
		//transfer, transfer, wait, wait longer; don't work
		//transfer, wait, wait longer; don't work
		//transfer, wait, transfer; does work
		return refresh_pm_table(ry);
	}

	return 0;
}

int CALL request_transfer_table(ryzen_access ry)
{
	int resp;
	unsigned int transfer_table_msg;
	smu_service_args_t args = {0, 0, 0, 0, 0, 0};
	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
		args.arg0 = 3;
		transfer_table_msg = 0x3D;
		break;
	case FAM_RENOIR:
	case FAM_CEZANNE:
		transfer_table_msg = 0x65;
		break;
	default:
		printf("Transfer table is not supported on this family\n");
		return PMTABLE_ERR_FAM_UNSUPPORTED;
	}

	resp = smu_service_req(ry->psmu, transfer_table_msg, &args);
	if(resp == REP_MSG_CmdRejectedPrereq){
		//2nd try is needed if SMU got interrupted or after boot on Renoir
		//need to wait because 2nd call will fail to if we don't wait: similar to Raven and Picasso issue
		//but because we don't have to check any physical memory values, don't waste CPU cycles and use sleep instead
		Sleep(10); 
		resp = smu_service_req(ry->psmu, transfer_table_msg, &args);
		if(resp == REP_MSG_CmdRejectedPrereq){
			printf("Transfer table was rejected twice\n");
			Sleep(100); 
			resp = smu_service_req(ry->psmu, transfer_table_msg, &args);
		}
	}

	if (resp == REP_MSG_OK) {
		return 0;
	} else if (resp == REP_MSG_UnknownCmd) {
		printf("Transfer table is unsupported\n");
		return PMTABLE_ERR_SMU_UNSUPPORTED;
	} else if (resp == REP_MSG_CmdRejectedPrereq){
		printf("Transfer table was rejected tree times\n");
		return PMTABLE_ERR_SMU_REJECTED;
	} else if (resp == REP_MSG_CmdRejectedBusy) {
		printf("Transfer table was rejected - busy\n");
		return PMTABLE_ERR_SMU_REJECTED;
	} else {
		printf("Transfer table failed\n");
		return PMTABLE_ERR_SMU_REJECTED;
	}
}

EXP int CALL refresh_pm_table(ryzen_access ry)
{
	int errorcode;
	if(ry->table_values) {
		errorcode = request_transfer_table(ry);
		if(errorcode){
			return errorcode;
		}

		if(copy_from_phyaddr(get_table_addr(ry), ry->table_values, get_table_size(ry))){
			return PMTABLE_ERR_MEMORY;
		}
	} else {
		DBG("refresh_pm_table was called before init\n");
		errorcode = init_pm_table(ry);
		if(errorcode){
			printf("Init power table failed: %d\n", errorcode);
			return errorcode;
		}
	}
	return 0;
}

EXP int CALL get_new_table(ryzen_access ry, void *dst, size_t size)
{
	int errorcode = 0;
	if(!ry->table_values) {
		//no table allocation, we need to init the table
		errorcode = init_pm_table(ry);
		if(errorcode){
			printf("Init power table failed: %d\n", errorcode);
			return errorcode;
		}
		if(size <= ry->table_size){
			//reuse table from init to avoid 2nd memory mapping
			memcpy(dst, ry->table_values, size < ry->table_size ? size : ry->table_size);
			return 0;
		}
	}

	errorcode = request_transfer_table(ry);
	if(errorcode){
		return errorcode;
	}

	if(copy_from_phyaddr(ry->table_addr, dst, size)){
		return PMTABLE_ERR_MEMORY;
	}
	//refresh internal table with new values
	memcpy(ry->table_values, dst, size > ry->table_size ? ry->table_size : size);

	return errorcode;
}

#define _do_adjust(OPT) \
do {                                                 \
	smu_service_args_t args = {0, 0, 0, 0, 0, 0};    \
	int resp;										 \
	args.arg0 = value;                               \
	resp = smu_service_req(ry->mp1_smu, OPT, &args); \
	if (resp == REP_MSG_OK) {                        \
		return 0;                                    \
	} else if (resp == REP_MSG_UnknownCmd) {         \
		return ADJ_ERR_SMU_UNSUPPORTED;              \
	} else {                                         \
		return ADJ_ERR_SMU_REJECTED;                 \
	}                                                \
} while (0);

#define _read_float_value(OFFSET)                   \
do {                                                \
	if(!ry->table_values)                           \
		return NAN;                                 \
	return ry->table_values[OFFSET / 4];            \
} while (0);



EXP int CALL set_stapm_limit(ryzen_access ry, uint32_t value){
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
		_do_adjust(0x14);
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_fast_limit(ryzen_access ry, uint32_t value){
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
		_do_adjust(0x15);
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_slow_limit(ryzen_access ry, uint32_t value){
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
		_do_adjust(0x16);
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_slow_time(ryzen_access ry, uint32_t value){
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
		_do_adjust(0x17);
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_stapm_time(ryzen_access ry, uint32_t value){
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
		_do_adjust(0x18);
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_tctl_temp(ryzen_access ry, uint32_t value){
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
		_do_adjust(0x19);
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_vrm_current(ryzen_access ry, uint32_t value){
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
		_do_adjust(0x1a);
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_vrmsoc_current(ryzen_access ry, uint32_t value){
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
		_do_adjust(0x1b);
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_vrmmax_current(ryzen_access ry, uint32_t value){
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
		_do_adjust(0x1c);
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_vrmsocmax_current(ryzen_access ry, uint32_t value){
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
		_do_adjust(0x1d);
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_psi0_current(ryzen_access ry, uint32_t value){
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
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_psi0soc_current(ryzen_access ry, uint32_t value){
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
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_max_gfxclk_freq(ryzen_access ry, uint32_t value) {
	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x46);
		break;
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_min_gfxclk_freq(ryzen_access ry, uint32_t value) {
	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x47);
		break;
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_max_socclk_freq(ryzen_access ry, uint32_t value){
	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x48);
		break;
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_min_socclk_freq(ryzen_access ry, uint32_t value){
	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x49);
		break;
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_max_fclk_freq(ryzen_access ry, uint32_t value){
	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x4A);
		break;
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_min_fclk_freq(ryzen_access ry, uint32_t value){
	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x4B);
		break;
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_max_vcn(ryzen_access ry, uint32_t value){
	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x4C);
		break;
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_min_vcn(ryzen_access ry, uint32_t value){
	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x4D);
		break;
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_max_lclk(ryzen_access ry, uint32_t value){
	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x4E);
		break;
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_min_lclk(ryzen_access ry, uint32_t value){
	switch (ry->family)
	{
	case FAM_RAVEN:
	case FAM_PICASSO:
	case FAM_DALI:
		_do_adjust(0x4F);
		break;
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_prochot_deassertion_ramp(ryzen_access ry, uint32_t value) {
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
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_apu_skin_temp_limit(ryzen_access ry, uint32_t value) {
	value *= 256;
	switch (ry->family)
	{
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
		_do_adjust(0x38);
		break;
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_dgpu_skin_temp_limit(ryzen_access ry, uint32_t value) {
	value *= 256;
	switch (ry->family)
	{
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
		_do_adjust(0x39);
		break;
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_apu_slow_limit(ryzen_access ry, uint32_t value) {
	switch (ry->family)
	{
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
		_do_adjust(0x21);
		break;
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

EXP int CALL set_skin_temp_power_limit(ryzen_access ry, uint32_t value) {
	switch (ry->family)
	{
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
		_do_adjust(0x53);
		break;
	}
	return ADJ_ERR_FAM_UNSUPPORTED;
}

//PM Table section, offset of first lines are stable across multiple PM Table versions
EXP float CALL get_stapm_limit(ryzen_access ry){_read_float_value(0x0);}
EXP float CALL get_stapm_value(ryzen_access ry){_read_float_value(0x4);}
EXP float CALL get_fast_limit(ryzen_access ry){_read_float_value(0x8);}
EXP float CALL get_fast_value(ryzen_access ry){_read_float_value(0xC);}
EXP float CALL get_slow_limit(ryzen_access ry){_read_float_value(0x10);}
EXP float CALL get_slow_value(ryzen_access ry){_read_float_value(0x14);}

//custom section, offsets are depending on ptable version
EXP float CALL get_apu_slow_limit(ryzen_access ry){
	switch (ry->table_ver)
	{
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
		_read_float_value(0x18);
	}
	return NAN;
}
EXP float CALL get_apu_slow_value(ryzen_access ry){
	switch (ry->table_ver)
	{
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
		_read_float_value(0x1C);
	}
	return NAN;
}
EXP float CALL get_vrm_current(ryzen_access ry){
	switch (ry->table_ver)
	{
	case 0x1E0004:
	case 0x1E0005:
		_read_float_value(0x18);
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
		_read_float_value(0x20);
	}
	return NAN;
}
EXP float CALL get_vrm_current_value(ryzen_access ry){
	switch (ry->table_ver)
	{
	case 0x1E0004:
	case 0x1E0005:
		_read_float_value(0x1C);
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
		_read_float_value(0x24);
	}
	return NAN;
}
EXP float CALL get_vrmsoc_current(ryzen_access ry){
	switch (ry->table_ver)
	{
	case 0x1E0004:
	case 0x1E0005:
		_read_float_value(0x20);
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
		_read_float_value(0x28);
	}
	return NAN;
}
EXP float CALL get_vrmsoc_current_value(ryzen_access ry){
	switch (ry->table_ver)
	{
	case 0x1E0004:
	case 0x1E0005:
		_read_float_value(0x24);
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
		_read_float_value(0x2C);
	}
	return NAN;
}
EXP float CALL get_vrmmax_current(ryzen_access ry){
	switch (ry->table_ver)
	{
	case 0x1E0004:
	case 0x1E0005:
		_read_float_value(0x28);
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
		_read_float_value(0x30);
	}
	return NAN;
}
EXP float CALL get_vrmmax_current_value(ryzen_access ry){
	switch (ry->table_ver)
	{
	case 0x1E0004:
	case 0x1E0005:
		_read_float_value(0x2C);
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
		_read_float_value(0x34);
	}
	return NAN;
}
EXP float CALL get_vrmsocmax_current(ryzen_access ry){
	switch (ry->table_ver)
	{
	case 0x1E0004:
	case 0x1E0005:
		_read_float_value(0x34);
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
		_read_float_value(0x38);
	}
	return NAN;
}
EXP float CALL get_vrmsocmax_current_value(ryzen_access ry){
	switch (ry->table_ver)
	{
	case 0x1E0004:
	case 0x1E0005:
		_read_float_value(0x38);
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
		_read_float_value(0x3C);
	}
	return NAN;
}
EXP float CALL get_tctl_temp(ryzen_access ry){
	switch (ry->table_ver)
	{
	case 0x1E0004:
	case 0x1E0005:
		_read_float_value(0x50);
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
		_read_float_value(0x40);
	}
	return NAN;
}
EXP float CALL get_tctl_temp_value(ryzen_access ry){
	switch (ry->table_ver)
	{
	case 0x1E0004:
	case 0x1E0005:
		_read_float_value(0x54);
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
		_read_float_value(0x44);
	}
	return NAN;
}
EXP float CALL get_apu_skin_temp_limit(ryzen_access ry){
	switch (ry->table_ver)
	{
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
		_read_float_value(0x58);
	}
	return NAN;
}
EXP float CALL get_apu_skin_temp_value(ryzen_access ry){
	switch (ry->table_ver)
	{
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
		_read_float_value(0x5C);
	}
	return NAN;
}
EXP float CALL get_dgpu_skin_temp_limit(ryzen_access ry){
	switch (ry->table_ver)
	{
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
		_read_float_value(0x60);
	}
	return NAN;
}
EXP float CALL get_dgpu_skin_temp_value(ryzen_access ry){
	switch (ry->table_ver)
	{
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
		_read_float_value(0x64);
	}
	return NAN;
}

EXP float CALL get_psi0_current(ryzen_access ry){
	switch (ry->table_ver)
	{
	case 0x1E0004:
	case 0x1E0005:
		_read_float_value(0x40);
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
		_read_float_value(0x78);
	}
	return NAN;
}

EXP float CALL get_psi0soc_current(ryzen_access ry){
	switch (ry->table_ver)
	{
	case 0x1E0004:
	case 0x1E0005:
		_read_float_value(0x48);
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
		_read_float_value(0x80);
	}
	return NAN;
}

EXP float CALL get_stapm_time(ryzen_access ry)
{
	switch (ry->table_ver)
	{
	case 0x1E0004:
	case 0x1E0005:
		_read_float_value(0x5E0);
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
		_read_float_value(0x880);
	case 0x00370005:
		_read_float_value(0x89C);
	}
	return NAN;
}

EXP float CALL get_slow_time(ryzen_access ry){
	switch (ry->table_ver)
	{
	case 0x1E0004:
	case 0x1E0005:
		_read_float_value(0x5E4);
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
		_read_float_value(0x884);
	case 0x00370005:
		_read_float_value(0x8A0);
	}
	return NAN;
}

