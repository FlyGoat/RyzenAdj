// SPDX-License-Identifier: LGPL
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* Access PCI Config Space - libpci */

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "nb_smu_ops.h"

bool mem_obj_obj = true;
int pm_table_fd = 0;
int dev_mem_fd = 0;

pci_obj_t init_pci_obj(){
	pci_obj_t obj;
	obj = pci_alloc();
	pci_init(obj);
	return obj;
}

nb_t get_nb(pci_obj_t obj){
	nb_t nb;
	nb = pci_get_dev(obj, 0, 0, 0, 0);
	pci_fill_info(nb, PCI_FILL_IDENT | PCI_FILL_BASES | PCI_FILL_CLASS);
	return nb;
}

void free_nb(nb_t nb){
	pci_free_dev(nb);
}


void free_pci_obj(pci_obj_t obj){
	pci_cleanup(obj);
}

u32 smn_reg_read(nb_t nb, u32 addr)
{
	pci_write_long(nb, NB_PCI_REG_ADDR_ADDR, (addr & (~0x3)));
	return pci_read_long(nb, NB_PCI_REG_DATA_ADDR);
}

void smn_reg_write(nb_t nb, u32 addr, u32 data)
{
	pci_write_long(nb, NB_PCI_REG_ADDR_ADDR, addr);
	pci_write_long(nb, NB_PCI_REG_DATA_ADDR, data);
}

mem_obj_t init_mem_obj()
{
	int dev_mem_errno, pm_table_errno;

	//It is to complicated to check PAT, CONFIG_NONPROMISC_DEVMEM, CONFIG_STRICT_DEVMEM or other dependencies, just try to open /dev/mem and try to use it later
	dev_mem_fd = open("/dev/mem", O_RDONLY);
	if (dev_mem_fd == -1){
		dev_mem_errno = errno;
	}

	pm_table_fd = open("/sys/kernel/ryzen_smu_drv/pm_table", O_RDONLY);
	if (pm_table_fd == -1){
		pm_table_errno = errno;
	}

	if(dev_mem_fd == -1 && dev_mem_fd -1){
		printf("failed to get /dev/mem: %s\n", strerror(dev_mem_errno));
		printf("failed to get /sys/kernel/ryzen_smu_drv/pm_table: %s\n", strerror(pm_table_errno));
		printf("If you don't want to change your memory access policy, you need a kernel module for this task.\n");
		printf("We do support usage of this kernel module https://gitlab.com/leogx9r/ryzen_smu\n");
		return NULL;
	}

	return &mem_obj_obj;
}

void free_mem_obj(mem_obj_t obj)
{
	if(dev_mem_fd > 0) {
		close(dev_mem_fd);
	}
	if(pm_table_fd > 0) {
		close(pm_table_fd);
	}
	return;
}

int copy_from_phyaddr(u32 physAddr, void *buffer, size_t size)
{
	void *phy_map;
	int read_size, last_errno;

	//use ryzen_smu kernel modul for pm table if existing
	if(pm_table_fd > 0){
		lseek(pm_table_fd, 0, SEEK_SET);
		read_size = read(pm_table_fd, buffer, size);
		if(read_size == -1) {
			printf("failed to get pm_table from ryzen_smu kernel module: %s\n", strerror(errno));
		} else {
			return 0;
		}
	}

	//use /dev/mem if ryzen_smu kernel module is not installed or if it did fail
	if(dev_mem_fd > 0) {
		phy_map = mmap(NULL, size, PROT_READ, MAP_SHARED, dev_mem_fd, physAddr);
		if (phy_map == MAP_FAILED) {
			last_errno = errno;
			printf("failed to map /dev/mem: %s\n", strerror(last_errno));
			if(last_errno == EPERM) {
				//we are already superuser if memory access is requested because we did successfully do the other smu calls
				printf("If you don't want to change your memory access policy, you need a kernel module for this task.\n");
				printf("We do support usage of this kernel module https://gitlab.com/leogx9r/ryzen_smu\n");
			}
		} else {
			memcpy(buffer, phy_map, size);
			munmap(phy_map, size);
			return 0;
		}
	}

	return -1;
}
