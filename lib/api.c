// SPDX-License-Identifier: LGPL
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* RyzenAdj API */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ryzenadj.h"
#include "math.h"

#ifndef _WIN32
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#endif

static unsigned int parse_smu_version_parts(unsigned int major, unsigned int minor, unsigned int patch, unsigned int build, int parts)
{
	if (major > 0xff || minor > 0xff || patch > 0xff || build > 0xff)
		return 0;
	if (parts == 4)
		return (major << 24) | (minor << 16) | (patch << 8) | build;
	if (parts == 3)
		return (major << 16) | (minor << 8) | patch;
	return 0;
}

#ifndef _WIN32
static unsigned int get_smu_version_from_sysfs(void)
{
	FILE *version_file = fopen("/sys/kernel/ryzen_smu_drv/version", "r");
	unsigned int major = 0;
	unsigned int minor = 0;
	unsigned int patch = 0;
	unsigned int build = 0;
	int parts;

	if (!version_file)
		return 0;

	parts = fscanf(version_file, "%u.%u.%u.%u", &major, &minor, &patch, &build);
	fclose(version_file);

	return parse_smu_version_parts(major, minor, patch, build, parts);
}
#endif


EXP ryzen_access CALL init_ryzenadj() {
	const enum ryzen_family family = cpuid_get_family();
	ryzen_access ry;

	if (family == FAM_UNKNOWN)
		return NULL;

	ry = (ryzen_access)malloc(sizeof(*ry));

	if (!ry){
		printf("Out of memory\n");
		return NULL;
	}

	memset(ry, 0, sizeof(*ry));

	ry->family = family;
	//init version and power metric table only on demand to avoid unnecessary SMU writes
	ry->bios_if_ver = 0;
	ry->table_values = NULL;

	ry->os_access = init_os_access_obj();
	if(!ry->os_access){
		printf("Unable to get os_access Obj, check permission\n");
		return NULL;
	}

#ifndef _WIN32
	if (is_using_smu_driver() &&
	    kmod_has_pm_table(ry->os_access) &&
	    !kmod_smn_writable(ry->os_access)) {
		// PM table sysfs can work without SMN write access (e.g. Matisse,
		// Strix Point with a recent ryzen_smu build).
		return ry;
	}
#endif

	ry->mp1_smu = get_smu(ry->os_access, TYPE_MP1);
	if(!ry->mp1_smu){
#ifndef _WIN32
		if (is_using_smu_driver()) {
			// Some systems expose the PM table through ryzen_smu sysfs while
			// blocking direct SMN writes. Keep read-only PM table access working.
			DBG("MP1 SMU unavailable, continuing with ryzen_smu PM table sysfs\n");
			goto skip_smu_init;
		}
#endif
		printf("Unable to get MP1 SMU Obj\n");
		goto err_exit;
	}

	ry->psmu = get_smu(ry->os_access, TYPE_PSMU);
	if(!ry->psmu){
#ifndef _WIN32
		if (is_using_smu_driver()) {
			DBG("RSMU unavailable, continuing with ryzen_smu PM table sysfs\n");
			goto skip_smu_init;
		}
#endif
		printf("Unable to get RSMU Obj\n");
		goto err_exit;
	}

skip_smu_init:
	return ry;

err_exit:
	cleanup_ryzenadj(ry);
	return NULL;
}

EXP void CALL cleanup_ryzenadj(ryzen_access ry) {
	if (ry == NULL)
	    return;

	free(ry->mp1_smu);
	free(ry->psmu);
	free_os_access_obj(ry->os_access);
	free(ry->table_values);
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
	if(!ry->mp1_smu)
		return 0;

	smu_service_args_t args = {0, 0, 0, 0, 0, 0};
	smu_service_req(ry->mp1_smu, 0x3, &args);
	ry->bios_if_ver = args.arg0;
	return ry->bios_if_ver;
}

