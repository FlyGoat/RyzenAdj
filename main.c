// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* Ryzen NB SMU Service Request Tool */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "lib/ryzenadj.h"
#include "argparse.h"
#include "json_output.h"

#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)
#define ARRAY_SIZE(ARRAY) (sizeof(ARRAY) / sizeof((ARRAY)[0]))

#define _do_adjust(ARG, OPTION) \
do {                                                                              \
	/* ignore max unsigned integer values */                                      \
	if ((ARG) != UINT32_MAX) {                                                    \
		int adjerr = set_##ARG(ry, (ARG));                                        \
		if (json) {                                                               \
			if (record_adjustment(adjustment_results,                           \
				ARRAY_SIZE(adjustment_results), &result.adjustment_count,       \
				OPTION, RYZENADJ_JSON_VALUE_U32, (ARG), adjerr)) {               \
				set_json_error(&result, &json_error, "internal", "failed", -1); \
				json_exit_code = -1;                                           \
			}                                                                     \
		} else {                                                                  \
			print_adjustment_result(STRINGIFY(ARG), (ARG), 0, adjerr);            \
		}                                                                         \
		if (!adjerr){                                                             \
			any_adjust_applied = 1;                                               \
		} else {                                                                  \
			err = -1;                                                             \
			if (!json_exit_code)                                                  \
				json_exit_code = -1;                                             \
		}                                                                         \
	}                                                                             \
} while(0);

#define _do_enable(ARG, OPTION) \
do {                                                                              \
	if (ARG) {                                                                    \
		int adjerr = set_##ARG(ry);                                               \
		if (json) {                                                               \
			if (record_adjustment(adjustment_results,                           \
				ARRAY_SIZE(adjustment_results), &result.adjustment_count,       \
				OPTION, RYZENADJ_JSON_VALUE_BOOLEAN, 1, adjerr)) {               \
				set_json_error(&result, &json_error, "internal", "failed", -1); \
				json_exit_code = -1;                                           \
			}                                                                     \
		} else {                                                                  \
			print_adjustment_result(STRINGIFY(ARG), 1, 1, adjerr);                \
		}                                                                         \
		if (!adjerr){                                                             \
			any_adjust_applied = 1;                                               \
		} else {                                                                  \
			err = -1;                                                             \
			if (!json_exit_code)                                                  \
				json_exit_code = -1;                                             \
		}                                                                         \
	}                                                                             \
} while(0);

static const char *const usage[] = {
	"ryzenadj [options]",
	NULL,
};

static int only_json_arguments(int argc, const char **argv)
{
	int index;

	if (argc < 2)
		return 0;

	for (index = 1; index < argc; index++) {
		if (strcmp(argv[index], "--json") != 0)
			return 0;
	}

	return 1;
}

static int record_adjustment(struct ryzenadj_json_adjustment *adjustments,
	size_t capacity, size_t *count, const char *option,
	enum ryzenadj_json_value_type value_type, uint32_t value, int code)
{
	struct ryzenadj_json_adjustment *adjustment;

	if (*count >= capacity)
		return -1;

	adjustment = &adjustments[(*count)++];
	adjustment->option = option;
	adjustment->value_type = value_type;
	adjustment->value = value;
	adjustment->code = code;

	return 0;
}

static void set_json_error(struct ryzenadj_json_result *result,
	struct ryzenadj_json_error *error, const char *stage,
	const char *status, int code)
{
	if (result->error)
		return;

	error->stage = stage;
	error->status = status;
	error->code = code;
	result->error = error;
}

static void print_adjustment_result(const char *name, uint32_t value,
	int is_boolean, int code)
{
	if (!code) {
		if (is_boolean)
			printf("Successfully enable %s\n", name);
		else
			printf("Successfully set %s to %u\n", name, value);
	} else if (code == ADJ_ERR_FAM_UNSUPPORTED) {
		printf("set_%s is not supported on this family\n", name);
	} else if (code == ADJ_ERR_SMU_UNSUPPORTED) {
		printf("set_%s is not supported on this SMU\n", name);
	} else if (code == ADJ_ERR_SMU_REJECTED) {
		printf("set_%s is rejected by SMU\n", name);
	} else {
		printf("Failed to set%s \n", name);
	}
}

