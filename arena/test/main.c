#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "my_device.h"

int main()
{
	int fd, value = 42;
	struct my_data data = { .input1 = 10, .input2 = 20 };

	fd = open("/dev/my_device", O_RDWR);
	if (fd < 0) {
		perror("Failed to open device!");
		return 1;
	}

	if (ioctl(fd, IOCTL_SET_VALUE, &value) < 0) {
		perror("IOCTL_SET_VALUE failed");
		close(fd);
		return 1;
	}

	value = 0;

	if (ioctl(fd, IOCTL_GET_VALUE, &value) < 0) {
		perror("IOCTL_SET_VALUE failed");
		close(fd);
		return 1;
	}
	printf("Value from kernel: %d\n", value);

	if (ioctl(fd, IOCTL_DO_OPERATION) < 0) {
		perror("IOCTL_DO_OPERATION failed");
		close(fd);
		return 1;
	}

	if (ioctl(fd, IOCTL_GET_VALUE, &value) < 0) {
		perror("IOCTL_SET_VALUE failed");
		close(fd);
		return 1;
	}
	printf("Value from kernel: %d\n", value);

	if (ioctl(fd, IOCTL_EXCHANGE_DATA, &data) < 0) {
		perror("IOCTL_SET_VALUE failed");
		close(fd);
		return 1;
	}
	printf("Value from kernel: %d\n", data.result);

	close(fd);
	return 0;
}
