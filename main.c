// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* Ryzen NB SMU Service Request Tool */

#include <string.h>
#include "lib/ryzenadj.h"
#include "argparse.h"

#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)

#define _do_adjust(ARG) \
do {                                                                              \
	/* ignore max unsigned integer values */                                      \
	if (ARG != -1) {                                                              \
		int adjerr = set_##ARG(ry, ARG);                                          \
		if (!adjerr){                                                             \
			any_adjust_applied = 1;                                               \
			printf("Sucessfully set " STRINGIFY(ARG) " to %u\n", ARG);            \
		} else if (adjerr == ADJ_ERR_FAM_UNSUPPORTED) {                           \
			printf("set_" STRINGIFY(ARG) " is not supported on this family\n");   \
			err = -1;                                                             \
		} else if (adjerr == ADJ_ERR_SMU_UNSUPPORTED) {                           \
			printf("set_" STRINGIFY(ARG) " is not supported on this SMU\n");      \
			err = -1;                                                             \
		} else if (adjerr == ADJ_ERR_SMU_REJECTED) {                              \
			printf("set_" STRINGIFY(ARG) " is rejected by SMU\n");                \
			err = -1;                                                             \
		} else {                                                                  \
			printf("Failed to set" STRINGIFY(ARG) " \n");                         \
			err = -1;                                                             \
		}                                                                         \
	}                                                                             \
} while(0);

#define _do_enable(ARG) \
do {                                                                              \
	if (ARG) {                                                                    \
		int adjerr = set_##ARG(ry);                                               \
		if (!adjerr){                                                             \
			any_adjust_applied = 1;                                               \
			printf("Sucessfully enable " STRINGIFY(ARG) "\n");                    \
		} else if (adjerr == ADJ_ERR_FAM_UNSUPPORTED) {                           \
			printf("set_" STRINGIFY(ARG) " is not supported on this family\n");   \
			err = -1;                                                             \
		} else if (adjerr == ADJ_ERR_SMU_UNSUPPORTED) {                           \
			printf("set_" STRINGIFY(ARG) " is not supported on this SMU\n");      \
			err = -1;                                                             \
		} else if (adjerr == ADJ_ERR_SMU_REJECTED) {                              \
			printf("set_" STRINGIFY(ARG) " is rejected by SMU\n");                \
			err = -1;                                                             \
		} else {                                                                  \
			printf("Failed to set" STRINGIFY(ARG) " \n");                         \
			err = -1;                                                             \
		}                                                                         \
	}                                                                             \
} while(0);

static const char *const usage[] = {
	"ryzenadj [options]",
	NULL,
};

static const char *family_name(enum ryzen_family fam)
{
	switch (fam)
	{
	case FAM_RAVEN: return "Raven";
	case FAM_PICASSO: return "Picasso";
	case FAM_RENOIR: return "Renoir";
	case FAM_CEZANNE: return "Cezanne";
	case FAM_DALI: return "Dali";
	case FAM_LUCIENNE: return "Lucienne";
	case FAM_VANGOGH: return "Vangogh";
	case FAM_REMBRANDT: return "Rembrandt";
	case FAM_PHOENIX: return "Phoenix";
	default:
		break;
	}

	return "Unknown";
}

static void show_info_header(ryzen_access ry)
{
	printf("CPU Family: %s\n", family_name(get_cpu_family(ry)));
	printf("SMU BIOS Interface Version: %d\n", get_bios_if_ver(ry));
	printf("Version: v" STRINGIFY(RYZENADJ_REVISION_VER) "." STRINGIFY(RYZENADJ_MAJOR_VER) "." STRINGIFY(RYZENADJ_MINIOR_VER) " \n");
}

