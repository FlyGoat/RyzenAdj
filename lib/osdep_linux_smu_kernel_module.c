/* Backend which uses the sysfs interface provided by the ryzen_smu kernel module. */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>

#include "nb_smu_ops.h"

typedef struct
{
	FILE *file;
	const char *static_file_path;
} file_handle_t;

struct smu_kernel_module {
	file_handle_t smn;
	file_handle_t pm_table;
	size_t pm_table_size;
};

static bool path_exists(const char *file_path)
{
	const int old_errno = errno;
	errno = 0;
	struct stat stats;
	const bool path_exists = lstat(file_path, &stats) == 0;
	if(!path_exists && errno != 0 && errno != ENOENT)
	{
		fprintf(stderr, "failed to check existence of path: %s\n", file_path);
	}
	errno = old_errno;
	return path_exists;
}

static file_handle_t open_file(const char *static_file_path, const char *mode)
{
	file_handle_t result = {
		.file = fopen(static_file_path, mode),
		.static_file_path = static_file_path
	};
	if(result.file == NULL)
	{
		fprintf(stderr, "failed to open file: '%s': %s\n",
			static_file_path, strerror(errno));
	}
	return result;
}

static bool close_file(file_handle_t file)
{
	if(fclose(file.file) != 0)
	{
		fprintf(stderr, "failed to close file: '%s': %s\n",
			file.static_file_path, strerror(errno));
		return false;
	}
	return true;
}

static bool read_from_start_up_to_bytes(file_handle_t input, void *out,
					const size_t out_size,
					size_t *out_bytes_read)
{
	fseek(input.file, 0, SEEK_SET);
	const size_t bytes_read = fread(out, 1, out_size, input.file);
	if(ferror(input.file))
	{
		fprintf(stderr, "failed to read from file: '%s': %s\n",
			input.static_file_path, strerror(errno));
		return false;
	}
	*out_bytes_read = bytes_read;
	return true;
}

static bool read_from_start(file_handle_t input, void *out, const size_t out_size)
{
	size_t bytes_read = 0;
	if(!read_from_start_up_to_bytes(input, out, out_size, &bytes_read))
	{
		return false;
	}
	if(bytes_read != out_size)
	{
		fprintf(stderr, "failed to read complete data from file: '%s'\n",
			input.static_file_path);
		return false;
	}
	return true;
}

static bool write_to_start(file_handle_t output, const void *data, const size_t data_size)
{
	fseek(output.file, 0, SEEK_SET);
	const size_t bytes_written = fwrite(data, 1, data_size, output.file);
	if(ferror(output.file))
	{
		fprintf(stderr, "failed to write to file: '%s': %s\n",
			output.static_file_path, strerror(errno));
		return false;
	}
	if(bytes_written != data_size)
	{
		fprintf(stderr, "failed to write complete data to file: '%s'\n",
			output.static_file_path);
		return false;
	}
	return true;
}

static bool driver_compatible(void)
{
	file_handle_t input = open_file("/sys/kernel/ryzen_smu_drv/drv_version", "rb");
	if(input.file == NULL)
	{
		return false;
	}

	size_t bytes_read = 0;
	char version[32] = {0};
	const bool success =
		read_from_start_up_to_bytes(input, version, sizeof(version), &bytes_read);
	(void)close_file(input);
	if(!success)
	{
		return false;
	}

	version[bytes_read] = '\0';
	if(strstr(version, "0.") != version)
	{
		fprintf(stderr, "incompatible ryzen_smu module major version: %s\n", version);
		return false;
	}
	return true;
}

static bool read_pm_table_size(u32 *out)
{
	file_handle_t input = open_file("/sys/kernel/ryzen_smu_drv/pm_table_size", "rb");
	if(input.file == NULL)
	{
		return false;
	}
	const bool success = read_from_start(input, out, sizeof(*out));
	(void)close_file(input);
	return success;
}

pci_obj_t init_pci_obj(void)
{
	if(!path_exists("/sys/kernel/ryzen_smu_drv"))
	{
		return NULL;
	}
	printf("detected ryzen_smu kernel module, initializing...\n");

	if(!driver_compatible())
	{
		return NULL;
	}

	u32 pm_table_size = 0;
	if(!read_pm_table_size(&pm_table_size))
	{
		fprintf(stderr, "failed to retrieve PM table size from ryzen_smu kernel module\n");
		return NULL;
	}

	smu_kernel_module_t *result = malloc(sizeof *result);
	if(result == NULL)
	{
		return NULL;
	}
	result->smn = open_file("/sys/kernel/ryzen_smu_drv/smn", "r+b");
	if(result->smn.file == NULL)
	{
		free(result);
		return NULL;
	}
	result->pm_table = open_file("/sys/kernel/ryzen_smu_drv/pm_table", "rb");
	if(result->pm_table.file == NULL)
	{
		(void)close_file(result->smn);
		free(result);
		return NULL;
	}
	result->pm_table_size = pm_table_size;
	return result;
}

void free_pci_obj(pci_obj_t obj)
{
	(void)close_file(obj->pm_table);
	(void)close_file(obj->smn);
	free(obj);
}

nb_t get_nb(pci_obj_t obj)
{
	return obj;
}

void free_nb(nb_t nb)
{
	(void)nb;
}

mem_obj_t init_mem_obj(const uintptr_t physAddr)
{
	(void)physAddr;
	return (mem_obj_t)0xDEADC0DE; /* Mem_obj is not needed in this backend. */
}

void free_mem_obj(mem_obj_t obj)
{
	(void)obj;
}

u32 smn_reg_read(nb_t nb, const u32 addr)
{
	u32 result = 0;
	if(!write_to_start(nb->smn, &addr, sizeof(addr)) ||
	   !read_from_start(nb->smn, &result, sizeof(result)))
	{
		return 0;
	}
	return result;
}

void smn_reg_write(nb_t nb, const u32 addr, const u32 data)
{
	const u32 write_buffer[2] = { addr, data };
	write_to_start(nb->smn, &write_buffer, sizeof(write_buffer));
}

int copy_pm_table(nb_t nb, void *buffer, const size_t size)
{
	if(nb->pm_table_size != size)
	{
		fprintf(stderr, "PM table size mismatch between ryzenadj and ryzen_smu kernel module\n");
		return -1;
	}
	if(!read_from_start(nb->pm_table, buffer, size))
	{
		return -1;
	}
	return 0;
}

int compare_pm_table(void *buffer, const size_t size)
{
	(void)buffer;
	(void)size;
	printf("internal error: compare_pm_table() should never be called if smu driver is available\n");
	return -1;
}

bool is_using_smu_driver(void)
{
	return true;
}
