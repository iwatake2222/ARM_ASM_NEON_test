#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arm_neon.h>

void add()
{
	uint8_t a[8];	// set to d0_low and d0_high
	uint8_t b[8];	// set to d1_low and d1_high
	uint8_t c[8];	// result
	memset(a, 0x11, sizeof(a));
	a[2] = 0xEE;	// 飽和確認用
	memset(b, 0x22, sizeof(b));
	memset(c, 0x00, sizeof(c));

	printf("%08X_%08X\n%08X_%08X\n\n", *(uint32_t*)&a[4], *(uint32_t*)&a[0], *(uint32_t*)&b[4], *(uint32_t*)&b[0]);

	__asm__ __volatile__ (
		"vld1.8 {d0}, [%0];"	// d0 <- a
		"vld1.8 {d1}, [%1];"	// d1 <- b
		"vqadd.u8 d2, d1, d0;"		// d2 <- d1 + d0 (per 8bit)	with saturation
		"vst1.8 {d2}, [%2];"		// d2 -> c
		:				 			// Output operands
		: "r"(a), "r"(b), "r"(c)	// Input operands
		: 							// Overwritten registers
	);

	printf("%08X_%08X\n", *(uint32_t*)&c[4], *(uint32_t*)&c[0]);
	return;
}

void add_emb()
{
	uint8_t a[8];
	uint8_t b[8];
	uint8_t c[8];	// result
	memset(a, 0x11, sizeof(a));
	a[2] = 0xEE;	// 飽和確認用
	memset(b, 0x22, sizeof(b));
	memset(c, 0x00, sizeof(c));

	printf("%08X_%08X\n%08X_%08X\n\n", *(uint32_t*)&a[4], *(uint32_t*)&a[0], *(uint32_t*)&b[4], *(uint32_t*)&b[0]);
	
	uint8x8_t va, vb, vc;		// 8-bit(uint8_t) x 8レーン
	va = vld1_u8(a);
	vb = vld1_u8(b);
	vc = vqadd_u8(va, vb);
	vst1_u8(c, vc);

	printf("%08X_%08X\n", *(uint32_t*)&c[4], *(uint32_t*)&c[0]);
	return;
}

int main()
{
	add();
	add_emb();
}

/*
gcc simd.c sub.o -mfpu=neon && ./a.out

*/
