// SPDX-License-Identifier: LGPL
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* Access PCI Config Space - libpci */
#include <sys/mman.h>
#include <pci/pci.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "osdep_linux_mem.h"

void *phy_map = MAP_FAILED;

os_access_obj_t *init_os_access_obj_mem() {
	os_access_obj_t *obj = malloc(sizeof(os_access_obj_t));

	if (obj == NULL)
		return NULL;

	memset(obj, 0, sizeof(os_access_obj_t));

	obj->access.mem.pci_acc = pci_alloc();
	if (!obj->access.mem.pci_acc) {
		fprintf(stderr, "pci_alloc failed\n");
		goto err_exit;
	}

	pci_init(obj->access.mem.pci_acc);

	obj->access.mem.pci_dev = pci_get_dev(obj->access.mem.pci_acc, 0, 0, 0, 0);
	if (!obj->access.mem.pci_dev) {
		fprintf(stderr, "Unable to get pci device\n");
		pci_cleanup(obj->access.mem.pci_acc);
		goto err_exit;
	}

	pci_fill_info(obj->access.mem.pci_dev, PCI_FILL_IDENT | PCI_FILL_BASES | PCI_FILL_CLASS);
	return obj;

err_exit:
	free(obj);
	return NULL;
}

int init_mem_obj_mem([[maybe_unused]] os_access_obj_t *os_access, const uintptr_t physAddr) {
	const int dev_mem_fd = open("/dev/mem", O_RDONLY);

	// It is too complicated to check PAT, CONFIG_NONPROMISC_DEVMEM, CONFIG_STRICT_DEVMEM or other dependencies, just try to open /dev/mem
	if (dev_mem_fd > 0) {
		phy_map = mmap(NULL, 0x1000, PROT_READ, MAP_SHARED, dev_mem_fd, (long)physAddr);
		close(dev_mem_fd);
	}

	return phy_map == MAP_FAILED ? -1 : 0;
}

void free_os_access_obj_mem(os_access_obj_t *obj) {
	if (obj == NULL)
		return;

	if (obj->access.mem.pci_dev)
		pci_free_dev(obj->access.mem.pci_dev);

	if (obj->access.mem.pci_acc)
		pci_cleanup(obj->access.mem.pci_acc);

	if (phy_map != MAP_FAILED) {
		munmap(phy_map, 0x1000);
		phy_map = MAP_FAILED;
	}

	free(obj);
}

uint32_t smn_reg_read_mem(const os_access_obj_t *obj, const uint32_t addr) {
	pci_write_long(obj->access.mem.pci_dev, NB_PCI_REG_ADDR_ADDR, addr & (~0x3));
	return pci_read_long(obj->access.mem.pci_dev, NB_PCI_REG_DATA_ADDR);
}

void smn_reg_write_mem(const os_access_obj_t *obj, const uint32_t addr, const uint32_t data) {
	pci_write_long(obj->access.mem.pci_dev, NB_PCI_REG_ADDR_ADDR, addr);
	pci_write_long(obj->access.mem.pci_dev, NB_PCI_REG_DATA_ADDR, data);
}

int copy_pm_table_mem([[maybe_unused]] const os_access_obj_t *obj, void *buffer, const size_t size) {
	if (phy_map != MAP_FAILED) {
		memcpy(buffer, phy_map, size);
		return 0;
	}

	DBG("failed to get pm_table from /dev/mem\n");
	return -1;
}

int compare_pm_table_mem(const void *buffer, const size_t size) {
	return memcmp(buffer, phy_map, size);
}
