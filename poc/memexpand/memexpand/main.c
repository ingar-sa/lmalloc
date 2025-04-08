#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "memexpand.h"

int main()
{
	int fd;
	struct memexpand_data mem_data;
	int *memory;

	fd = open("/dev/memexpand", O_RDWR);
	if (fd < 0) {
		perror("Failed to open device");
		return 1;
	}

	mem_data.size = 1024 * 1;

	if (ioctl(fd, MEMEXPAND_ALLOCATE, &mem_data) < 0) {
		perror("Memory allocation failed");
		close(fd);
		return 1;
	}

	printf("Memory size allocated: %lu bytes\n", mem_data.size);

	memory = mmap(NULL, mem_data.size, PROT_READ | PROT_WRITE, MAP_SHARED,
		      fd, 0);
	if (memory == MAP_FAILED) {
		perror("mmap failed");
		close(fd);
		return 1;
	}

	if (!memory) {
		perror("memory is null pointer\n");
		close(fd);
		return 1;
	}

	printf("Memory mapped at address: %p\n", memory);

	memory[0] = 42;
	printf("Value written: %d\n", memory[0]);

	munmap(memory, mem_data.size);
	close(fd);
	return 0;
}