static void show_info_table(ryzen_access ry)
{
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
	printf(tableFormat, "STAPM LIMIT", get_stapm_limit(ry), "stapm-limit");
	printf(tableFormat, "STAPM VALUE", get_stapm_value(ry), "");
	printf(tableFormat, "PPT LIMIT FAST", get_fast_limit(ry), "fast-limit");
	printf(tableFormat, "PPT VALUE FAST", get_fast_value(ry), "");
	printf(tableFormat, "PPT LIMIT SLOW", get_slow_limit(ry), "slow-limit");
	printf(tableFormat, "PPT VALUE SLOW", get_slow_value(ry), "");
	printf(tableFormat, "StapmTimeConst", get_stapm_time(ry), "stapm-time");
	printf(tableFormat, "SlowPPTTimeConst", get_slow_time(ry), "slow-time");
	printf(tableFormat, "PPT LIMIT APU", get_apu_slow_limit(ry), "apu-slow-limit");
	printf(tableFormat, "PPT VALUE APU", get_apu_slow_value(ry), "");
	printf(tableFormat, "TDC LIMIT VDD", get_vrm_current(ry), "vrm-current");
	printf(tableFormat, "TDC VALUE VDD", get_vrm_current_value(ry), "");
	printf(tableFormat, "TDC LIMIT SOC", get_vrmsoc_current(ry), "vrmsoc-current");
	printf(tableFormat, "TDC VALUE SOC", get_vrmsoc_current_value(ry), "");
	printf(tableFormat, "EDC LIMIT VDD", get_vrmmax_current(ry), "vrmmax-current");
	printf(tableFormat, "EDC VALUE VDD", get_vrmmax_current_value(ry), "");
	printf(tableFormat, "EDC LIMIT SOC", get_vrmsocmax_current(ry), "vrmsocmax-current");
	printf(tableFormat, "EDC VALUE SOC", get_vrmsocmax_current_value(ry), "");
	printf(tableFormat, "THM LIMIT CORE", get_tctl_temp(ry), "tctl-temp");
	printf(tableFormat, "THM VALUE CORE", get_tctl_temp_value(ry), "");
	printf(tableFormat, "STT LIMIT APU", get_apu_skin_temp_limit(ry), "apu-skin-temp");
	printf(tableFormat, "STT VALUE APU", get_apu_skin_temp_value(ry), "");
	printf(tableFormat, "STT LIMIT dGPU", get_dgpu_skin_temp_limit(ry), "dgpu-skin-temp");
	printf(tableFormat, "STT VALUE dGPU", get_dgpu_skin_temp_value(ry), "");
	printf(tableFormat, "CCLK Boost SETPOINT", get_cclk_setpoint(ry), "power-saving /");
	printf(tableFormat, "CCLK BUSY VALUE", get_cclk_busy_value(ry), "max-performance");
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
	int err = 0;

	int info = 0, dump_table = 0, any_adjust_applied = 0;
	int power_saving = 0, max_performance = 0, enable_oc = 0x0, disable_oc = 0x0;
	//init unsigned types with max value because we treat max value as unset
	uint32_t stapm_limit = -1, fast_limit = -1, slow_limit = -1, slow_time = -1, stapm_time = -1, tctl_temp = -1;
	uint32_t vrm_current = -1, vrmsoc_current = -1, vrmmax_current = -1, vrmsocmax_current = -1, psi0_current = -1, psi0soc_current = -1;
	uint32_t vrmgfx_current = -1, vrmcvip_current = -1, vrmgfxmax_current = -1, psi3cpu_current = -1, psi3gfx_current = -1;
	uint32_t max_socclk_freq = -1, min_socclk_freq = -1, max_fclk_freq = -1, min_fclk_freq = -1, max_vcn = -1, min_vcn = -1, max_lclk = -1, min_lclk = -1;
	uint32_t max_gfxclk_freq = -1, min_gfxclk_freq = -1, prochot_deassertion_ramp = -1, apu_skin_temp_limit = -1, dgpu_skin_temp_limit = -1, apu_slow_limit = -1;
	uint32_t skin_temp_power_limit = -1;
	uint32_t gfx_clk = -1, oc_clk = -1, oc_volt = -1, coall = -1, coper = -1, cogfx = -1;

	//create structure for parseing
	struct argparse_option options[] = {
		OPT_HELP(),
		OPT_GROUP("Options"),
		OPT_BOOLEAN('i', "info", &info, "Show information and most important power metrics after adjustment"),
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
		OPT_U32('\0', "oc-volt", &oc_volt, "Forced Core VID: Must follow this calcuation (1.55 - [VID you want to set e.g. 1.25 for 1.25v]) / 0.00625 (Renoir and up Only)"),
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


	struct argparse argparse;
	argparse_init(&argparse, options, usage, ARGPARSE_NON_OPTION_IS_INVALID);
	argparse_describe(&argparse, "\n Ryzen Power Management adjust tool.", "\nWARNING: Use at your own risk!\nBy Jiaxun Yang <jiaxun.yang@flygoat.com>, Under LGPL.\nVersion: v" STRINGIFY(RYZENADJ_REVISION_VER) "." STRINGIFY(RYZENADJ_MAJOR_VER) "." STRINGIFY(RYZENADJ_MINIOR_VER));
	argc = argparse_parse(&argparse, argc, argv);


	//init RyzenAdj and validate that it was able to
	ry = init_ryzenadj();
	if(!ry){
		printf("Unable to init ryzenadj\n");
		return -1;
	}

	//shows info header before init_table
	if (info) {
		show_info_header(ry);
	}

	if (info || dump_table) {
		//init before adjustment to get the default values
		err = init_table(ry);
		if (err) {
			printf("Unable to init power metric table: %d, this does not affect adjustments because it is only needed for monitoring.\n", err);
		}
	}

	//adjust all the arguments sent to RyzenAdj.exe
	_do_adjust(stapm_limit);
	_do_adjust(fast_limit);
	_do_adjust(slow_limit);
	_do_adjust(slow_time);
	_do_adjust(stapm_time);
	_do_adjust(tctl_temp);
	_do_adjust(vrm_current);
	_do_adjust(vrmsoc_current);
	_do_adjust(vrmgfx_current);
	_do_adjust(vrmcvip_current);
	_do_adjust(vrmmax_current);
	_do_adjust(vrmsocmax_current);
	_do_adjust(vrmgfxmax_current);
	_do_adjust(psi0_current);
	_do_adjust(psi3cpu_current);
	_do_adjust(psi0soc_current);
	_do_adjust(psi3gfx_current);
	_do_adjust(max_socclk_freq);
	_do_adjust(min_socclk_freq);
	_do_adjust(max_fclk_freq);
	_do_adjust(min_fclk_freq);
	_do_adjust(max_vcn);
	_do_adjust(min_vcn);
	_do_adjust(max_lclk);
	_do_adjust(min_lclk);
	_do_adjust(max_gfxclk_freq);
	_do_adjust(min_gfxclk_freq);
	_do_adjust(prochot_deassertion_ramp);
	_do_adjust(apu_skin_temp_limit);
	_do_adjust(dgpu_skin_temp_limit);
	_do_adjust(apu_slow_limit);
	_do_adjust(skin_temp_power_limit);
	_do_adjust(gfx_clk);
	_do_adjust(oc_clk);
	_do_adjust(oc_volt);
	_do_enable(power_saving);
	_do_enable(max_performance);
	_do_enable(enable_oc)
	_do_enable(disable_oc);
	_do_adjust(coall);
	_do_adjust(coper);
	_do_adjust(cogfx);

	if (!err) {
		//call show table dump before anybody did call table refresh, because we want to copy the old values first
		if (dump_table) {
			show_table_dump(ry, any_adjust_applied);
		}
		//show power table after apply settings
		if (info) {
			show_info_table(ry);
		}
	}

	cleanup_ryzenadj(ry);

	return err;
}
