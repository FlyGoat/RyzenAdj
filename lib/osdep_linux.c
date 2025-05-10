// SPDX-License-Identifier: LGPL
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* Access PCI Config Space - libpci */

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "nb_smu_ops.h"

bool mem_obj_obj = true;
void *phy_map = MAP_FAILED;

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

mem_obj_t init_mem_obj(uintptr_t physAddr)
{
	int dev_mem_fd; 

	//It is to complicated to check PAT, CONFIG_NONPROMISC_DEVMEM, CONFIG_STRICT_DEVMEM or other dependencies, just try to open /dev/mem
	dev_mem_fd = open("/dev/mem", O_RDONLY);
	if (dev_mem_fd > 0){
		phy_map = mmap(NULL, 0x1000, PROT_READ, MAP_SHARED, dev_mem_fd, (long)physAddr);
		close(dev_mem_fd);
	}

	return &mem_obj_obj;
}

void free_mem_obj(mem_obj_t obj)
{
	if(phy_map != MAP_FAILED){
		munmap(phy_map, 0x1000);
	}
	return;
}

int copy_pm_table(nb_t nb, void *buffer, size_t size)
{
	(void)nb;

	if(phy_map != MAP_FAILED){
		memcpy(buffer, phy_map, size);
		return 0;
	}

	printf("failed to get pm_table from /dev/mem\n");
	return -1;
}

int compare_pm_table(void *buffer, size_t size)
{
	return memcmp(buffer, phy_map, size);
}

bool is_using_smu_driver()
{
	return false;
}
