#include <stdio.h>

int main()
{
	int a;
	int ret;
	extern int func1(int *a);
	ret = func1(&a);
	printf("ret = %d, a = %d\n", ret, a);
}

/*
as -o sub.o sub.s && gcc call.c sub.o && ./a.out
*/