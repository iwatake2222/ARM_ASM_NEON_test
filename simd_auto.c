#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define NUM 128

void add(uint8_t * a, uint8_t * b, uint8_t * c)
{

	for(int i = 0; i < NUM; i++) {
		c[i] = a[i] + b[i];
	}
}

int main()
{
	uint8_t a[NUM], b[NUM], c[NUM];
	memset(a, 0x12, sizeof(a));
	memset(b, 0x34, sizeof(a));
	add(a, b, c);
	printf("%08X\n", *(uint32_t*)&c[0]);
}

/*
gcc -mfpu=neon -O3 -S simd_emb.c && more simd_emb.s
gcc -O3 -S simd_emb.c && more simd_emb.s

gcc -mfpu=neon -O3 simd_emb.c && ./a.out

*/
