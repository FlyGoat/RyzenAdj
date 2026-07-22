// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2026 Piotr Karbowski */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../json_output.h"
#include "../lib/ryzenadj.h"

static char *serialize(const struct ryzenadj_json_result *result)
{
	FILE *stream = tmpfile();
	char *output;
	long length;

	if (!stream)
		return NULL;

	ryzenadj_json_print_result(stream, result);
	if (fflush(stream) || fseek(stream, 0, SEEK_END)) {
		fclose(stream);
		return NULL;
	}
	length = ftell(stream);
	if (length < 0 || fseek(stream, 0, SEEK_SET)) {
		fclose(stream);
		return NULL;
	}

	output = malloc((size_t)length + 1);
	if (!output) {
		fclose(stream);
		return NULL;
	}
	if (fread(output, 1, (size_t)length, stream) != (size_t)length) {
		free(output);
		fclose(stream);
		return NULL;
	}
	output[length] = '\0';
	fclose(stream);
	return output;
}

static int expect_equal(const char *name, const char *actual,
	const char *expected)
{
	if (strcmp(actual, expected) == 0)
		return 0;

	fprintf(stderr, "%s: expected output:\n%sactual output:\n%s",
		name, expected, actual);
	return 1;
}

static int expect_contains(const char *name, const char *output,
	const char *expected)
{
	if (strstr(output, expected))
		return 0;

	fprintf(stderr, "%s: expected output to contain:\n%s\nactual output:\n%s",
		name, expected, output);
	return 1;
}

static int test_success(void)
{
	static const char expected[] =
		"{\n"
		"  \"success\": true,\n"
		"  \"adjustments\": [\n"
		"    {\"option\": \"fast-limit\", \"value\": 25000, \"status\": \"success\", \"code\": 0}\n"
		"  ],\n"
		"  \"info\": null,\n"
		"  \"error\": null\n"
		"}\n";
	struct ryzenadj_json_adjustment adjustments[] = {
		{ "fast-limit", RYZENADJ_JSON_VALUE_U32, 25000, 0 },
	};
	struct ryzenadj_json_result result = {
		adjustments, 1, NULL, NULL,
	};
	char *output = serialize(&result);
	int failed;

	if (!output)
		return 1;
	failed = expect_equal("success", output, expected);
	free(output);
	return failed;
}

static int test_adjustment_failure(void)
{
	static const char expected[] =
		"{\n"
		"  \"success\": false,\n"
		"  \"adjustments\": [\n"
		"    {\"option\": \"stapm-limit\", \"value\": 25000, \"status\": \"success\", \"code\": 0},\n"
		"    {\"option\": \"power-saving\", \"value\": true, \"status\": \"smu_rejected\", \"code\": -4}\n"
		"  ],\n"
		"  \"info\": null,\n"
		"  \"error\": null\n"
		"}\n";
	struct ryzenadj_json_adjustment adjustments[] = {
		{ "stapm-limit", RYZENADJ_JSON_VALUE_U32, 25000, 0 },
		{ "power-saving", RYZENADJ_JSON_VALUE_BOOLEAN, 1,
			ADJ_ERR_SMU_REJECTED },
	};
	struct ryzenadj_json_result result = {
		adjustments, 2, NULL, NULL,
	};
	char *output = serialize(&result);
	int failed;

	if (!output)
		return 1;
	failed = expect_equal("adjustment failure", output, expected);
	free(output);
	return failed;
}

static int test_info(void)
{
	struct ryzenadj_json_metric metrics[] = {
		{ "fast_limit_w", 25.0f },
		{ "unsupported", NAN },
	};
	struct ryzenadj_json_info info = {
		"Test \"Family\"", 21, "0.19.0", 0x5d0009, metrics, 2,
	};
	struct ryzenadj_json_result result = {
		NULL, 0, &info, NULL,
	};
	char *output = serialize(&result);
	int failed = 0;

	if (!output)
		return 1;
	failed |= expect_contains("info", output, "\"success\": true");
	failed |= expect_contains("info", output,
		"\"cpu_family\": \"Test \\\"Family\\\"\"");
	failed |= expect_contains("info", output,
		"\"pm_table_version\": \"0x5d0009\"");
	failed |= expect_contains("info", output, "\"fast_limit_w\": 25.000");
	failed |= expect_contains("info", output, "\"unsupported\": null");
	failed |= expect_contains("info", output, "\"error\": null");
	free(output);
	return failed;
}

static int test_pipeline_error(const char *stage, const char *status, int code)
{
	struct ryzenadj_json_error error = { stage, status, code };
	struct ryzenadj_json_result result = { NULL, 0, NULL, &error };
	char expected[256];
	char *output = serialize(&result);
	int failed = 0;

	if (!output)
		return 1;
	failed |= expect_contains("pipeline error", output, "\"success\": false");
	failed |= expect_contains("pipeline error", output, "\"info\": null");
	snprintf(expected, sizeof(expected),
		"{\"stage\": \"%s\", \"status\": \"%s\", \"code\": %d}",
		stage, status, code);
	failed |= expect_contains("pipeline error", output, expected);
	free(output);
	return failed;
}

static int test_statuses(void)
{
	return strcmp(ryzenadj_json_status_from_code(0), "success") != 0 ||
		strcmp(ryzenadj_json_status_from_code(ADJ_ERR_FAM_UNSUPPORTED),
			"family_unsupported") != 0 ||
		strcmp(ryzenadj_json_status_from_code(ADJ_ERR_SMU_TIMEOUT),
			"smu_timeout") != 0 ||
		strcmp(ryzenadj_json_status_from_code(ADJ_ERR_SMU_UNSUPPORTED),
			"smu_unsupported") != 0 ||
		strcmp(ryzenadj_json_status_from_code(ADJ_ERR_SMU_REJECTED),
			"smu_rejected") != 0 ||
		strcmp(ryzenadj_json_status_from_code(ADJ_ERR_MEMORY_ACCESS),
			"memory_access_error") != 0 ||
		strcmp(ryzenadj_json_status_from_code(-100), "failed") != 0;
}

int main(void)
{
	int failed = 0;

	failed |= test_success();
	failed |= test_adjustment_failure();
	failed |= test_info();
	failed |= test_pipeline_error("initialization", "failed", -1);
	failed |= test_pipeline_error("info_initialization", "family_unsupported",
		ADJ_ERR_FAM_UNSUPPORTED);
	failed |= test_pipeline_error("info_refresh", "memory_access_error",
		ADJ_ERR_MEMORY_ACCESS);
	failed |= test_statuses();
	return failed;
}
