// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* Ryzen NB SMU Service Request Tool */

#include <string.h>
#include <stdlib.h>

#include "lib/ryzenadj.h"
#include "argparse.h"

#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)

#define MAX_INFO_CORES 16

#define _do_adjust(ARG) \
do {                                                                              \
	/* ignore max unsigned integer values */                                      \
	if (ARG != -1) {                                                              \
		int adjerr = set_##ARG(ry, ARG);                                          \
		if (!adjerr){                                                             \
			any_adjust_applied = 1;                                               \
			printf("Successfully set " STRINGIFY(ARG) " to %u\n", ARG);            \
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
			printf("Successfully enable " STRINGIFY(ARG) "\n");                    \
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
	switch (fam) {
		case FAM_RAVEN: return "Raven";
		case FAM_PICASSO: return "Picasso";
		case FAM_RENOIR: return "Renoir";
		case FAM_CEZANNE: return "Cezanne";
		case FAM_DALI: return "Dali";
		case FAM_LUCIENNE: return "Lucienne";
		case FAM_VANGOGH: return "Vangogh";
		case FAM_MATISSE: return "Matisse";
		case FAM_REMBRANDT: return "Rembrandt";
		case FAM_MENDOCINO: return "Mendocino";
		case FAM_PHOENIX: return "Phoenix Point";
		case FAM_HAWKPOINT: return "Hawk Point";
		case FAM_DRAGONRANGE: return "Raphael / Dragon Range";
		case FAM_KRACKANPOINT: return "Krackan Point";
		case FAM_STRIXPOINT: return "Strix Point";
		case FAM_STRIXHALO: return "Strix Halo";
		case FAM_FIRERANGE: return "Granite Ridge / Fire Range";
		default: break;
	}

	return "Unknown";
}

static void print_info_value_if_valid(const char *tableFormat, const char *name, float value, const char *parameter)
{
	if (value == value)
		printf(tableFormat, name, value, parameter);
}

static int info_core_count(ryzen_access ry)
{
	switch (get_table_ver(ry)) {
		case 0x00540004:
		case 0x00620205:
			return 16;
		case 0x00620105:
			return 8;
		case 0x005D0008:
		case 0x005D0009:
		case 0x005D000A:
		case 0x005D000B:
		case 0x00650005:
		case 0x00650007:
			return 12;
		case 0x0064020c:
			return 16;
		default:
			return 8;
	}
}

static int info_core_values_valid(float power, float volt, float temp, float clk)
{
	return power == power && power > 0.0f && power < 300.0f &&
	       volt == volt && volt > 0.0f && volt < 2.0f &&
	       temp == temp && temp > 0.0f && temp < 130.0f &&
	       clk == clk && clk > 0.0f && clk < 10.0f;
}

static void show_info_header(ryzen_access ry)
{
	printf("CPU Family: %s\n", family_name(get_cpu_family(ry)));
	printf("SMU BIOS Interface Version: %d\n", get_bios_if_ver(ry));
	unsigned int smufw = get_smu_version(ry);
	if ((smufw >> 24) & 0xff)
		printf("SMU FW Version: %d.%d.%d.%d\n", (smufw >> 24) & 0xff, (smufw >> 16) & 0xff, (smufw >> 8) & 0xff, smufw & 0xff);
	else
		printf("SMU FW Version: %d.%d.%d\n", (smufw >> 16) & 0xff, (smufw >> 8) & 0xff, smufw & 0xff);
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
	print_info_value_if_valid(tableFormat, "STAPM LIMIT", get_stapm_limit(ry), "stapm-limit");
	print_info_value_if_valid(tableFormat, "STAPM VALUE", get_stapm_value(ry), "");
	print_info_value_if_valid(tableFormat, "PPT LIMIT FAST", get_fast_limit(ry), "fast-limit");
	print_info_value_if_valid(tableFormat, "PPT VALUE FAST", get_fast_value(ry), "");
	print_info_value_if_valid(tableFormat, "PPT LIMIT SLOW", get_slow_limit(ry), "slow-limit");
	print_info_value_if_valid(tableFormat, "PPT VALUE SLOW", get_slow_value(ry), "");
	print_info_value_if_valid(tableFormat, "StapmTimeConst", get_stapm_time(ry), "stapm-time");
	print_info_value_if_valid(tableFormat, "SlowPPTTimeConst", get_slow_time(ry), "slow-time");
	print_info_value_if_valid(tableFormat, "PPT LIMIT APU", get_apu_slow_limit(ry), "apu-slow-limit");
	print_info_value_if_valid(tableFormat, "PPT VALUE APU", get_apu_slow_value(ry), "");
	print_info_value_if_valid(tableFormat, "TDC LIMIT VDD", get_vrm_current(ry), "vrm-current");
	print_info_value_if_valid(tableFormat, "TDC VALUE VDD", get_vrm_current_value(ry), "");
	print_info_value_if_valid(tableFormat, "TDC LIMIT SOC", get_vrmsoc_current(ry), "vrmsoc-current");
	print_info_value_if_valid(tableFormat, "TDC VALUE SOC", get_vrmsoc_current_value(ry), "");
	print_info_value_if_valid(tableFormat, "EDC LIMIT VDD", get_vrmmax_current(ry), "vrmmax-current");
	print_info_value_if_valid(tableFormat, "EDC VALUE VDD", get_vrmmax_current_value(ry), "");
	print_info_value_if_valid(tableFormat, "EDC LIMIT SOC", get_vrmsocmax_current(ry), "vrmsocmax-current");
	print_info_value_if_valid(tableFormat, "EDC VALUE SOC", get_vrmsocmax_current_value(ry), "");
	print_info_value_if_valid(tableFormat, "PSI0 CURRENT", get_psi0_current(ry), "psi0-current");
	print_info_value_if_valid(tableFormat, "PSI0 SOC CURRENT", get_psi0soc_current(ry), "psi0soc-current");
	print_info_value_if_valid(tableFormat, "THM LIMIT CORE", get_tctl_temp(ry), "tctl-temp");
	print_info_value_if_valid(tableFormat, "THM VALUE CORE", get_tctl_temp_value(ry), "");
	print_info_value_if_valid(tableFormat, "STT LIMIT APU", get_apu_skin_temp_limit(ry), "apu-skin-temp");
	print_info_value_if_valid(tableFormat, "STT VALUE APU", get_apu_skin_temp_value(ry), "");
	print_info_value_if_valid(tableFormat, "STT LIMIT dGPU", get_dgpu_skin_temp_limit(ry), "dgpu-skin-temp");
	print_info_value_if_valid(tableFormat, "STT VALUE dGPU", get_dgpu_skin_temp_value(ry), "");
	print_info_value_if_valid(tableFormat, "CCLK Boost SETPOINT", get_cclk_setpoint(ry), "power-saving /");
	print_info_value_if_valid(tableFormat, "CCLK BUSY VALUE", get_cclk_busy_value(ry), "max-performance");
	print_info_value_if_valid(tableFormat, "SOCKET POWER", get_socket_power(ry), "");
	print_info_value_if_valid(tableFormat, "SOC POWER", get_soc_power(ry), "");
	print_info_value_if_valid(tableFormat, "SOC VOLT", get_soc_volt(ry), "");
	print_info_value_if_valid(tableFormat, "GFX POWER", get_gfx_power(ry), "");
	print_info_value_if_valid(tableFormat, "GFX CLK (MHz)", get_gfx_clk(ry), "");
	print_info_value_if_valid(tableFormat, "GFX VOLT", get_gfx_volt(ry), "");
	print_info_value_if_valid(tableFormat, "GFX TEMP", get_gfx_temp(ry), "");
	print_info_value_if_valid(tableFormat, "L3 CLK (MHz)", get_l3_clk(ry), "");
	print_info_value_if_valid(tableFormat, "L3 LOGIC", get_l3_logic(ry), "");
	print_info_value_if_valid(tableFormat, "L3 VDDM", get_l3_vddm(ry), "");
	print_info_value_if_valid(tableFormat, "L3 TEMP", get_l3_temp(ry), "");
	print_info_value_if_valid(tableFormat, "FCLK (MHz)", get_fclk(ry), "");
	float uclk = get_uclk(ry);
	if (uclk == uclk)
		print_info_value_if_valid(tableFormat, "UCLK (MHz)", uclk, "");
	else
		print_info_value_if_valid(tableFormat, "MEM CLK (MHz)", get_mem_clk(ry), "");
	print_info_value_if_valid(tableFormat, "MEM PHY CLK (MHz)", get_mem_phy_clk(ry), "");
	print_info_value_if_valid(tableFormat, "MEM RATE (MT/s)", get_mem_transfer_rate(ry), "");
	print_info_value_if_valid(tableFormat, "VCLK (MHz)", get_vclk(ry), "");
	print_info_value_if_valid(tableFormat, "SOCCLK (MHz)", get_socclk(ry), "");
	print_info_value_if_valid(tableFormat, "MPIPU CLK (MHz)", get_mpipu_clk(ry), "");
	print_info_value_if_valid(tableFormat, "IPU CLK (MHz)", get_ipu_clk(ry), "");

	//per-core detail: power/volt/temp/reported clock plus effective clock and
	//C-state residency (effective clock and residencies are new metrics)
	{
		int core, printed_any = 0, has_freqeff = 0, has_c0 = 0, has_cc1 = 0, has_c6 = 0;
		int core_count = info_core_count(ry);
		float power[MAX_INFO_CORES], volt[MAX_INFO_CORES], temp[MAX_INFO_CORES], clk[MAX_INFO_CORES];
		float freqeff[MAX_INFO_CORES], c0[MAX_INFO_CORES], cc1[MAX_INFO_CORES], c6[MAX_INFO_CORES];

		for (core = 0; core < core_count; core++) {
			power[core] = get_core_power(ry, core);
			if (!(power[core] > 0.0f))
				continue; //skip disabled/absent cores (0W) and unsupported tables (nan)
			volt[core] = get_core_volt(ry, core);
			temp[core] = get_core_temp(ry, core);
			clk[core] = get_core_clk(ry, core);
			freqeff[core] = get_core_freqeff(ry, core);
			c0[core] = get_core_c0(ry, core);
			cc1[core] = get_core_cc1(ry, core);
			c6[core] = get_core_cc6(ry, core);
			if (!info_core_values_valid(power[core], volt[core], temp[core], clk[core]))
				continue;
			has_freqeff |= (freqeff[core] == freqeff[core]);
			has_c0 |= (c0[core] == c0[core]);
			has_cc1 |= (cc1[core] == cc1[core]);
			has_c6 |= (c6[core] == c6[core]);
			printed_any = 1;
		}
		if (printed_any) {
			printf("\n| Core | Power(W) | Volt(V) | Temp(C) | Freq(GHz) |");
			if (has_freqeff) printf(" FreqEff |");
			if (has_c0) printf(" C0%%  |");
			if (has_cc1) printf(" CC1%% |");
			if (has_c6) printf(" C6%%  |");
			printf("\n|------|----------|---------|---------|-----------|");
			if (has_freqeff) printf("---------|");
			if (has_c0) printf("------|");
			if (has_cc1) printf("------|");
			if (has_c6) printf("------|");
			printf("\n");

			for (core = 0; core < core_count; core++) {
				if (!(power[core] > 0.0f))
					continue;
				if (!info_core_values_valid(power[core], volt[core], temp[core], clk[core]))
					continue;
				printf("| %4d | %8.3f | %7.3f | %7.1f | %9.3f |",
					core, power[core], volt[core], temp[core], clk[core]);
				if (has_freqeff) printf(" %7.3f |", freqeff[core]);
				if (has_c0) printf(" %4.0f |", c0[core]);
				if (has_cc1) printf(" %4.0f |", cc1[core]);
				if (has_c6) printf(" %4.0f |", c6[core]);
				printf("\n");
			}
		}
	}
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
	uint32_t gfx_clk = -1, oc_clk = -1, per_core_oc_clk = -1, oc_volt = -1, coall = -1, coper = -1, cogfx = -1;

	//create structure for parsing
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
		OPT_U32('\0', "vrmgfxmax-current", &vrmgfxmax_current, "VRM GFX Maximum Current Limit - EDC LIMIT GFX (mA)"),
		OPT_U32('m', "psi0-current", &psi0_current, "PSI0 VDD Current Limit (mA)"),
		OPT_U32('\0', "psi3cpu-current", &psi3cpu_current, "PSI3 CPU Current Limit (mA)"),
		OPT_U32('n', "psi0soc-current", &psi0soc_current, "PSI0 SoC Current Limit (mA)"),
		OPT_U32('\0', "psi3gfx-current", &psi3gfx_current, "PSI3 GFX Current Limit (mA)"),
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
		OPT_U32('\0', "oc-clk-per-core", &per_core_oc_clk, "Forced Per Core Clock Speed MHz (Renoir, Cezanne, Rembrandt Only)"),
		OPT_U32('\0', "oc-volt", &oc_volt, "Forced Core VID: Must follow this calculation (1.55 - [VID you want to set e.g. 1.25 for 1.25v]) / 0.00625 (Renoir and up Only)"),
		OPT_BOOLEAN('\0', "enable-oc", &enable_oc, "Enable OC (Renoir and up Only)"),
		OPT_BOOLEAN('\0', "disable-oc", &disable_oc, "Disable OC (Renoir and up Only)"),
		OPT_U32('\0', "set-coall", &coall, "All core Curve Optimiser"),
		OPT_U32('\0', "set-coper", &coper, "Per core Curve Optimiser"),
		OPT_U32('\0', "set-cogfx", &cogfx, "iGPU Curve Optimiser"),
		OPT_BOOLEAN('\0', "power-saving", &power_saving, "Hidden options to improve power efficiency (is set when AC unplugged): behavior depends on CPU generation, Device and Manufacture"),
		OPT_BOOLEAN('\0', "max-performance", &max_performance, "Hidden options to improve performance (is set when AC plugged in): behavior depends on CPU generation, Device and Manufacture"),
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
	_do_adjust(per_core_oc_clk);
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