EXP unsigned int get_smu_version(ryzen_access ry)
{
	// OP 0x02 (with arg0=1) returns the SMU firmware version. Unlike the BIOS
	// interface version query (msg 0x3, which returns 0 on desktop Raphael /
	// Dragon Range), this message works across all platforms. The result is
	// packed one byte per field: 0x04540400 -> 4.84.4.0.
	if(!ry->mp1_smu) {
#ifndef _WIN32
		if (is_using_smu_driver())
			return get_smu_version_from_sysfs();
#endif
		return 0;
	}
	smu_service_args_t args = {1, 0, 0, 0, 0, 0};
	smu_service_req(ry->mp1_smu, 0x2, &args);
	if (!args.arg0) {
#ifndef _WIN32
		if (is_using_smu_driver())
			return get_smu_version_from_sysfs();
#endif
	}
	return args.arg0;
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

static int request_table_ver_and_size(ryzen_access ry) {
	unsigned int get_table_ver_msg;
	int resp;

	switch (ry->family) {
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
		case FAM_HAWKPOINT:
		case FAM_KRACKANPOINT:
		case FAM_STRIXPOINT:
		case FAM_STRIXHALO:
			get_table_ver_msg = 0x6;
			break;
		case FAM_DRAGONRANGE:
		case FAM_FIRERANGE:
			get_table_ver_msg = 0x5;
			break;
		default:
#ifndef _WIN32
			// Desktop Raphael / Dragon Range (Ryzen 7000) have no known "get PM
			// table version" SMU message, but the ryzen_smu kernel module already
			// exposes the version and size through sysfs. Trust those, consistent
			// with init_table() which already prefers the kmod-reported size.
			if (is_using_smu_driver()) {
				ry->table_ver = ry->os_access->access.kmod.pm_table_version;
				ry->table_size = ry->os_access->access.kmod.pm_table_size;
				if (!ry->table_ver) {
					printf("ryzen_smu did not report a PM table version\n");
					return ADJ_ERR_SMU_UNSUPPORTED;
				}
				return 0;
			}
#endif
			printf("request_table_ver_and_size is not supported on this family\n");
			return ADJ_ERR_FAM_UNSUPPORTED;
	}

	smu_service_args_t args = {0, 0, 0, 0, 0, 0};
	resp = smu_service_req(ry->psmu, get_table_ver_msg, &args);
	ry->table_ver = args.arg0;

	switch (ry->table_ver) {
		case 0x1E0001: ry->table_size = 0x568; break;
		case 0x1E0002: ry->table_size = 0x580; break;
		case 0x1E0003: ry->table_size = 0x578; break;
		case 0x1E0004:
		case 0x1E0005:
		case 0x1E000A:
		case 0x1E0101: ry->table_size = 0x608; break;
		case 0x370000: ry->table_size = 0x794; break;
		case 0x370001: ry->table_size = 0x884; break;
		case 0x370002: ry->table_size = 0x88C; break;
		case 0x370003:
		case 0x370004: ry->table_size = 0x8AC; break;
		case 0x370005: ry->table_size = 0x8C8; break;
		case 0x3F0000: ry->table_size = 0x7AC; break;
		case 0x400001: ry->table_size = 0x910; break;
		case 0x400002: ry->table_size = 0x928; break;
		case 0x400003: ry->table_size = 0x94C; break;
		case 0x400004:
		case 0x400005: ry->table_size = 0x944; break;
		case 0x450004: ry->table_size = 0xAA4; break;
		case 0x450005: ry->table_size = 0xAB0; break;
		case 0x4C0003: ry->table_size = 0xB18; break;
		case 0x4C0004: ry->table_size = 0xB1C; break;
		case 0x4C0005: ry->table_size = 0xAF8; break;
		case 0x4C0006: ry->table_size = 0xAFC; break;
		case 0x4C0008: ry->table_size = 0xAF0; break;
		case 0x4C0007:
		case 0x4C0009: ry->table_size = 0xB00; break;
		case 0x5D0008:
		case 0x5D0009:
		case 0x5D000A:
		case 0x5D000B: ry->table_size = 0xD54; break;
		case 0x540104: ry->table_size = 0x6A8; break;
		case 0x540105: ry->table_size = 0x6B4; break;
		case 0x621102:
		case 0x620105: ry->table_size = 0x724; break;
		case 0x621202:
		case 0x620205: ry->table_size = 0x994; break;
		case 0x650004: ry->table_size = 0xB74; break;
		case 0x650005: ry->table_size = 0xB78; break;
		case 0x650006: ry->table_size = 0xB80; break;
		case 0x650007: ry->table_size = 0xD54; break;
		case 0x64020c: ry->table_size = 0xE50; break;
		// Dragon Range / Raphael
		case 0x540208: ry->table_size = 0x8D0; break;

		// use a larger size then the largest known table to be able to test real table size of unknown tables
		default: ry->table_size = 0x1000; break;
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

static int request_table_addr(ryzen_access ry)
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
	case FAM_HAWKPOINT:
	case FAM_KRACKANPOINT:
	case FAM_STRIXPOINT:
	case FAM_STRIXHALO:
		get_table_addr_msg = 0x66;
		break;
	case FAM_DRAGONRANGE:
	case FAM_FIRERANGE:
		args.arg0 = 1;
		args.arg1 = 1;
		get_table_addr_msg = 0x4;
		break;
	default:
#ifndef _WIN32
		// In ryzen_smu kernel-module mode the PM table is read straight from
		// sysfs (copy_pm_table_kmod), so its physical address is never needed
		// and init_mem_obj_kmod() ignores it.
		if (is_using_smu_driver()) {
			ry->table_addr = 0;
			return 0;
		}
#endif
		printf("request_table_addr is not supported on this family\n");
		return ADJ_ERR_FAM_UNSUPPORTED;
	}

	resp = smu_service_req(ry->psmu, get_table_addr_msg, &args);

	switch (ry->family)
	{
	case FAM_REMBRANDT:
	case FAM_PHOENIX:
	case FAM_HAWKPOINT:
	case FAM_KRACKANPOINT:
	case FAM_STRIXPOINT:
	case FAM_STRIXHALO:
	case FAM_DRAGONRANGE:
	case FAM_FIRERANGE:
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

static int request_transfer_table(ryzen_access ry)
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
	case FAM_HAWKPOINT:
	case FAM_KRACKANPOINT:
	case FAM_STRIXPOINT:
	case FAM_STRIXHALO:
		transfer_table_msg = 0x65;
		break;
	case FAM_DRAGONRANGE:
	case FAM_FIRERANGE:
		transfer_table_msg = 0x3;
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
	if (init_mem_obj(ry->os_access, ry->table_addr) < 0) {
		printf("Unable to get memory access for PM table monitoring\n");
#ifndef _WIN32
		if (is_using_smu_driver())
			printf("hint: install ryzen_smu with PM table sysfs support, or run as root for /dev/mem fallback\n");
		else
			printf("hint: run as root (sudo) for /dev/mem access, or load ryzen_smu with PM table support\n");
#endif
		return ADJ_ERR_MEMORY_ACCESS;
	}

	//when using the ryzen_smu kernel module, its reported PM table size is authoritative
	//because it reads the real value from hardware; prefer it over the SMU-derived size,
	//especially for unknown table versions that fall back to the 0x1000 sentinel
#ifndef _WIN32
	if (is_using_smu_driver() &&
	    ry->os_access->access.kmod.pm_table_size != ry->table_size) {
		DBG("PM table size: using kmod value (%zu) over SMU-derived value (%zu)\n",
		    ry->os_access->access.kmod.pm_table_size, ry->table_size);
		ry->table_size = ry->os_access->access.kmod.pm_table_size;
	}
#endif

	//hold copy of table value in memory for our single value getters
	ry->table_values = calloc(ry->table_size / 4, 4);

	errorcode = refresh_table(ry);
	if(errorcode)
	{
		return errorcode;
	}

	//On the Raphael / Dragon Range 0x540104 table, offset 0x0 is legitimately always
	//zero, so table_values[0] would flag every init as "empty" and force a needless
	//retry. Use offset 0x8 (fast PPT limit, nonzero when populated) for it.
	int table_empty = (ry->table_ver == 0x00540104 || ry->table_ver == 0x00540004 || ry->table_ver == 0x00620105 || ry->table_ver == 0x00620205)
		? !ry->table_values[0x8 / 4]
		: !ry->table_values[0];
	if(table_empty){
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

	// Read PM table through /dev/mem when ryzen_smu has no pm_table sysfs export.
	if(!is_using_smu_driver() || pm_table_uses_devmem(ry->os_access)){
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

	if(copy_pm_table(ry->os_access, ry->table_values, ry->table_size)){
#ifndef _WIN32
		if (is_using_smu_driver() && pm_table_uses_devmem(ry->os_access))
			printf("refresh_table failed: ryzen_smu has no PM table sysfs and /dev/mem read failed (run as root)\n");
		else if (is_using_smu_driver())
			printf("refresh_table failed: unable to read PM table from ryzen_smu sysfs\n");
		else
			printf("refresh_table failed: unable to read PM table via /dev/mem (run as root, check secure boot)\n");
#else
		printf("refresh_table failed\n");
#endif
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

#define _read_float_value(OFFSET)                    \
do {                                                 \
	if(!ry->table_values)                            \
		return NAN;                                  \
	return ry->table_values[(OFFSET) / 4];           \
} while (0);

static float read_float_value(ryzen_access ry, uint32_t offset)
{
	if (!ry->table_values)
		return NAN;
	return ry->table_values[offset / 4];
}

static float valid_range_or_nan(float value, float min, float max)
{
	if (value == value && value >= min && value <= max)
		return value;
	return NAN;
}

static float read_valid_range(ryzen_access ry, uint32_t offset, float min, float max)
{
	return valid_range_or_nan(read_float_value(ry, offset), min, max);
}

static float read_granite_ridge_hottest_core_temp(ryzen_access ry)
{
	float value = NAN;
	int core;

	if (!ry->table_values)
		return NAN;

	for (core = 0; core < 8; core++) {
		float core_temp = ry->table_values[(0x4F4 + (core * 4)) / 4];
		if (core_temp > 0.0f && core_temp < 130.0f &&
		    (!(value == value) || core_temp > value))
			value = core_temp;
	}

	return value;
}


EXP int CALL set_stapm_limit(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	/* \_SB.ALIB (0x0c, [size, 0x05, val]) */

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
	case FAM_HAWKPOINT:
	case FAM_KRACKANPOINT:
	case FAM_STRIXPOINT:
	case FAM_STRIXHALO:
		_do_adjust(0x14);
		if (err) {
			printf("%s: Retry with PSMU\n", __func__);
				_do_adjust_psmu(0x31);
		}
		break;
	case FAM_DRAGONRANGE:
	case FAM_FIRERANGE:
		_do_adjust(0x4f);
		break;
	default:
		break;
	}
	return err;
}

EXP int CALL set_fast_limit(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	/* \_SB.ALIB (0x0c, [size, 0x06, val]) */
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
	case FAM_HAWKPOINT:
	case FAM_KRACKANPOINT:
	case FAM_STRIXPOINT:
	case FAM_STRIXHALO:
		_do_adjust(0x15);
		break;
	case FAM_DRAGONRANGE:
	case FAM_FIRERANGE:
		_do_adjust(0x3e);
		break;
	default:
		break;
	}
	return err;
}

EXP int CALL set_slow_limit(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	/* \_SB.ALIB (0x0c, [size, 0x07, val]) */

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
	case FAM_HAWKPOINT:
	case FAM_KRACKANPOINT:
	case FAM_STRIXPOINT:
	case FAM_STRIXHALO:
		_do_adjust(0x16);
		break;
	case FAM_DRAGONRANGE:
	case FAM_FIRERANGE:
		_do_adjust(0x5f);
		break;
	default:
		break;
	}
	return err;
}

EXP int CALL set_slow_time(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	/* \_SB.ALIB (0x0c, [size, 0x08, val]) */

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
	case FAM_HAWKPOINT:
	case FAM_KRACKANPOINT:
	case FAM_STRIXPOINT:
	case FAM_STRIXHALO:
		_do_adjust(0x17);
		break;
	case FAM_DRAGONRANGE:
	case FAM_FIRERANGE:
		_do_adjust(0x60);
		break;
	default:
		break;
	}
	return err;
}

EXP int CALL set_stapm_time(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	/* \_SB.ALIB (0x0c, [size, 0x01, val]) */

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
	case FAM_HAWKPOINT:
	case FAM_KRACKANPOINT:
	case FAM_STRIXPOINT:
	case FAM_STRIXHALO:
		_do_adjust(0x18);
		break;
	case FAM_DRAGONRANGE:
	case FAM_FIRERANGE:
		_do_adjust(0x4e);
		break;
	default:
		break;
	}
	return err;
}

EXP int CALL set_tctl_temp(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	/* \_SB.ALIB (0x0c, [size, 0x03, val]) */

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
	case FAM_HAWKPOINT:
	case FAM_KRACKANPOINT:
	case FAM_STRIXPOINT:
	case FAM_STRIXHALO:
		_do_adjust(0x19);
		break;
	case FAM_DRAGONRANGE:
	case FAM_FIRERANGE:
		_do_adjust(0x3f);
		break;
	default:
		break;
	}
	return err;
}

