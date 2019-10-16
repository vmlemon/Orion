#include <stdio.h>
size_t CacheLineSize() {
	FILE * p = 0;
	p = fopen("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size", "r");
	unsigned int lineSize = 0;
	if (p) {
		fscanf(p, "%d", &lineSize);
		fclose(p);
	}
	return lineSize;
}

void main() {
	printf("%d\n", CacheLineSize());
}
