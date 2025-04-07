#include <linux/ioctl.h>

#define MY_DEVICE_MAGIC 'M'

#define IOCTL_GET_VALUE _IOR(MY_DEVICE_MAGIC, 1, int)
#define IOCTL_SET_VALUE _IOW(MY_DEVICE_MAGIC, 2, int)
#define IOCTL_DO_OPERATION _IO(MY_DEVICE_MAGIC, 3)
#define IOCTL_EXCHANGE_DATA _IOWR(MY_DEVICE_MAGIC, 4, struct my_data)

struct my_data {
	int input1;
	int input2;
	int result;
};