EXP int CALL set_vrm_current(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	/* \_SB.ALIB (0x0c, [size, 0x0b, val]) */

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
	case FAM_HAWKPOINT:
	case FAM_KRACKANPOINT:
	case FAM_STRIXPOINT:
	case FAM_STRIXHALO:
		_do_adjust(0x1a);
		break;
	case FAM_DRAGONRANGE:
		_do_adjust(0x3c);
		break;
	case FAM_FIRERANGE:
		_do_adjust(0x3d);
		break;
	default:
		break;
	}
	return err;
}

EXP int CALL set_vrmsoc_current(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	/* \_SB.ALIB (0x0c, [size, 0x0e, val]) */

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
	case FAM_HAWKPOINT:
	case FAM_KRACKANPOINT:
	case FAM_STRIXPOINT:
	case FAM_STRIXHALO:
		_do_adjust(0x1b);
		break;
	default:
		break;
	}
	return err;
}

EXP int CALL set_vrmgfx_current(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_VANGOGH:
		_do_adjust(0x1c);
		break;
	default:
		break;
	}
	return err;
}

EXP int CALL set_vrmcvip_current(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_VANGOGH:
		_do_adjust(0x1d);
		break;
	default:
		break;
	}
	return err;
}

EXP int CALL set_vrmmax_current(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	/* \_SB.ALIB (0x0c, [size, 0x0c, val]) */

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
	case FAM_HAWKPOINT:
	case FAM_KRACKANPOINT:
	case FAM_STRIXPOINT:
	case FAM_STRIXHALO:
		_do_adjust(0x1c);
		break;
	case FAM_VANGOGH:
		_do_adjust(0x1e);
		break;
	case FAM_FIRERANGE:
		_do_adjust(0x3c);
		break;
	default:
		break;
	}
	return err;
}

EXP int CALL set_vrmgfxmax_current(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_VANGOGH:
		_do_adjust(0x1f);
		break;
	default:
		break;
	}
	return err;
}

EXP int CALL set_vrmsocmax_current(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	/* \_SB.ALIB (0x0c, [size, 0x11, val]) */

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
	case FAM_HAWKPOINT:
	case FAM_KRACKANPOINT:
	case FAM_STRIXPOINT:
	case FAM_STRIXHALO:
		_do_adjust(0x1d);
		break;
	default:
		break;
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
		break;
	default:
		break;
	}
	return err;
}

EXP int CALL set_psi3cpu_current(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_VANGOGH:
		_do_adjust(0x20);
		break;
	default:
		break;
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
		break;
	default:
		break;
	}
	return err;
}

EXP int CALL set_psi3gfx_current(ryzen_access ry, uint32_t value){
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	switch (ry->family)
	{
	case FAM_VANGOGH:
		_do_adjust(0x21);
		break;
	default:
		break;
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
	case FAM_LUCIENNE:
		_do_adjust(0x46);
		break;
	default:
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
	case FAM_LUCIENNE:
		_do_adjust(0x47);
		break;
	default:
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
	default:
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
	default:
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
	default:
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
	default:
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
	default:
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
	default:
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
	default:
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
	default:
		break;
	}
	return err;
}

EXP int CALL set_prochot_deassertion_ramp(ryzen_access ry, uint32_t value) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	/* \_SB.ALIB (0x0c, [size, 0x09, val]) */

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
	case FAM_HAWKPOINT:
	case FAM_KRACKANPOINT:
	case FAM_STRIXPOINT:
	case FAM_STRIXHALO:
		_do_adjust(0x1f);
		break;
	default:
		break;
	}
	return err;
}

EXP int CALL set_apu_skin_temp_limit(ryzen_access ry, uint32_t value) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	/* \_SB.ALIB (0x0c, [size, 0x22, val]) */

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
	case FAM_HAWKPOINT:
		_do_adjust(0x33);
		break;
	case FAM_STRIXHALO:
		// not implemented on StrixHalo, seems to be controlled only via tctl-temp
	default:
		break;
	}
	return err;
}

EXP int CALL set_dgpu_skin_temp_limit(ryzen_access ry, uint32_t value) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	/* \_SB.ALIB (0x0c, [size, 0x23, val]) */

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
	case FAM_HAWKPOINT:
	case FAM_STRIXPOINT:
		_do_adjust(0x34);
		break;
	case FAM_STRIXHALO:
		// not implemented on StrixHalo, seems to be controlled only via tctl-temp
	default:
		break;
	}
	return err;
}

EXP int CALL set_apu_slow_limit(ryzen_access ry, uint32_t value) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;

	/* \_SB.ALIB (0x0c, [size, 0x13, val]) */

	switch (ry->family)
	{
	case FAM_RENOIR:
	case FAM_LUCIENNE:
	case FAM_CEZANNE:
		_do_adjust(0x21);
		break;
	case FAM_REMBRANDT:
	case FAM_PHOENIX:
	case FAM_HAWKPOINT:
	case FAM_KRACKANPOINT:
	case FAM_STRIXPOINT:
	case FAM_STRIXHALO:
		_do_adjust(0x23);
		break;
	default:
		break;
	}
	return err;
}

EXP int CALL set_skin_temp_power_limit(ryzen_access ry, uint32_t value) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;

    	/* \_SB.ALIB (0x0c, [size, 0x2e, val]) */

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
	case FAM_HAWKPOINT:
	case FAM_KRACKANPOINT:
	case FAM_STRIXPOINT:
	case FAM_STRIXHALO:
		_do_adjust(0x4a);
		break;
	default:
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
	case FAM_HAWKPOINT:
	case FAM_KRACKANPOINT: /* Added to debug on KRK, STX, & STXH */
	case FAM_STRIXPOINT:
	case FAM_STRIXHALO:
		_do_adjust_psmu(0x89);
		break;
	default:
		break;
	}
	return err;
}

EXP int CALL set_power_saving(ryzen_access ry) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;
	uint32_t value = 0;

	/* \_SB.ALIB (0x01, [size, 0x1]) */

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
	case FAM_HAWKPOINT:
	case FAM_KRACKANPOINT:
	case FAM_STRIXPOINT:
	case FAM_STRIXHALO:
		_do_adjust(0x12);
		break;
	default:
		break;
	}
	return err;
}