static const char *family_name(enum ryzen_family fam)
{
	switch (fam) {
		case FAM_RAVEN: return "Raven";
		case FAM_PICASSO: return "Picasso";
		case FAM_RENOIR: return "Renoir";
		case FAM_CEZANNE: return "Cezanne";
		case FAM_DALI: return "Dali";
		case FAM_LUCIENNE: return "Lucienne";
		case FAM_VANGOGH: return "Vangogh";
		case FAM_REMBRANDT: return "Rembrandt";
		case FAM_PHOENIX: return "Phoenix Point";
		case FAM_HAWKPOINT: return "Hawk Point";
		case FAM_DRAGONRANGE: return "Dragon Range";
		case FAM_KRACKANPOINT: return "Krackan Point";
		case FAM_STRIXPOINT: return "Strix Point";
		case FAM_STRIXHALO: return "Strix Halo";
		case FAM_FIRERANGE: return "Fire Range";
		default: break;
	}

	return "Unknown";
}

struct info_metric {
	const char *key;
	const char *name;
	const char *parameter;
	float (CALL *get_value)(ryzen_access ry);
};

static const struct info_metric info_metrics[] = {
	{ "stapm_limit_w", "STAPM LIMIT", "stapm-limit", get_stapm_limit },
	{ "stapm_value_w", "STAPM VALUE", "", get_stapm_value },
	{ "fast_limit_w", "PPT LIMIT FAST", "fast-limit", get_fast_limit },
	{ "fast_value_w", "PPT VALUE FAST", "", get_fast_value },
	{ "slow_limit_w", "PPT LIMIT SLOW", "slow-limit", get_slow_limit },
	{ "slow_value_w", "PPT VALUE SLOW", "", get_slow_value },
	{ "stapm_time_s", "StapmTimeConst", "stapm-time", get_stapm_time },
	{ "slow_time_s", "SlowPPTTimeConst", "slow-time", get_slow_time },
	{ "apu_slow_limit_w", "PPT LIMIT APU", "apu-slow-limit", get_apu_slow_limit },
	{ "apu_slow_value_w", "PPT VALUE APU", "", get_apu_slow_value },
	{ "vrm_current_a", "TDC LIMIT VDD", "vrm-current", get_vrm_current },
	{ "vrm_current_value_a", "TDC VALUE VDD", "", get_vrm_current_value },
	{ "vrmsoc_current_a", "TDC LIMIT SOC", "vrmsoc-current", get_vrmsoc_current },
	{ "vrmsoc_current_value_a", "TDC VALUE SOC", "", get_vrmsoc_current_value },
	{ "vrmmax_current_a", "EDC LIMIT VDD", "vrmmax-current", get_vrmmax_current },
	{ "vrmmax_current_value_a", "EDC VALUE VDD", "", get_vrmmax_current_value },
	{ "vrmsocmax_current_a", "EDC LIMIT SOC", "vrmsocmax-current", get_vrmsocmax_current },
	{ "vrmsocmax_current_value_a", "EDC VALUE SOC", "", get_vrmsocmax_current_value },
	{ "tctl_temp_limit_c", "THM LIMIT CORE", "tctl-temp", get_tctl_temp },
	{ "tctl_temp_value_c", "THM VALUE CORE", "", get_tctl_temp_value },
	{ "apu_skin_temp_limit_c", "STT LIMIT APU", "apu-skin-temp", get_apu_skin_temp_limit },
	{ "apu_skin_temp_value_c", "STT VALUE APU", "", get_apu_skin_temp_value },
	{ "dgpu_skin_temp_limit_c", "STT LIMIT dGPU", "dgpu-skin-temp", get_dgpu_skin_temp_limit },
	{ "dgpu_skin_temp_value_c", "STT VALUE dGPU", "", get_dgpu_skin_temp_value },
	{ "cclk_setpoint", "CCLK Boost SETPOINT", "power-saving /", get_cclk_setpoint },
	{ "cclk_busy_value", "CCLK BUSY VALUE", "max-performance", get_cclk_busy_value },
};

static void show_info_header(ryzen_access ry)
{
	printf("CPU Family: %s\n", family_name(get_cpu_family(ry)));
	printf("SMU BIOS Interface Version: %d\n", get_bios_if_ver(ry));
	printf("Version: v" STRINGIFY(RYZENADJ_REVISION_VER) "." STRINGIFY(RYZENADJ_MAJOR_VER) "." STRINGIFY(RYZENADJ_MINIOR_VER) " \n");
}

