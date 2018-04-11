#include <stdio.h>

int add(int a, int b)
{
	int ret = 0;
	__asm__ __volatile__ (
		"add %0, %1, %2;"
		: "=r"(ret)			// Output operands
		: "r"(a), "r"(b)	// Input operands
		:					// Overwritten registers
	);
	return ret;
}

int add_label(int a, int b)
{
	int ret = 0;
	__asm__ __volatile__ (
		"add %[Rret], %[Ra], %[Rb];"
		: [Rret]"=r"(ret)			// Output operands
		: [Ra]"r"(a), [Rb]"r"(b)	// Input operands
		:							// Overwritten registers
	);
	return ret;
}

void copy(int *src, int *dst)
{
	__asm__ __volatile__ (
		"ldr r2, [%0];"	// r2 <- *src
		"str r2, [%1];"	// r2 -> *dst
		: 						// Output operands
		: "r"(src), "r"(dst)	// Input operands
		: "r2", "memory"		// Overwritten registers
	);
	return;
}

int main()
{
	printf("Hello World\n");
	int ret;
	ret = add(1, 2);
	printf("ret = %d\n", ret);
	ret = add_label(1, 2);
	printf("ret = %d\n", ret);

	int a = 10;
	int b;
	copy(&a, &b);
	printf("a = %d b = %d\n", a, b);
}

/*
gcc inline.c && ./a.out

*/
