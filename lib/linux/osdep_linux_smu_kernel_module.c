// SPDX-License-Identifier: LGPL
/* Backend which uses the sysfs interface provided by the ryzen_smu kernel module. */
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "osdep_linux_smu_kernel_module.h"

static uint32_t get_pm_table_size() {
	const int fd = open("/sys/kernel/ryzen_smu_drv/pm_table_size", O_RDONLY);
	uint32_t table_sz = 0;

	if (fd == -1)
		return -1;

	if (read(fd, &table_sz, sizeof(table_sz)) == -1) {
		DBG("failed to retrieve PM table size: %s\n", strerror(errno));
		close(fd);
		return -1;
	}

	close(fd);
	return table_sz;
}

os_access_obj_t *init_os_access_obj_kmod() {
	os_access_obj_t *obj = malloc(sizeof(os_access_obj_t));

	if (obj == NULL)
		return NULL;

	memset(obj, 0, sizeof(os_access_obj_t));

	obj->access.kmod.pm_table_size = get_pm_table_size();
	if (obj->access.kmod.pm_table_size == -1)
		goto err_exit;

	obj->access.kmod.smn_fd = open("/sys/kernel/ryzen_smu_drv/smn", O_RDWR);
	if (obj->access.kmod.smn_fd == -1) {
		DBG("failed to open smn fd: %s\n", strerror(errno));
		goto err_exit;
	}

	obj->access.kmod.pm_table_fd = open("/sys/kernel/ryzen_smu_drv/pm_table", O_RDONLY);
	if (obj->access.kmod.pm_table_fd == -1) {
		DBG("failed to open pm_table fd: %s\n", strerror(errno));
		close(obj->access.kmod.smn_fd);
		goto err_exit;
	}

	return obj;

err_exit:
	free(obj);
	return NULL;
}

int init_mem_obj_kmod([[maybe_unused]] os_access_obj_t *os_access, [[maybe_unused]] const uintptr_t physAddr) {
	return 0;
}

void free_os_access_obj_kmod(os_access_obj_t *obj) {
	close(obj->access.kmod.smn_fd);
	close(obj->access.kmod.pm_table_fd);
	free(obj);
}

uint32_t smn_reg_read_kmod(const os_access_obj_t *obj, const uint32_t addr) {
	uint32_t result = 0;

	lseek(obj->access.kmod.smn_fd, 0, SEEK_SET);

	if (write(obj->access.kmod.smn_fd, &addr, sizeof(addr)) == -1) {
		DBG("%s: write error: %s\n", __func__, strerror(errno));
		return 0;
	}

	lseek(obj->access.kmod.smn_fd, 0, SEEK_SET);

	if (read(obj->access.kmod.smn_fd, &result, sizeof(result)) == -1) {
		DBG("%s: read error: %s\n", __func__, strerror(errno));
		return 0;
	}

	return result;
}

void smn_reg_write_kmod(const os_access_obj_t *obj, const uint32_t addr, const uint32_t data) {
	const uint32_t write_buffer[2] = { addr, data };

	lseek(obj->access.kmod.smn_fd, 0, SEEK_SET);

	if (write(obj->access.kmod.smn_fd, &write_buffer, sizeof(write_buffer)) == -1)
		DBG("%s: error: %s\n", __func__, strerror(errno));
}

int copy_pm_table_kmod(const os_access_obj_t *obj, void *buffer, const size_t size) {
	if (obj->access.kmod.pm_table_size != size) {
		DBG("PM table size mismatch: ryzenadj (%zd) | ryzen_smu (%zd)\n", size, obj->access.kmod.pm_table_size);
		return -1;
	}

	lseek(obj->access.kmod.pm_table_fd, 0, SEEK_SET);

	if (read(obj->access.kmod.pm_table_fd, buffer, size) == -1) {
		DBG("%s: error: %s\n", __func__, strerror(errno));
		return -1;
	}

	return 0;
}

int compare_pm_table_kmod([[maybe_unused]] const void *buffer, [[maybe_unused]] size_t size) {
	DBG("internal error: compare_pm_table() should never be called if ryzen_smu is loaded\n");
	return -1;
}