static void show_info_table(ryzen_access ry)
{
	size_t index;

	printf("PM Table Version: %x\n", get_table_ver(ry));

	//get refresh table after adjust
	int errorcode = refresh_table(ry);
	if(errorcode){
		printf("Unable to refresh power metric table: %d\n", errorcode);
		return;
	}

	//print table in github markdown
	printf("|        Name         |   Value   |     Parameter      |\n");
	printf("|---------------------|-----------|--------------------|\n");
	char tableFormat[] = "| %-19s | %9.3lf | %-18s |\n";
	for (index = 0; index < sizeof(info_metrics) / sizeof(info_metrics[0]); index++) {
		printf(tableFormat, info_metrics[index].name,
			info_metrics[index].get_value(ry), info_metrics[index].parameter);
	}
}

static int collect_info_json(ryzen_access ry, struct ryzenadj_json_info *info,
	struct ryzenadj_json_metric *metrics, size_t metric_capacity)
{
	size_t index;
	int errorcode = refresh_table(ry);

	if (errorcode)
		return errorcode;
	if (metric_capacity < ARRAY_SIZE(info_metrics))
		return -1;

	info->cpu_family = family_name(get_cpu_family(ry));
	info->smu_bios_interface_version = get_bios_if_ver(ry);
	info->ryzenadj_version = STRINGIFY(RYZENADJ_REVISION_VER) "." STRINGIFY(RYZENADJ_MAJOR_VER) "." STRINGIFY(RYZENADJ_MINIOR_VER);
	info->pm_table_version = get_table_ver(ry);
	for (index = 0; index < ARRAY_SIZE(info_metrics); index++) {
		metrics[index].key = info_metrics[index].key;
		metrics[index].value = info_metrics[index].get_value(ry);
	}
	info->metrics = metrics;
	info->metric_count = ARRAY_SIZE(info_metrics);

	return 0;
}

static void show_table_dump(ryzen_access ry, int any_adjust_applied)
{
	size_t index, table_size;
	uint32_t *table_data_copy;
	float *current_table_values, *old_table_values;

	printf("PM Table Dump of Version: %x\n", get_table_ver(ry));
	table_size = get_table_size(ry);

	current_table_values = get_table_values(ry);
	table_data_copy = malloc(table_size);
	memcpy(table_data_copy, current_table_values, table_size);

	if(any_adjust_applied)
	{
		//copy old values before refresh
		old_table_values = malloc(table_size);
		memcpy(old_table_values, table_data_copy, table_size);

		int errorcode = refresh_table(ry);
		if(errorcode){
			printf("Unable to refresh power metric table: %d\n", errorcode);
		}

		//print table in github markdown
		printf("| Offset |    Data    |   Value   | After Adjust |\n");
		printf("|--------|------------|-----------|--------------|\n");
		char tableFormat[] = "| 0x%04X | 0x%08X | %9.3lf | %12.3lf |\n";
		for(index = 0; index < table_size / 4; index++)
		{
			printf(tableFormat, index * 4, table_data_copy[index], old_table_values[index], current_table_values[index]);
		}

		free(old_table_values);
	}
	else
	{
		//print table in github markdown
		printf("| Offset |    Data    |   Value   |\n");
		printf("|--------|------------|-----------|\n");
		char tableFormat[] = "| 0x%04X | 0x%08X | %9.3lf |\n";
		for(index = 0; index < table_size / 4; index++)
		{
			printf(tableFormat, index * 4, table_data_copy[index], current_table_values[index]);
		}
	}

	free(table_data_copy);
	//don't free current_table_values because this would deinitialize our table
}


