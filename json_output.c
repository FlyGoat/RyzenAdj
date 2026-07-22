// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2026 Piotr Karbowski */

#include <math.h>

#include "json_output.h"
#include "lib/ryzenadj.h"

const char *ryzenadj_json_status_from_code(int code)
{
	switch (code) {
		case 0: return "success";
		case ADJ_ERR_FAM_UNSUPPORTED: return "family_unsupported";
		case ADJ_ERR_SMU_TIMEOUT: return "smu_timeout";
		case ADJ_ERR_SMU_UNSUPPORTED: return "smu_unsupported";
		case ADJ_ERR_SMU_REJECTED: return "smu_rejected";
		case ADJ_ERR_MEMORY_ACCESS: return "memory_access_error";
		default: return "failed";
	}
}

static void print_string(FILE *stream, const char *value)
{
	const unsigned char *character = (const unsigned char *)value;

	fputc('"', stream);
	while (*character) {
		switch (*character) {
			case '"': fputs("\\\"", stream); break;
			case '\\': fputs("\\\\", stream); break;
			case '\b': fputs("\\b", stream); break;
			case '\f': fputs("\\f", stream); break;
			case '\n': fputs("\\n", stream); break;
			case '\r': fputs("\\r", stream); break;
			case '\t': fputs("\\t", stream); break;
			default:
				if (*character < 0x20) {
					fprintf(stream, "\\u%04x", (unsigned int)*character);
				} else {
					fputc(*character, stream);
				}
		}
		character++;
	}
	fputc('"', stream);
}

static void print_float(FILE *stream, float value)
{
	if (isfinite(value))
		fprintf(stream, "%.3f", value);
	else
		fputs("null", stream);
}

static int result_succeeded(const struct ryzenadj_json_result *result)
{
	size_t index;

	if (result->error)
		return 0;

	for (index = 0; index < result->adjustment_count; index++) {
		if (result->adjustments[index].code)
			return 0;
	}

	return 1;
}

static void print_adjustments(FILE *stream,
	const struct ryzenadj_json_result *result)
{
	size_t index;

	fputs("  \"adjustments\": [", stream);
	if (result->adjustment_count)
		fputc('\n', stream);

	for (index = 0; index < result->adjustment_count; index++) {
		const struct ryzenadj_json_adjustment *adjustment =
			&result->adjustments[index];

		fputs("    {\"option\": ", stream);
		print_string(stream, adjustment->option);
		fputs(", \"value\": ", stream);
		if (adjustment->value_type == RYZENADJ_JSON_VALUE_BOOLEAN)
			fputs(adjustment->value ? "true" : "false", stream);
		else
			fprintf(stream, "%u", (unsigned int)adjustment->value);
		fputs(", \"status\": ", stream);
		print_string(stream, ryzenadj_json_status_from_code(adjustment->code));
		fprintf(stream, ", \"code\": %d}%s\n", adjustment->code,
			index + 1 < result->adjustment_count ? "," : "");
	}

	if (result->adjustment_count)
		fputs("  ", stream);
	fputs("],\n", stream);
}

static void print_info(FILE *stream, const struct ryzenadj_json_info *info)
{
	size_t index;

	fputs("  \"info\": ", stream);
	if (!info) {
		fputs("null,\n", stream);
		return;
	}

	fputs("{\n    \"cpu_family\": ", stream);
	print_string(stream, info->cpu_family);
	fprintf(stream, ",\n    \"smu_bios_interface_version\": %d,\n",
		info->smu_bios_interface_version);
	fputs("    \"ryzenadj_version\": ", stream);
	print_string(stream, info->ryzenadj_version);
	fprintf(stream, ",\n    \"pm_table_version\": \"0x%x\",\n",
		(unsigned int)info->pm_table_version);
	fputs("    \"metrics\": {", stream);
	if (info->metric_count)
		fputc('\n', stream);

	for (index = 0; index < info->metric_count; index++) {
		fputs("      ", stream);
		print_string(stream, info->metrics[index].key);
		fputs(": ", stream);
		print_float(stream, info->metrics[index].value);
		fprintf(stream, "%s\n", index + 1 < info->metric_count ? "," : "");
	}

	if (info->metric_count)
		fputs("    ", stream);
	fputs("}\n  },\n", stream);
}

static void print_error(FILE *stream, const struct ryzenadj_json_error *error)
{
	fputs("  \"error\": ", stream);
	if (!error) {
		fputs("null\n", stream);
		return;
	}

	fputs("{\"stage\": ", stream);
	print_string(stream, error->stage);
	fputs(", \"status\": ", stream);
	print_string(stream, error->status);
	fprintf(stream, ", \"code\": %d}\n", error->code);
}

void ryzenadj_json_print_result(FILE *stream,
	const struct ryzenadj_json_result *result)
{
	fputs("{\n  \"success\": ", stream);
	fputs(result_succeeded(result) ? "true,\n" : "false,\n", stream);
	print_adjustments(stream, result);
	print_info(stream, result->info);
	print_error(stream, result->error);
	fputs("}\n", stream);
}