EXP int CALL set_max_performance(ryzen_access ry) {
    int err = ADJ_ERR_FAM_UNSUPPORTED;
	uint32_t value = 0;

	/* \_SB.ALIB (0x01, [size, 0x0]) */

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
	case FAM_HAWKPOINT:
	case FAM_KRACKANPOINT:
	case FAM_STRIXPOINT:
	case FAM_STRIXHALO:
		_do_adjust(0x11);
		break;
	default:
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
	default:
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
	default:
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
		_do_adjust(0x33);
        if (err) {
            printf("%s: Retry with PSMU\n", __func__);
		    _do_adjust_psmu(0x1b);
        }
		break;
	default:
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
	default:
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
	default:
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
	case FAM_PHOENIX:
	case FAM_HAWKPOINT:
	case FAM_KRACKANPOINT:
	case FAM_STRIXPOINT:
	case FAM_STRIXHALO:
		_do_adjust(0x4C);
		break;
	case FAM_DRAGONRANGE:
	case FAM_FIRERANGE:
		_do_adjust_psmu(0x7);
		break;
	default:
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
		break;
	case FAM_REMBRANDT:
	case FAM_PHOENIX:
	case FAM_VANGOGH:
	case FAM_HAWKPOINT:
	case FAM_KRACKANPOINT:
	case FAM_STRIXPOINT:
	case FAM_STRIXHALO:
		_do_adjust(0x4b);
		break;
	case FAM_DRAGONRANGE:
	case FAM_FIRERANGE:
		_do_adjust_psmu(0x6);
		break;
	default:
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
	case FAM_PHOENIX:
	case FAM_HAWKPOINT:
	case FAM_VANGOGH:
		_do_adjust_psmu(0xB7);
		break;
	case FAM_STRIXHALO:
		// 0xB7 is rejected on this architecture
	default:
		break;
	}
	return err;
}

//PM Table section, offset of first lines are stable across multiple PM Table versions
// The "stable prefix" below mostly does NOT hold for desktop Raphael / Dragon
// Range (table 0x540104), Granite Ridge (table 0x620105), and Fire Range
// (table 0x620205). The fast PPT limit is still at 0x8, while STAPM and slow PPT are not confirmed there; keep them
// hidden until located.
EXP float CALL get_stapm_limit(ryzen_access ry){if(ry->table_ver==0x00540104||ry->table_ver==0x00540004||ry->table_ver==0x00620105||ry->table_ver==0x00620205||ry->table_ver==0x00240903)return NAN;_read_float_value(0x0);}
EXP float CALL get_stapm_value(ryzen_access ry){if(ry->table_ver==0x00540104||ry->table_ver==0x00540004||ry->table_ver==0x00620105||ry->table_ver==0x00620205||ry->table_ver==0x00240903)return NAN;_read_float_value(0x4);}
EXP float CALL get_fast_limit(ryzen_access ry){if(ry->table_ver==0x00240903){_read_float_value(0x0);}_read_float_value(0x8);}
EXP float CALL get_fast_value(ryzen_access ry){if(ry->table_ver==0x00620105){return read_valid_range(ry, 0x458, 0.001f, 1000.0f);}if(ry->table_ver==0x00240903){_read_float_value(0x4);}_read_float_value(0xC);}
EXP float CALL get_slow_limit(ryzen_access ry){
	if(ry->table_ver==0x00620105){return read_valid_range(ry,0x3EC,0.001f,500.0f);}
	if(ry->table_ver==0x00540104||ry->table_ver==0x00540004||ry->table_ver==0x00620205||ry->table_ver==0x00240903)return NAN;
	_read_float_value(0x10);
}
EXP float CALL get_slow_value(ryzen_access ry){if(ry->table_ver==0x00540104||ry->table_ver==0x00540004||ry->table_ver==0x00620105||ry->table_ver==0x00620205||ry->table_ver==0x00240903)return NAN;_read_float_value(0x14);}

