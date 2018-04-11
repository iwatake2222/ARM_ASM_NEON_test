#ARMアセンブリ言語の実装色々とNEON命令のサンプル

Raspberry Pi 2 (Raspbian stretch)で動作確認

# ARMアセンブリ言語
## どうでもいいことですが
- アセンブリ (Assembly): 部品
	- 中間成果物などのコンポーネント的な意味合いが強いので、言語を示すときにはあまり使わないらしい
- アセンブリ言語 (Assembly language): プログラミング言語
- アセンブリコード (Assembly code): アセンブリ言語で書かれたコード
- アセンブラ (Assembler): アセンブリ言語をマシン語に変換するプログラム
- アセンブル (Assemble): 動詞

例: アセンブリコード(*.s)をアセンブラ(ar)でアセンブルする。

## 独立したアセンブリコードファイルを使う
```asm:sub.s
	.file	"sub.s"
	.text
	.global func1
	.type	func1, %function
func1:
	mov r1, #99
	str r1, [r0]	@ r0 is the first arg
	.end
```

```c:call.c
#include <stdio.h>
int main()
{
	int a;
	extern void func1(int *a);
	func1(&a);
	printf("a = %d\n", a);
}
```

```sh:ビルドコマンドと実行結果
as -o sub.o sub.s
gcc call.c sub.o
./a.out
a = 99
```

### 注意
- このサンプルでは考慮していないが、レジスタの退避などを自分で実装する必要がある
- 引数や戻り値もAAPCSに沿った実装が必要

## Cコード内でインラインアセンブラを使う
ダブルクオーテーションで囲ったり、エラーが見ずらいのが難点だけど、実際にはこの方法の方が便利だと思う。

```c:inline.c
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
```

```sh:ビルドコマンドと実行結果
gcc inline.c
./a.out
ret = 3
ret = 3
a = 10 b = 10
```

# NEON命令を試す
NEON = Advanced SIMD。
NEONユニットは32本の64-bit SIMDレジスタ(D0-D31)を持つ。16本x128-bitとしても使える(Qレジスタ)。
なので、32-bitアーキのCPUからデータコピーするときは注意。
ビルド時に`-mfpu=neon`オプションを忘れない。

## 自分でNEON命令を書く
8-bit x 8個のデータの飽和加算例。

```c:simd.c
#include <stdio.h>

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

int main()
{
	add();
}
```

```sh:ビルドコマンドと実行結果
gcc simd.c -mfpu=neon
./a.out
11111111_11EE1111
22222222_22222222

33333333_33FF3333
```

## NEONの組み込み関数を使う
```c:simd.c
#include <stdio.h>
#include <arm_neon.h>

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
	add_emb();
}
```

```sh:ビルドコマンドと実行結果
gcc simd.c -mfpu=neon
./a.out
11111111_11EE1111
22222222_22222222

33333333_33FF3333
```

## コンパイラに任せる
適切なコンパイラオプションを付ければ、できる範囲で自動ベクトル化してNEON命令を使ってくれる。
うまくベクトル化されるようにコードを書くテクニックが必要そう。

```c:simd_auto.c
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
```

```sh:ビルドコマンド確認用
gcc -mfpu=neon -O3 -S simd_auto.c
more simd_auto.s
```

最適化オプション(`-O3`)がないとNEON命令が使われなかった。`-O2`でもダメ。

# 注意
- gccビルド時に`-march=native`オプションがあった方がいいかも


# 参照
https://www.aps-web.jp/academy/ca/14/
(↑図が分かりやすい。けど、ここでの説明はGCCじゃなくてARM製コンパイラを前提にしているっぽいので、コマンドは微妙に異なるので注意)