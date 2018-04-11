#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arm_neon.h>

void add()
{
	uint8_t a[8];	// d0
	uint8_t b[8];	// d1
	uint8_t c[8];	// result
	memset(a, 0x12, sizeof(a));
	a[2] = 0xEE;	// 飽和確認用
	memset(b, 0x34, sizeof(b));
	memset(c, 0x00, sizeof(c));

	__asm__ __volatile__ (
		"vld1.8 {d0}, [%0];"	// d0 <- a
		"vld1.8 {d1}, [%1];"	// d1 <- b
		"vqadd.u8 d2, d1, d0;"		// d2 <- d1 + d0 (per 8bit)	with saturation
		"vst1.8 {d2}, [%2];"		// d2 -> c
		:				 			// Output operands
		: "r"(a), "r"(b), "r"(c)	// Input operands
		: 							// Overwritten registers
	);

	for (int i = 0; i < 8; i++)
		printf("%02X + %02X = %02X\n", a[i], b[i], c[i]);

	return;
}

void add_emb()
{
	uint8_t a[8];	// d0
	uint8_t b[8];	// d1
	uint8_t c[8];	// result
	memset(a, 0x12, sizeof(a));
	a[2] = 0xEE;	// 飽和確認用
	memset(b, 0x34, sizeof(b));
	memset(c, 0x00, sizeof(c));

	uint8x8_t va, vb, vc;		// 8-bit(uint8_t) x 8レーン
	va = vld1_u8(a);
	vb = vld1_u8(b);
	vc = vqadd_u8(va, vb);
	vst1_u8(c, vc);

	for (int i = 0; i < 8; i++)
		printf("%02X + %02X = %02X\n", a[i], b[i], c[i]);

	return;
}

int main()
{
	add();
	printf("=====================\n");
	add_emb();
}

/*
gcc simd.c sub.o -mfpu=neon && ./a.out

*/