//custom section, offsets are depending on table version
EXP float CALL get_apu_slow_limit(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000A:
	case 0x005D000B:
	case 0x00650005:
	case 0x00650007:
		return read_valid_range(ry, 0x18, 0.001f, 500.0f);
	default:
		break;
	}
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
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
	case 0x005D0008: // Strix Point - looks correct from dumping table, defaults to 45W
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650007:
	case 0x0064020c: // StrixHalo - looks correct from dumping table, defaults to 70W
	case 0x00650005: // Krackan Point
		_read_float_value(0x18);
	default:
		break;
	}
	return NAN;
}
EXP float CALL get_apu_slow_value(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000A:
	case 0x005D000B:
	case 0x00650005:
	case 0x00650007:
		return read_valid_range(ry, 0x1C, 0.001f, 500.0f);
	default:
		break;
	}
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
	case 0x004C0009:
	case 0x005D0008: // Strix Point - untested, always 0?
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650007:
	case 0x0064020c: // StrixHalo - untested!
	case 0x00650005: // Krackan Point
		_read_float_value(0x1C);
	default:
		break;
	}
	return NAN;
}
EXP float CALL get_vrm_current(ryzen_access ry) {
	if (ry->table_ver == 0x00540104 || ry->table_ver == 0x00540004) {
		// Raphael / Dragon Range / EPYC 4004: some boards expose a real TDC
		// limit at 0x20 (75A on EPYC 4344P). Others report 1000A here, which is
		// effectively an unlimited/sentinel value, so keep those hidden.
		if (!ry->table_values)
			return NAN;
		float value = ry->table_values[0x20 / 4];
		if (value > 0.0f && value <= 500.0f)
			return value;
		return NAN;
	}
	if (ry->table_ver == 0x00620105)
		return read_valid_range(ry, 0x28, 0.001f, 500.0f);
	if (ry->table_ver == 0x00620205)
		return read_valid_range(ry, 0x20, 0.001f, 500.0f);
	if (ry->table_ver == 0x00240903)
		return read_valid_range(ry, 0x8, 0.001f, 500.0f);
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
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
	case 0x00540208: // Dragon Range / Raphael
		_read_float_value(0x20);
	case 0x005D0008: // Strix Point - tested, defaults to 70, max 70
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650007:
	case 0x00650005: // Krackan Point
		_read_float_value(0x30);
	default:
		break;
	}
	return NAN;
}
EXP float CALL get_vrm_current_value(ryzen_access ry) {
	// Raphael / Dragon Range (0x540104): TDC, current pulled from the VDD rail (A)
	if (ry->table_ver == 0x00540104 || ry->table_ver == 0x00540004) { _read_float_value(0x50); }
	if (ry->table_ver == 0x00620105) { _read_float_value(0x438); }
	if (ry->table_ver == 0x00620205) { _read_float_value(0x24); }
	if (ry->table_ver == 0x00240903) { _read_float_value(0xC); }
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
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
	case 0x00540208: // Dragon Range / Raphael
		_read_float_value(0x24);
	case 0x005D0008: // Strix Point - looks correct from dumping table
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650007:
	case 0x00650005: // Krackan Point
		_read_float_value(0x34);
	default:
		break;
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
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
		_read_float_value(0x28);
	case 0x005D0008: // Strix Point - tested, defaults to 30, max 30
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650007:
	case 0x00650005: // Krackan Point
		_read_float_value(0x38);
	default:
		break;
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
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
		_read_float_value(0x2C);
	case 0x005D0008: // Strix Point - looks correct from dumping table
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650007:
	case 0x00650005: // Krackan Point
		_read_float_value(0x3C);
	default:
		break;
	}
	return NAN;
}
EXP float CALL get_vrmmax_current(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000A:
	case 0x005D000B:
	case 0x00650005:
	case 0x00650007:
		return NAN;
	default:
		break;
	}
	if (ry->table_ver == 0x00540104 || ry->table_ver == 0x00540004) {
		// EPYC 4004 exposes the EDC limit at 0xF4. As with the TDC limit,
		// desktop/mobile dumps can carry a 1000A sentinel here.
		if (!ry->table_values)
			return NAN;
		float value = ry->table_values[0xF4 / 4];
		if (value > 0.0f && value <= 500.0f)
			return value;
		return NAN;
	}
	if (ry->table_ver == 0x00620105)
		return read_valid_range(ry, 0x20, 0.001f, 500.0f);
	if (ry->table_ver == 0x00620205)
		return read_valid_range(ry, 0xFC, 0.001f, 500.0f);
	if (ry->table_ver == 0x00240903)
		return read_valid_range(ry, 0x20, 0.001f, 500.0f);
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
	case 0x004C0006:
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
		_read_float_value(0x30);
	default:
		break;
	}
	return NAN;
}
EXP float CALL get_vrmmax_current_value(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000A:
	case 0x005D000B:
	case 0x00650005:
	case 0x00650007:
		return NAN;
	default:
		break;
	}
	switch (ry->table_ver)
	{
	case 0x00620205:
		_read_float_value(0x100);
	case 0x00240903:
		_read_float_value(0x24);
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
	case 0x004C0006:
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
		_read_float_value(0x34);
	default:
		break;
	}
	return NAN;
}
EXP float CALL get_vrmsocmax_current(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000A:
	case 0x005D000B:
	case 0x00650005:
	case 0x00650007:
		return NAN;
	default:
		break;
	}
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
	case 0x004C0006:
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
		_read_float_value(0x38);
	default:
		break;
	}
	return NAN;
}
EXP float CALL get_vrmsocmax_current_value(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000A:
	case 0x005D000B:
	case 0x00650005:
	case 0x00650007:
		return NAN;
	default:
		break;
	}
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
	case 0x004C0006:
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
		_read_float_value(0x3C);
	default:
		break;
	}
	return NAN;
}
EXP float CALL get_tctl_temp(ryzen_access ry) {
	// Raphael / Dragon Range (0x540104): THM limit (Tctl throttle target, C).
	// Confirmed: constant 80 across dumps while the value at 0x2C rises to ~81
	// under load and holds there (thermal-limited), i.e. a configured 80C cap.
	if (ry->table_ver == 0x00620105) { _read_float_value(0x344); }
	if (ry->table_ver == 0x00540104 || ry->table_ver == 0x00540004 || ry->table_ver == 0x00620205) { _read_float_value(0x28); }
	if (ry->table_ver == 0x00240903) { return NAN; }
	switch (ry->table_ver)
	{
	case 0x001E0001:
	case 0x001E0002:
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
	case 0x0064020c: // StrixHalo tested
		_read_float_value(0x58); //use core1 because core0 is not reported on dual core cpus
	case 0x00540208: // Dragon Range / Raphael
		_read_float_value(0x28);
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
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
	case 0x005D0008: // Strix Point - untested, defaults to 100
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650007:
	case 0x00650005: // Krackan Point
		_read_float_value(0x40);
	default:
		break;
	}
	return NAN;
}
EXP float CALL get_tctl_temp_value(ryzen_access ry) {
	// Raphael / Dragon Range (0x540104): CPU control temperature Tctl (C)
	if (ry->table_ver == 0x00620105) {
		// Granite Ridge aggregate thermal metrics are encoded; the per-core
		// block is direct Celsius, so expose the hottest core as THM VALUE.
		return read_granite_ridge_hottest_core_temp(ry);
	}
	if (ry->table_ver == 0x00540004) {
		// 0x2C and nearby thermal values do not consistently match k10temp Tctl
		// on EPYC 4584PX, so keep THM VALUE hidden until the control temp is known.
		return NAN;
	}
	if (ry->table_ver == 0x00620205) {
		// Fire Range 0x2C moves with thermals, but did not consistently match
		// k10temp Tctl/Tccd during sampling, so keep the aggregate value hidden.
		return NAN;
	}
	if (ry->table_ver == 0x00240903) {
		// Matisse PM table 0x240903: offset 0x14 tracks k10temp Tctl/TSI0.
		_read_float_value(0x14);
	}
	if (ry->table_ver == 0x00540104) {
		if (!ry->table_values)
			return NAN;
		float value = ry->table_values[0x2C / 4];
		float alternate = ry->table_values[0xF0 / 4];
		// 0x2C tracks Tctl on Raphael desktops. EPYC 4004 can report the
		// k10temp-matching control temperature at 0xF0; 0xF8 is not stable there.
		if (alternate > value && alternate < 130.0f)
			value = alternate;
		return value;
	}
	if (ry->table_ver == 0x001E0004) {
		// Picasso 0x1e0004 has a later per-core temperature block that tracks
		// k10temp/TSI0. Return the hottest core as the package control temp.
		int core;
		float value;
		if (!ry->table_values)
			return NAN;
		value = ry->table_values[0x624 / 4];
		for (core = 1; core < 4; core++) {
			float core_temp = ry->table_values[(0x624 + (core * 4)) / 4];
			if (core_temp > value)
				value = core_temp;
		}
		return value;
	}
	switch (ry->table_ver)
	{
	case 0x001E0001:
	case 0x001E0002:
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
	case 0x0064020c: // StrixHalo tested
		_read_float_value(0x5C); //use core1 because core0 is not reported on dual core cpus
	case 0x00540208: // Dragon Range / Raphael
		_read_float_value(0x2C);
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
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
	/*
	 * Strix Point - tested
	 * 0x44: Zen5 clst, taskset + stress-ng
	 * 0x4C: Zen5c clst, ditto
	 * 0x54: gfx, llama-bench/FurMark, gpu_metrics_v3_0
	 * 0x5C: soc, all of above + memtester
	 * Corresponding limits default to 100 (untested)
	 */
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000A:
	case 0x005D000B:
	case 0x00650005:
	case 0x00650007:
		_read_float_value(0x44);
	default:
		break;
	}
	return NAN;
}
EXP float CALL get_apu_skin_temp_limit(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000A:
	case 0x005D000B:
	case 0x00650005:
	case 0x00650007:
		return read_valid_range(ry, 0x58, 20.0f, 130.0f);
	default:
		break;
	}
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
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
	case 0x005D0008: // Strix Point - untested
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650007:
	case 0x0064020c: // StrixHalo tested
		_read_float_value(0x58);
		break;
	default:
		break;
	}
	return NAN;
}
EXP float CALL get_apu_skin_temp_value(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000A:
	case 0x005D000B:
	case 0x00650005:
	case 0x00650007:
		return read_valid_range(ry, 0x5C, 20.0f, 130.0f);
	default:
		break;
	}
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
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
	case 0x005D0008: // Strix Point - this is gpu_metrics_v3_0.temperature_soc, !=gpu_metrics_v3_0.temperature_skin
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650007:
	case 0x0064020c: // StrixHalo tested
		_read_float_value(0x5C);
	default:
		break;
	}
	return NAN;
}
EXP float CALL get_dgpu_skin_temp_limit(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000A:
	case 0x005D000B:
	case 0x00650005:
	case 0x00650007:
		return read_valid_range(ry, 0x68, 20.0f, 130.0f);
	default:
		break;
	}
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
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
	case 0x0064020c: // StrixHalo tested
		_read_float_value(0x60);
	case 0x005D0008: // Strix Point - tested
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650007:
		_read_float_value(0x68);
	default:
		break;
	}
	return NAN;
}
EXP float CALL get_dgpu_skin_temp_value(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000A:
	case 0x005D000B:
	case 0x00650005:
	case 0x00650007:
		return read_valid_range(ry, 0x6C, 20.0f, 130.0f);
	default:
		break;
	}
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
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
	case 0x0064020c:
		_read_float_value(0x64);
	case 0x005D0008: // Strix Point - calculated from corresponding limit + 0x4, 0 on my device due to no dGPU
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650007:
		_read_float_value(0x6C);
	default:
		break;
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
	case 0x004C0006:
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
		_read_float_value(0x78);
	default:
		break;
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
	case 0x004C0006:
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
		_read_float_value(0x80);
	default:
		break;
	}
	return NAN;
}

EXP float CALL get_cclk_setpoint(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00620105:
		return read_valid_range(ry, 0x440, 0.001f, 10.0f);
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
	case 0x005D0008: // Strix Point - first entry of 12-core boost target array, tested
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650007:
		_read_float_value(0xD0);
	default:
		break;
	}
	return NAN;
}

EXP float CALL get_cclk_busy_value(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00620105:
		return read_valid_range(ry, 0x71C, 0.001f, 10.0f);
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
	case 0x005D0008: // Strix Point - aggregate busy clock, tested
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650007:
		_read_float_value(0xCC);
	default:
		break;
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
	case 0x004C0006:
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
		_read_float_value(0x918);
	case 0x005D0008: // Strix Point - calculated from slow time (0x9C0 - 0x4), always 1?
	case 0x005D0009:
	case 0x005D000A:
	case 0x005D000B:
	case 0x00650007:
		_read_float_value(0x9BC);
	case 0x00650005: // Krackan Point, might be incorrect
		_read_float_value(0x90C);
	default:
		break;
	}
	return NAN;
}

