// SPDX-License-Identifier: LGPL
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
#include <sys/stat.h>

#include "osdep_linux_mem.h"
#include "osdep_linux_smu_kernel_module.h"

bool is_smu = false;
static bool pm_table_mem_active = false;

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
	bool kmod_unusable = false;

	pm_table_mem_active = false;

	if (lstat("/sys/kernel/ryzen_smu_drv", &stats) == 0 && is_ryzen_smu_driver_compatible()) {
		os_access_obj_t *obj;
		fprintf(stderr, "detected compatible ryzen_smu kernel module\n");
		is_smu = true;
		obj = init_os_access_obj_kmod();
		if (obj)
			return obj;
		fprintf(stderr, "compatible ryzen_smu kernel module is unusable, fallback to /dev/mem\n");
		fprintf(stderr, "hint: run as root (sudo) or install ryzen_smu with PM table sysfs support\n");
		is_smu = false;
		kmod_unusable = true;
	}

	if (!kmod_unusable) {
		if (lstat("/sys/module/ryzen_smu", &stats) == 0)
			fprintf(stderr, "incompatible ryzen_smu kernel module loaded: PM table sysfs interface unavailable\n");
		else if (lstat("/sys/kernel/ryzen_smu_drv", &stats) == 0)
			fprintf(stderr, "incompatible ryzen_smu kernel module found, need driver version >= 0.1.7\n");
	}

	if (lstat("/dev/mem", &stats) == -1) {
		if (kmod_unusable)
			fprintf(stderr, "compatible ryzen_smu kernel module is unusable and /dev/mem is unavailable\n");
		else
			fprintf(stderr, "no compatible ryzen_smu kernel module found and /dev/mem is unavailable\n");
	} else if (!kmod_unusable) {
		fprintf(stderr, "no compatible ryzen_smu kernel module found, fallback to /dev/mem\n");
	}
	return init_os_access_obj_mem();
}

bool pm_table_uses_devmem(const os_access_obj_t *obj) {
	return is_smu && obj && !kmod_has_pm_table(obj);
}

int init_mem_obj(os_access_obj_t *os_access, const uintptr_t physAddr) {
	if (pm_table_uses_devmem(os_access)) {
		if (init_mem_obj_mem(os_access, physAddr) < 0) {
			fprintf(stderr, "ryzen_smu PM table sysfs unavailable and /dev/mem fallback failed\n");
			fprintf(stderr, "hint: run as root (sudo), or install ryzen_smu with PM table export\n");
			pm_table_mem_active = false;
			return -1;
		}
		fprintf(stderr, "ryzen_smu PM table sysfs unavailable, using /dev/mem fallback for monitoring\n");
		pm_table_mem_active = true;
		return 0;
	}

	if (is_smu)
		return init_mem_obj_kmod(os_access, physAddr);

	pm_table_mem_active = false;
	return init_mem_obj_mem(os_access, physAddr);
}

void free_os_access_obj(os_access_obj_t *obj) {
	if (pm_table_mem_active) {
		cleanup_mem_pm_table();
		pm_table_mem_active = false;
	}

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
	if (is_smu && kmod_has_pm_table(obj))
		return copy_pm_table_kmod(obj, buffer, size);

	return copy_pm_table_mem(obj, buffer, size);
}

int compare_pm_table(const void *buffer, const size_t size) {
	if (is_smu && !pm_table_mem_active)
		return compare_pm_table_kmod(buffer, size);

	return compare_pm_table_mem(buffer, size);
}

bool is_using_smu_driver() {
	return is_smu;
}

bool kmod_has_pm_table(const os_access_obj_t *obj) {
	return is_smu && obj && obj->access.kmod.has_pm_table;
}

bool kmod_smn_writable(const os_access_obj_t *obj) {
	return is_smu && obj && obj->access.kmod.smn_writable;
}