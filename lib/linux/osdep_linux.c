// SPDX-License-Identifier: LGPL
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
#include <sys/stat.h>

#include "osdep_linux_mem.h"
#include "osdep_linux_smu_kernel_module.h"

bool is_smu = false;

static bool is_ryzen_smu_driver_compatible() {
	FILE *drv_ver = fopen("/sys/kernel/ryzen_smu_drv/drv_version", "r");
	int major, minor, patch, ret;

	if (drv_ver == NULL) {
		DBG("failed to open drv_version");
		return false;
	}

	ret = fscanf(drv_ver, "%d.%d.%d", &major, &minor, &patch);
	if (ret == EOF || ret < 3) {
		DBG("failed to parse ryzen_smu version string\n");
		fclose(drv_ver);
		return false;
	}

	if (major != 0 || minor != 1 || patch < 7) {
		fclose(drv_ver);
		return false;
	}

	fclose(drv_ver);
	return true;
}

os_access_obj_t *init_os_access_obj() {
	struct stat stats;

	if (lstat("/sys/kernel/ryzen_smu_drv", &stats) == 0 && is_ryzen_smu_driver_compatible()) {
		fprintf(stderr, "detected compatible ryzen_smu kernel module\n");
		is_smu = true;
		return init_os_access_obj_kmod();
	}

	fprintf(stderr, "no compatible ryzen_smu kernel module found, fallback to /dev/mem\n");
	return init_os_access_obj_mem();
}

int init_mem_obj(os_access_obj_t *os_access, const uintptr_t physAddr) {
	if (is_smu)
		return init_mem_obj_kmod(os_access, physAddr);

	return init_mem_obj_mem(os_access, physAddr);
}

void free_os_access_obj(os_access_obj_t *obj) {
	if (is_smu)
		free_os_access_obj_kmod(obj);
	else
		free_os_access_obj_mem(obj);

	is_smu = false;
}

uint32_t smn_reg_read(const os_access_obj_t *obj, const uint32_t addr) {
	if (is_smu)
		return smn_reg_read_kmod(obj, addr);

	return smn_reg_read_mem(obj, addr);
}

void smn_reg_write(const os_access_obj_t *obj, const uint32_t addr, const uint32_t data) {
	if (is_smu)
		smn_reg_write_kmod(obj, addr, data);
	else
		smn_reg_write_mem(obj, addr, data);
}

int copy_pm_table(const os_access_obj_t *obj, void *buffer, const size_t size) {
	if (is_smu)
		return copy_pm_table_kmod(obj, buffer, size);

	return copy_pm_table_mem(obj, buffer, size);
}

int compare_pm_table(const void *buffer, const size_t size) {
	if (is_smu)
		return compare_pm_table_kmod(buffer, size);

	return compare_pm_table_mem(buffer, size);
}

bool is_using_smu_driver() {
	return is_smu;
}