EXP float CALL get_slow_time(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000A:
	case 0x005D000B:
	case 0x00650007:
		return read_valid_range(ry, 0x9C0, 0.001f, 10000.0f);
	default:
		break;
	}
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
	case 0x004C0006:
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
		_read_float_value(0x91C);
	case 0x00650005: // Krackan Point, might be incorrect
		_read_float_value(0x910);
	default:
		break;
	}
	return NAN;
}

EXP float CALL get_core_power(ryzen_access ry, uint32_t core) {
	if (core > 15)
		return NAN;

	uint32_t baseOffset;
	// kevin 0x104 might be power, 16 entries
	switch (ry->table_ver) {
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			baseOffset = 0x300;
			break;
		case 0x00370005:
			baseOffset = 0x31C;
			break;
		case 0x003F0000: { // Van Gogh
			if (core >= 4)
				return NAN;

			baseOffset = 0x238;
		}
			break;
		case 0x00400001:
			baseOffset = 0x304;
			break;
		case 0x00400004:
		case 0x00400005:
			baseOffset = 0x320;
			break;
		case 0x001E0004: // Picasso, 4 physical cores
			if (core >= 4)
				return NAN;
			baseOffset = 0x180;
			break;
		case 0x005D0008: // Strix Point - 12-core arrays start at 0x9D4 on 0x5d000b
		case 0x005D0009:
		case 0x005D000B:
		case 0x00650007:
			baseOffset = 0x9D4;
			break;
		case 0x0064020c: // Strix Halo
			baseOffset = 0xB90;
			break;
		case 0x00620105: // Granite Ridge, 8-entry CCD layout
			if (core >= 8)
				return NAN;
			baseOffset = 0x534;
			break;
		case 0x00540004: // EPYC 4004 / 16-core Raphael, 16 core entries
			baseOffset = 0x494;
			break;
		case 0x00540104: // Raphael / Dragon Range desktop (Ryzen 7000), 6/8-core CCD
			baseOffset = 0x494;
			break;
		case 0x00620205: // Fire Range, 16-entry CCD layout with disabled-core holes
			baseOffset = 0x4B4;
			break;
		case 0x00240903: // Matisse desktop, 8-entry layout with disabled-core holes
			baseOffset = 0x24C;
			break;
		default:
			return NAN;
	}

	_read_float_value(baseOffset + (core * 4));
}

EXP float CALL get_core_volt(ryzen_access ry, uint32_t core) {
	if (core > 15)
		return NAN;

	uint32_t baseOffset;
	// kevinh 0x1cc - 17 entries?
	switch (ry->table_ver) {
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			baseOffset = 0x320;
			break;
		case 0x00370005:
			baseOffset = 0x33C;
			break;
		case 0x003F0000: { // Van Gogh
			if (core >= 4)
				return NAN;

			baseOffset = 0x248;
		}
			break;
		case 0x00400004:
		case 0x00400005:
			baseOffset = 0x340;
			break;
		case 0x001E0004: // Picasso, 4 physical cores
			if (core >= 4)
				return NAN;
			baseOffset = 0x1A0;
			break;
		case 0x005D0008: // Strix Point - 12-core arrays start at 0xA04 on 0x5d000b
		case 0x005D0009:
		case 0x005D000B:
		case 0x00650007:
			baseOffset = 0xA04;
			break;
		case 0x0064020c: // Strix Halo
			baseOffset = 0xBD0;
			break;
		case 0x00620105: // Granite Ridge
			if (core >= 8)
				return NAN;
			baseOffset = 0x4D4;
			break;
		case 0x00540004: // EPYC 4004 / 16-core Raphael
			baseOffset = 0x4D4;
			break;
		case 0x00540104: // Raphael / Dragon Range desktop (Ryzen 7000)
			baseOffset = 0x4B4;
			break;
		case 0x00620205: // Fire Range
			baseOffset = 0x4F4;
			break;
		case 0x00240903: // Matisse desktop
			baseOffset = 0x26C;
			break;
		default:
			return NAN;
	}

	_read_float_value(baseOffset + (core * 4));
}

EXP float CALL get_core_temp(ryzen_access ry, uint32_t core) {
	if (core > 15)
		return NAN;

	uint32_t baseOffset;

	switch (ry->table_ver) {
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			baseOffset = 0x340;
			break;
		case 0x00370005:
			baseOffset = 0x35C;
			break;
		case 0x003F0000: { // Van Gogh
			if (core >= 4)
				return NAN;

			baseOffset = 0x258;
		}
			break;
		case 0x00400004:
		case 0x00400005:
			baseOffset = 0x360;
			break;
		case 0x001E0004: // Picasso, 4 physical cores
			if (core >= 4)
				return NAN;
			baseOffset = 0x624;
			break;
		case 0x005D0008: // Strix Point - 12-core arrays start at 0xA34 on 0x5d000b
		case 0x005D0009:
		case 0x005D000B:
		case 0x00650007:
			baseOffset = 0xA34;
			break;
		case 0x0064020c: // Strix Halo
			baseOffset = 0xC10;
			break;
		case 0x00620105: // Granite Ridge
			if (core >= 8)
				return NAN;
			baseOffset = 0x4F4;
			break;
		case 0x00540004: // EPYC 4004 / 16-core Raphael
			baseOffset = 0x514;
			break;
		case 0x00540104: // Raphael / Dragon Range desktop (Ryzen 7000)
			baseOffset = 0x4D4;
			break;
		case 0x00620205: // Fire Range
			baseOffset = 0x534;
			break;
		case 0x00240903: // Matisse desktop
			baseOffset = 0x28C;
			break;
		default:
			return NAN;
	}

	_read_float_value(baseOffset + (core * 4));
}

EXP float CALL get_core_clk(ryzen_access ry, uint32_t core) {
	if (core > 15)
		return NAN;

	if (ry->table_ver == 0x001E0004) {
		// Picasso reports the sampled core clock in MHz; RyzenAdj's per-core
		// clock getters use GHz in the CLI table.
		if (core >= 4 || !ry->table_values)
			return NAN;
		return ry->table_values[(0x1C0 + (core * 4)) / 4] / 1000.0f;
	}

	uint32_t baseOffset;

	switch (ry->table_ver) {
		case 0x00370000:
		case 0x00370001:
		case 0x00370002:
		case 0x00370003:
		case 0x00370004:
			baseOffset = 0x3A0;
			break;
		case 0x00370005:
			baseOffset = 0x3BC;
			break;
		case 0x003F0000: { // Van Gogh
			if (core >= 4)
				return NAN;

			baseOffset = 0x288;
		}
			break;
		case 0x00400004:
		case 0x00400005:
			baseOffset = 0x3c0;
			break;
		case 0x005D0008: // Strix Point - 12-core arrays start at 0xA64 on 0x5d000b
		case 0x005D0009:
		case 0x005D000B:
		case 0x00650007:
			baseOffset = 0xA64;
			break;
		case 0x0064020c:
			baseOffset = 0xc50;
			break;
		case 0x00620105: // Granite Ridge
			if (core >= 8)
				return NAN;
			baseOffset = 0x514;
			break;
		case 0x00540004: // EPYC 4004 / 16-core Raphael
			baseOffset = 0x554;
			break;
		case 0x00540104: // Raphael / Dragon Range desktop (Ryzen 7000)
			baseOffset = 0x4F4;
			break;
		case 0x00620205: // Fire Range
			baseOffset = 0x574;
			break;
		case 0x00240903: // Matisse desktop
			baseOffset = 0x2EC;
			break;
		default:
			return NAN;
	}

	_read_float_value(baseOffset + (core * 4));
}