int main(int argc, const char **argv)
{
	ryzen_access ry;
	int err = 0, json_exit_code = 0, table_error = 0;
	int json_only = only_json_arguments(argc, argv);

	int info = 0, json = 0, dump_table = 0, any_adjust_applied = 0;
	int power_saving = 0, max_performance = 0, enable_oc = 0x0, disable_oc = 0x0;
	//init unsigned types with max value because we treat max value as unset
	uint32_t stapm_limit = UINT32_MAX, fast_limit = UINT32_MAX, slow_limit = UINT32_MAX, slow_time = UINT32_MAX, stapm_time = UINT32_MAX, tctl_temp = UINT32_MAX;
	uint32_t vrm_current = UINT32_MAX, vrmsoc_current = UINT32_MAX, vrmmax_current = UINT32_MAX, vrmsocmax_current = UINT32_MAX, psi0_current = UINT32_MAX, psi0soc_current = UINT32_MAX;
	uint32_t vrmgfx_current = UINT32_MAX, vrmcvip_current = UINT32_MAX, vrmgfxmax_current = UINT32_MAX, psi3cpu_current = UINT32_MAX, psi3gfx_current = UINT32_MAX;
	uint32_t max_socclk_freq = UINT32_MAX, min_socclk_freq = UINT32_MAX, max_fclk_freq = UINT32_MAX, min_fclk_freq = UINT32_MAX, max_vcn = UINT32_MAX, min_vcn = UINT32_MAX, max_lclk = UINT32_MAX, min_lclk = UINT32_MAX;
	uint32_t max_gfxclk_freq = UINT32_MAX, min_gfxclk_freq = UINT32_MAX, prochot_deassertion_ramp = UINT32_MAX, apu_skin_temp_limit = UINT32_MAX, dgpu_skin_temp_limit = UINT32_MAX, apu_slow_limit = UINT32_MAX;
	uint32_t skin_temp_power_limit = UINT32_MAX;
	uint32_t gfx_clk = UINT32_MAX, oc_clk = UINT32_MAX, oc_volt = UINT32_MAX, coall = UINT32_MAX, coper = UINT32_MAX, cogfx = UINT32_MAX;

	//create structure for parsing
	struct argparse_option options[] = {
		OPT_HELP(),
		OPT_GROUP("Options"),
		OPT_BOOLEAN('i', "info", &info, "Show information and most important power metrics after adjustment"),
		OPT_BOOLEAN('\0', "json", &json, "Format command results as JSON"),
		OPT_BOOLEAN('\0', "dump-table", &dump_table, "Show whole power metric table before and after adjustment"),
		OPT_GROUP("Settings"),
		OPT_U32('a', "stapm-limit", &stapm_limit, "Sustained Power Limit         - STAPM LIMIT (mW)"),
		OPT_U32('b', "fast-limit", &fast_limit, "Actual Power Limit            - PPT LIMIT FAST (mW)"),
		OPT_U32('c', "slow-limit", &slow_limit, "Average Power Limit           - PPT LIMIT SLOW (mW)"),
		OPT_U32('d', "slow-time", &slow_time, "Slow PPT Constant Time (s)"),
		OPT_U32('e', "stapm-time", &stapm_time, "STAPM constant time (s)"),
		OPT_U32('f', "tctl-temp", &tctl_temp, "Tctl Temperature Limit (degree C)"),
		OPT_U32('g', "vrm-current", &vrm_current, "VRM Current Limit             - TDC LIMIT VDD (mA)"),
		OPT_U32('j', "vrmsoc-current", &vrmsoc_current, "VRM SoC Current Limit         - TDC LIMIT SoC (mA)"),
		OPT_U32('\0', "vrmgfx-current", &vrmgfx_current, "VRM GFX Current Limit - TDC LIMIT GFX (mA)"),
		OPT_U32('\0', "vrmcvip-current", &vrmcvip_current, "VRM CVIP Current Limit - TDC LIMIT CVIP (mA)"),
		OPT_U32('k', "vrmmax-current", &vrmmax_current, "VRM Maximum Current Limit     - EDC LIMIT VDD (mA)"),
		OPT_U32('l', "vrmsocmax-current", &vrmsocmax_current, "VRM SoC Maximum Current Limit - EDC LIMIT SoC (mA)"),
		OPT_U32('\0', "vrmgfxmax_current", &vrmgfxmax_current, "VRM GFX Maximum Current Limit - EDC LIMIT GFX (mA)"),
		OPT_U32('m', "psi0-current", &psi0_current, "PSI0 VDD Current Limit (mA)"),
		OPT_U32('\0', "psi3cpu_current", &psi3cpu_current, "PSI3 CPU Current Limit (mA)"),
		OPT_U32('n', "psi0soc-current", &psi0soc_current, "PSI0 SoC Current Limit (mA)"),
		OPT_U32('\0', "psi3gfx_current", &psi3gfx_current, "PSI3 GFX Current Limit (mA)"),
		OPT_U32('o', "max-socclk-frequency", &max_socclk_freq, "Maximum SoC Clock Frequency (MHz)"),
		OPT_U32('p', "min-socclk-frequency", &min_socclk_freq, "Minimum SoC Clock Frequency (MHz)"),
		OPT_U32('q', "max-fclk-frequency", &max_fclk_freq, "Maximum Transmission (CPU-GPU) Frequency (MHz)"),
		OPT_U32('r', "min-fclk-frequency", &min_fclk_freq, "Minimum Transmission (CPU-GPU) Frequency (MHz)"),
		OPT_U32('s', "max-vcn", &max_vcn, "Maximum Video Core Next (VCE - Video Coding Engine) (MHz)"),
		OPT_U32('t', "min-vcn", &min_vcn, "Minimum Video Core Next (VCE - Video Coding Engine) (MHz)"),
		OPT_U32('u', "max-lclk", &max_lclk, "Maximum Data Launch Clock (MHz)"),
		OPT_U32('v', "min-lclk", &min_lclk, "Minimum Data Launch Clock (MHz)"),
		OPT_U32('w', "max-gfxclk", &max_gfxclk_freq, "Maximum GFX Clock (MHz)"),
		OPT_U32('x', "min-gfxclk", &min_gfxclk_freq, "Minimum GFX Clock (MHz)"),
		OPT_U32('y', "prochot-deassertion-ramp", &prochot_deassertion_ramp, "Ramp Time After Prochot is Deasserted: limit power based on value, higher values does apply tighter limits after prochot is over"),
		OPT_U32('\0', "apu-skin-temp", &apu_skin_temp_limit, "APU Skin Temperature Limit    - STT LIMIT APU (degree C)"),
		OPT_U32('\0', "dgpu-skin-temp", &dgpu_skin_temp_limit, "dGPU Skin Temperature Limit   - STT LIMIT dGPU (degree C)"),
		OPT_U32('\0', "apu-slow-limit", &apu_slow_limit, "APU PPT Slow Power limit for A+A dGPU platform - PPT LIMIT APU (mW)"),
		OPT_U32('\0', "skin-temp-limit", &skin_temp_power_limit, "Skin Temperature Power Limit (mW)"),
		OPT_U32('\0', "gfx-clk", &gfx_clk, "Forced Clock Speed MHz (Renoir Only)"),
		OPT_U32('\0', "oc-clk", &oc_clk, "Forced Core Clock Speed MHz (Renoir and up Only)"),
		OPT_U32('\0', "oc-volt", &oc_volt, "Forced Core VID: Must follow this calculation (1.55 - [VID you want to set e.g. 1.25 for 1.25v]) / 0.00625 (Renoir and up Only)"),
		OPT_BOOLEAN('\0', "enable-oc", &enable_oc, "Enable OC (Renoir and up Only)"),
		OPT_BOOLEAN('\0', "disable-oc", &disable_oc, "Disable OC (Renoir and up Only)"),
		OPT_U32('\0', "set-coall", &coall, "All core Curve Optimiser"),
		OPT_U32('\0', "set-coper", &coper, "Per core Curve Optimiser"),
		OPT_U32('\0', "set-cogfx", &cogfx, "iGPU Curve Optimiser"),
		OPT_BOOLEAN('\0', "power-saving", &power_saving, "Hidden options to improve power efficiency (is set when AC unplugged): behavior depends on CPU generation, Device and Manufacture"),
		OPT_BOOLEAN('\0', "max-performance", &max_performance, "Hidden options to improve performance (is set when AC plugged in): behavior depends on CPU generation, Device and Manufacture"),
		OPT_GROUP("P-State Functions"),
		OPT_END(),
	};
	struct ryzenadj_json_adjustment adjustment_results[ARRAY_SIZE(options)] = { 0 };
	struct ryzenadj_json_metric metric_values[ARRAY_SIZE(info_metrics)] = { 0 };
	struct ryzenadj_json_info json_info = { 0 };
	struct ryzenadj_json_error json_error = { 0 };
	struct ryzenadj_json_result result = { 0 };
	struct argparse argparse;

	result.adjustments = adjustment_results;

	argparse_init(&argparse, options, usage, ARGPARSE_NON_OPTION_IS_INVALID);
	argparse_describe(&argparse, "\n Ryzen Power Management adjust tool.", "\nWARNING: Use at your own risk!\nBy Jiaxun Yang <jiaxun.yang@flygoat.com>, Under LGPL.\nVersion: v" STRINGIFY(RYZENADJ_REVISION_VER) "." STRINGIFY(RYZENADJ_MAJOR_VER) "." STRINGIFY(RYZENADJ_MINIOR_VER));
	(void)argparse_parse(&argparse, argc, argv);

	set_ryzenadj_quiet(json != 0);

	if (json && dump_table) {
		set_json_error(&result, &json_error, "arguments", "invalid_arguments", 2);
		ryzenadj_json_print_result(stdout, &result);
		return 2;
	}
	if (json && json_only) {
		set_json_error(&result, &json_error, "arguments", "invalid_arguments", 2);
		ryzenadj_json_print_result(stdout, &result);
		return 2;
	}


	//init RyzenAdj and validate that it was able to
	ry = init_ryzenadj();
	if(!ry){
		if (json) {
			set_json_error(&result, &json_error, "initialization", "failed", -1);
			ryzenadj_json_print_result(stdout, &result);
		} else {
			printf("Unable to init ryzenadj\n");
		}
		return -1;
	}

	//shows info header before init_table
	if (info && !json) {
		show_info_header(ry);
	}

	if (info || dump_table) {
		//init before adjustment to get the default values
		table_error = init_table(ry);
		if (table_error) {
			err = table_error;
			if (json) {
				set_json_error(&result, &json_error, "info_initialization",
					ryzenadj_json_status_from_code(table_error), table_error);
				json_exit_code = table_error;
			} else {
				printf("Unable to init power metric table: %d, this does not affect adjustments because it is only needed for monitoring.\n", table_error);
			}
		}
	}

	//adjust all the arguments sent to RyzenAdj.exe
	_do_adjust(stapm_limit, "stapm-limit");
	_do_adjust(fast_limit, "fast-limit");
	_do_adjust(slow_limit, "slow-limit");
	_do_adjust(slow_time, "slow-time");
	_do_adjust(stapm_time, "stapm-time");
	_do_adjust(tctl_temp, "tctl-temp");
	_do_adjust(vrm_current, "vrm-current");
	_do_adjust(vrmsoc_current, "vrmsoc-current");
	_do_adjust(vrmgfx_current, "vrmgfx-current");
	_do_adjust(vrmcvip_current, "vrmcvip-current");
	_do_adjust(vrmmax_current, "vrmmax-current");
	_do_adjust(vrmsocmax_current, "vrmsocmax-current");
	_do_adjust(vrmgfxmax_current, "vrmgfxmax_current");
	_do_adjust(psi0_current, "psi0-current");
	_do_adjust(psi3cpu_current, "psi3cpu_current");
	_do_adjust(psi0soc_current, "psi0soc-current");
	_do_adjust(psi3gfx_current, "psi3gfx_current");
	_do_adjust(max_socclk_freq, "max-socclk-frequency");
	_do_adjust(min_socclk_freq, "min-socclk-frequency");
	_do_adjust(max_fclk_freq, "max-fclk-frequency");
	_do_adjust(min_fclk_freq, "min-fclk-frequency");
	_do_adjust(max_vcn, "max-vcn");
	_do_adjust(min_vcn, "min-vcn");
	_do_adjust(max_lclk, "max-lclk");
	_do_adjust(min_lclk, "min-lclk");
	_do_adjust(max_gfxclk_freq, "max-gfxclk");
	_do_adjust(min_gfxclk_freq, "min-gfxclk");
	_do_adjust(prochot_deassertion_ramp, "prochot-deassertion-ramp");
	_do_adjust(apu_skin_temp_limit, "apu-skin-temp");
	_do_adjust(dgpu_skin_temp_limit, "dgpu-skin-temp");
	_do_adjust(apu_slow_limit, "apu-slow-limit");
	_do_adjust(skin_temp_power_limit, "skin-temp-limit");
	_do_adjust(gfx_clk, "gfx-clk");
	_do_adjust(oc_clk, "oc-clk");
	_do_adjust(oc_volt, "oc-volt");
	_do_enable(power_saving, "power-saving");
	_do_enable(max_performance, "max-performance");
	_do_enable(enable_oc, "enable-oc");
	_do_enable(disable_oc, "disable-oc");
	_do_adjust(coall, "set-coall");
	_do_adjust(coper, "set-coper");
	_do_adjust(cogfx, "set-cogfx");

	if (!json && !err) {
		//call show table dump before anybody did call table refresh, because we want to copy the old values first
		if (dump_table) {
			show_table_dump(ry, any_adjust_applied);
		}
		//show power table after apply settings
		if (info)
			show_info_table(ry);
	}

	if (json && info && !table_error) {
		int info_error = collect_info_json(ry, &json_info, metric_values,
			ARRAY_SIZE(metric_values));
		if (info_error) {
			set_json_error(&result, &json_error, "info_refresh",
				ryzenadj_json_status_from_code(info_error), info_error);
			json_exit_code = info_error;
		} else {
			result.info = &json_info;
		}
	}

	cleanup_ryzenadj(ry);

	if (json) {
		ryzenadj_json_print_result(stdout, &result);
		return json_exit_code;
	}

	return err;
}
