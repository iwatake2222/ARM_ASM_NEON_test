#include <stdio.h>

int main()
{
	int a;
	extern void func1(int *a);
	func1(&a);
	printf("a = %d\n", a);
}

/*
as -o sub.o sub.s && gcc call.c sub.o && ./a.out
*/