// Effective (achieved) per-core clock in GHz. New metric: RyzenAdj historically
// only exposed the requested clock via get_core_clk.
EXP float CALL get_core_freqeff(ryzen_access ry, uint32_t core) {
	if (core > 15)
		return NAN;

	uint32_t baseOffset;
	switch (ry->table_ver) {
		case 0x005D0008: // Strix Point - 12-core arrays, 0x30 stride
		case 0x005D0009:
		case 0x005D000B:
		case 0x00650007:
			baseOffset = 0xA94;
			break;
		case 0x0064020c: // Strix Halo - 16-core arrays, 0x40 stride
			baseOffset = 0xC90;
			break;
		case 0x00540004: // EPYC 4004 / 16-core Raphael
			baseOffset = 0x594;
			break;
		case 0x00540104: // Raphael / Dragon Range desktop (Ryzen 7000)
			baseOffset = 0x514;
			break;
		case 0x00620205: // Fire Range
			baseOffset = 0x5B4;
			break;
		case 0x00240903: // Matisse desktop
			baseOffset = 0x30C;
			break;
		default:
			return NAN;
	}

	_read_float_value(baseOffset + (core * 4));
}

// Per-core C0 residency (% of time the core was active). New metric.
EXP float CALL get_core_c0(ryzen_access ry, uint32_t core) {
	if (core > 15)
		return NAN;

	uint32_t baseOffset;
	switch (ry->table_ver) {
		case 0x005D0008:
		case 0x005D0009:
		case 0x005D000B:
		case 0x00650007:
			baseOffset = 0xAB4;
			break;
		case 0x0064020c:
			baseOffset = 0xCD0;
			break;
		case 0x00620105:
			if (core >= 8)
				return NAN;
			baseOffset = 0x594;
			break;
		case 0x00540004:
			baseOffset = 0x5D4;
			break;
		case 0x00540104: // Raphael / Dragon Range desktop (Ryzen 7000)
			baseOffset = 0x534;
			break;
		case 0x00620205: // Fire Range
			baseOffset = 0x5F4;
			break;
		case 0x00240903: // Matisse desktop
			baseOffset = 0x32C;
			break;
		default:
			return NAN;
	}

	_read_float_value(baseOffset + (core * 4));
}

// Per-core CC1 residency (% of time in the CC1 shallow sleep state). New metric.
EXP float CALL get_core_cc1(ryzen_access ry, uint32_t core) {
	if (core > 15)
		return NAN;

	uint32_t baseOffset;
	switch (ry->table_ver) {
		case 0x005D0008:
		case 0x005D0009:
		case 0x005D000B:
		case 0x00650007:
			baseOffset = 0xAD4;
			break;
		case 0x0064020c:
			baseOffset = 0xD10;
			break;
		case 0x00620105:
			if (core >= 8)
				return NAN;
			baseOffset = 0x5B4;
			break;
		case 0x00540004:
			baseOffset = 0x614;
			break;
		case 0x00540104: // Raphael / Dragon Range desktop (Ryzen 7000)
			baseOffset = 0x554;
			break;
		case 0x00620205: // Fire Range
			baseOffset = 0x634;
			break;
		case 0x00240903: // Matisse desktop
			baseOffset = 0x34C;
			break;
		default:
			return NAN;
	}

	_read_float_value(baseOffset + (core * 4));
}

// Per-core CC6 residency (% of time in the CC6 deep sleep state). New metric.
EXP float CALL get_core_cc6(ryzen_access ry, uint32_t core) {
	if (core > 15)
		return NAN;

	uint32_t baseOffset;
	switch (ry->table_ver) {
		case 0x005D0008:
		case 0x005D0009:
		case 0x005D000B:
		case 0x00650007:
			baseOffset = 0xAF4;
			break;
		case 0x0064020c:
			baseOffset = 0xD50;
			break;
		case 0x00620105:
			if (core >= 8)
				return NAN;
			baseOffset = 0x574;
			break;
		case 0x00540004:
			baseOffset = 0x654;
			break;
		case 0x00540104: // Raphael / Dragon Range desktop (Ryzen 7000)
			baseOffset = 0x574;
			break;
		case 0x00620205: // Fire Range
			baseOffset = 0x674;
			break;
		case 0x00240903: // Matisse desktop
			baseOffset = 0x36C;
			break;
		default:
			return NAN;
	}

	_read_float_value(baseOffset + (core * 4));
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
	default:
		break;
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
	default:
		break;
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
	default:
		break;
	}
	return NAN;
}

EXP float CALL get_l3_temp(ryzen_access ry) {
	if (ry->table_ver == 0x00620105)
		return read_valid_range(ry, 0x4A8, 1.0f, 130.0f);

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
	default:
		break;
	}
	return NAN;
}

EXP float CALL get_gfx_power(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00620105:
		return read_valid_range(ry, 0x1AC, 0.001f, 300.0f);
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000A:
	case 0x005D000B:
	case 0x00650005:
	case 0x00650007:
		// Strix Point iGPU power (W); rises under FurMark, not memtester.
		return read_valid_range(ry, 0x4B4, 0.001f, 300.0f);
	case 0x0064020c:
		return read_valid_range(ry, 0x54C, 0.001f, 300.0f);
	default:
		break;
	}
	return NAN;
}

EXP float CALL get_gfx_clk(ryzen_access ry) {
	if (ry->table_ver == 0x001E0004) { _read_float_value(0x2DC); }
	// Raphael / Dragon Range iGPU clock, matches amdgpu hwmon freq1_input.
	if (ry->table_ver == 0x00540104) { return read_valid_range(ry, 0x190, 1.0f, 10000.0f); }
	if (ry->table_ver == 0x00620105) { return read_valid_range(ry, 0x1B0, 1.0f, 10000.0f); }
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
	/*
	 * Strix Point - tested, RAPL + gpu_metrics_v3_0 + llama-bench/FurMark/memtester + switching platform_profile
	 * 0x4B4: gfx power, as FurMark consumes more than llama-bench when pkg power is constant
	 *        memtester won't increase this, so this is not uncore power
	 * 0x4B8: gfx temp on 0x5d000b dumps, 4C{0,4}: gfx-related clk (MHz), 4C8: unknown clk
	 */
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000A:
	case 0x005D000B:
	case 0x00650005:
	case 0x00650007:
		_read_float_value(0x4C0); // 4C0 and 4C4 are always close to each other, but 4C0 seems more correct
	case 0x0064020c: // Strix Halo
		_read_float_value(0x558);
	default:
		break;
	}
	return NAN;
}

EXP float CALL get_gfx_volt(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000A:
	case 0x005D000B:
	case 0x00650005:
	case 0x00650007:
		// On the 0x5d000b dump this offset tracks ~46C, not voltage.
		return NAN;
	default:
		break;
	}
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
	case 0x0064020c: // Strix Halo
		_read_float_value(0x54C);
	default:
		break;
	}
	return NAN;
}

EXP float CALL get_gfx_temp(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00540104:
		// Raphael / Dragon Range iGPU edge temp, matches amdgpu hwmon temp1_input.
		if (!(read_float_value(ry, 0x190) >= 1.0f))
			return NAN;
		return read_valid_range(ry, 0x188, 1.0f, 130.0f);
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000A:
	case 0x005D000B:
	case 0x00650005:
	case 0x00650007:
		return read_valid_range(ry, 0x4B8, 1.0f, 130.0f);
	default:
		break;
	}
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
	case 0x005D0008: // Strix Point
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650007:
		_read_float_value(0x4C8);
	case 0x0064020c: // Strix Halo
		_read_float_value(0x550);
	default:
		break;
	}
	return NAN;
}

