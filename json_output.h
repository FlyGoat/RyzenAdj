// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2026 Piotr Karbowski */

#ifndef RYZENADJ_JSON_OUTPUT_H
#define RYZENADJ_JSON_OUTPUT_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

enum ryzenadj_json_value_type {
	RYZENADJ_JSON_VALUE_U32,
	RYZENADJ_JSON_VALUE_BOOLEAN,
};

struct ryzenadj_json_adjustment {
	const char *option;
	enum ryzenadj_json_value_type value_type;
	uint32_t value;
	int code;
};

struct ryzenadj_json_error {
	const char *stage;
	const char *status;
	int code;
};

struct ryzenadj_json_metric {
	const char *key;
	float value;
};

struct ryzenadj_json_info {
	const char *cpu_family;
	int smu_bios_interface_version;
	const char *ryzenadj_version;
	uint32_t pm_table_version;
	const struct ryzenadj_json_metric *metrics;
	size_t metric_count;
};

struct ryzenadj_json_result {
	const struct ryzenadj_json_adjustment *adjustments;
	size_t adjustment_count;
	const struct ryzenadj_json_info *info;
	const struct ryzenadj_json_error *error;
};

const char *ryzenadj_json_status_from_code(int code);
void ryzenadj_json_print_result(FILE *stream,
	const struct ryzenadj_json_result *result);

#endif
