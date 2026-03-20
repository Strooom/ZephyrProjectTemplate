#include <stdio.h>
#include <zephyr/kernel.h>

int main(void)
{
	while (1) {
		printf("Hello JLink !\n");
		k_msleep(1000);
	}
	return 0;
}