EXP float CALL get_fclk(ryzen_access ry) {
	// Raphael / Dragon Range (0x540104): Infinity Fabric clock (MHz)
	if (ry->table_ver == 0x00540104 || ry->table_ver == 0x00540004) { _read_float_value(0x1A4); }
	if (ry->table_ver == 0x00620105) { return read_valid_range(ry, 0x11C, 1.0f, 10000.0f); }
	if (ry->table_ver == 0x00620205) { return read_valid_range(ry, 0x1C4, 1.0f, 10000.0f); }
	if (ry->table_ver == 0x00240903) { return read_valid_range(ry, 0xC0, 1.0f, 10000.0f); }
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
	/*
	 * Strix Point - tested (AI 365, LPDDR5-7500), switching platform_profile
	 * 0x4E0-0x548 seems to be target clocks (avg between two reads)
	 * 0x550-0x5B4 seems to be corresponding sampled clocks (avg between two reads)
	 * 0x550-0x568 maybe 0 if platform_profile is low-power
	 * 0x4E0/54C: fabric_clk = gpu_metrics_v3_0.average_fclk_frequency
	 * 0x4E4/550: controller_clk? = DRM_IOCTL_AMDGPU_INFO GFX_MCLK/gpu_metrics_v3_0.average_uclk_frequency
	 * 0x4E8/554: phy_clk? = controller_clk * 2
	 * 0x4EC/558: transfer rate (MT/s) = phy_clk * 2 (low-power) or 4 (balanced/performance)
	 * 0x4F0/55C: = gpu_metrics_v3_0.average_vclk_frequency
	 * 0x4F8/564: = gpu_metrics_v3_0.average_socclk_frequency
	 * 0x50C/578: = gpu_metrics_v3_0.average_mpipu_frequency
	 * 0x510/57C: = gpu_metrics_v3_0.average_ipuclk_frequency
	 * Possible 0x4E0 values:
	 *   400, 1050, 1200, 1400, 1600, 1960 (declared at 0x240-0x254)
	 *   400, 1050, 1200, 1400, 1500, 1600, 1800, 1960 (amdgpu sysfs pp_dpm_fclk)
	 * Possible 0x4E4 values:
	 *   400, 800, 937.5, 937.5, 937.5, 937.5 (declared at 0x258-0x26C)
	 *   400, 800, 937.5 (amdgpu sysfs pp_dpm_mclk)
	 * Possible 0x4E8 values:
	 *   800, 1600, 1875, 1875, 1875, 1875 (declared at 0x270-0x284)
	 * Possible 0x4EC values:
	 *   1600, 6400, 7500, 7500, 7500, 7500 (declared at 0x288-0x29C)
	 * Possible 0x4F0 values:
	 *   800, 800, 1605, 1766, 1962, 2208, 2523, 2944 (amdgpu sysfs pp_dpm_vclk)
	 * Possible 0x4F8 values:
	 *   600, 736, 883, 981, 1104, 1261, 1261, 1472 (amdgpu sysfs pp_dpm_socclk)
	 * See also: https://semiengineering.com/advantages-of-lpddr5-a-new-clocking-scheme/
	 */
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000A:
	case 0x005D000B:
	case 0x00650005:
	case 0x00650007:
		_read_float_value(0x4E0);
	default:
		break;
	}
	return NAN;
}

EXP float CALL get_mem_clk(ryzen_access ry) {
	// Raphael / Dragon Range (0x540104): memory clock (MHz)
	if (ry->table_ver == 0x00540104 || ry->table_ver == 0x00540004) { _read_float_value(0x1D4); }
	if (ry->table_ver == 0x00620105) { _read_float_value(0x13C); }
	if (ry->table_ver == 0x001E0004) { _read_float_value(0x298); }
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
	case 0x005D0008: // Strix Point - UCLK/controller clock, see get_fclk()
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650007:
		_read_float_value(0x4E4);
	default:
		break;
	}
	return NAN;
}

EXP float CALL get_uclk(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00240903: // Matisse desktop - memory controller clock
		return read_valid_range(ry, 0xC4, 1.0f, 10000.0f);
	case 0x00620105: // Granite Ridge - memory controller clock
		return read_valid_range(ry, 0x12C, 1.0f, 10000.0f);
	case 0x00620205: // Fire Range - memory controller clock
		return read_valid_range(ry, 0x1C8, 1.0f, 10000.0f);
	case 0x005D0008: // Strix Point - UCLK/controller clock, see get_fclk()
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650007:
		return read_valid_range(ry, 0x4E4, 1.0f, 10000.0f);
	default:
		break;
	}
	return NAN;
}

EXP float CALL get_mem_phy_clk(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00240903: // Matisse desktop - memory PHY clock
		return read_valid_range(ry, 0xC8, 1.0f, 10000.0f);
	case 0x00620205: // Fire Range - memory PHY clock
		return read_valid_range(ry, 0x1CC, 1.0f, 10000.0f);
	case 0x005D0008: // Strix Point - memory PHY clock, see get_fclk()
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650007:
		return read_valid_range(ry, 0x4E8, 1.0f, 10000.0f);
	default:
		break;
	}
	return NAN;
}

EXP float CALL get_mem_transfer_rate(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x00240903: { // Matisse desktop - DDR transfer rate
		float phy = get_mem_phy_clk(ry);
		if (phy == phy)
			return phy * 2.0f;
		return NAN;
	}
	case 0x00620205: { // Fire Range - DDR transfer rate
		float phy = get_mem_phy_clk(ry);
		if (phy == phy)
			return phy * 2.0f;
		return NAN;
	}
	case 0x00620105: { // Granite Ridge - DDR transfer rate from memory clock
		float mem = get_mem_clk(ry);
		if (mem == mem)
			return mem * 2.0f;
		return NAN;
	}
	case 0x005D0008: // Strix Point - LPDDR transfer rate (MT/s), see get_fclk()
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650007: {
		float raw = read_float_value(ry, 0x4EC);
		float phy = get_mem_phy_clk(ry);

		if (raw == raw && phy == phy && raw >= phy && raw <= 20000.0f)
			return raw;
		if (phy == phy)
			return phy * 2.0f;
		return NAN;
	}
	default:
		break;
	}
	return NAN;
}

EXP float CALL get_vclk(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x005D0008: // Strix Point - VCN clock, see get_fclk()
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650007:
		return read_valid_range(ry, 0x4F0, 1.0f, 10000.0f);
	default:
		break;
	}
	return NAN;
}

EXP float CALL get_socclk(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x005D0008: // Strix Point - SoC clock, see get_fclk()
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650007:
		return read_valid_range(ry, 0x4F8, 1.0f, 10000.0f);
	default:
		break;
	}
	return NAN;
}

EXP float CALL get_mpipu_clk(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x005D0008: // Strix Point - MPIPU clock, see get_fclk()
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650007:
		return read_valid_range(ry, 0x50C, 1.0f, 10000.0f);
	default:
		break;
	}
	return NAN;
}

EXP float CALL get_ipu_clk(ryzen_access ry) {
	switch (ry->table_ver)
	{
	case 0x005D0008: // Strix Point - IPU clock, see get_fclk()
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650007:
		return read_valid_range(ry, 0x510, 1.0f, 10000.0f);
	default:
		break;
	}
	return NAN;
}

EXP float CALL get_soc_volt(ryzen_access ry) {
	// Raphael / Dragon Range (0x540104): SoC-domain voltage (~1.1V on DDR5). Best
	// effort: this ~1.1V rail is stable idle-vs-load; it may be VDDCR_SOC or the
	// coupled memory voltage, but the reading is a valid SoC-domain voltage.
	if (ry->table_ver == 0x00540104 || ry->table_ver == 0x00540004) { _read_float_value(0xE0); }
	if (ry->table_ver == 0x00620105) { return read_valid_range(ry, 0x43C, 0.001f, 2.0f); }
	if (ry->table_ver == 0x00620205) { _read_float_value(0xE8); }
	if (ry->table_ver == 0x00240903) { _read_float_value(0xB0); }
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

	default:
		break;
	}
	return NAN;
}

EXP float CALL get_soc_power(ryzen_access ry) {
	if (ry->table_ver == 0x00620105) { return read_valid_range(ry, 0x54, 0.001f, 500.0f); }
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
	default:
		break;
	}
	return NAN;
}

EXP float CALL get_socket_power(ryzen_access ry) {
	// Raphael / Dragon Range (0x540104): whole-socket power (W)
	if (ry->table_ver == 0x00540104 || ry->table_ver == 0x00540004) { _read_float_value(0x68); }
	if (ry->table_ver == 0x00620105) { _read_float_value(0x50); }
	if (ry->table_ver == 0x00620205) { _read_float_value(0x68); }
	if (ry->table_ver == 0x00240903) { _read_float_value(0x4); }
	switch (ry->table_ver)
	{
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000A:
	case 0x005D000B:
	case 0x00650005:
	case 0x00650007:
		// On the 0x5d000b dump 0xD0 matches the CCLK boost setpoint, not socket power.
		return NAN;
	default:
		break;
	}
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
	default:
		break;
	}
	return NAN;
}
