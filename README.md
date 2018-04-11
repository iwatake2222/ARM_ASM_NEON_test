# ARMアセンブリ言語の実装色々とNEON命令のサンプル
Raspberry Pi 2 (Raspbian stretch)で動作確認

# ARMアセンブリ言語
## どうでもいいことですが
- アセンブリ (Assembly): 部品
	- 中間成果物などのコンポーネント的な意味合いが強いので、言語を示すときにはあまり使わないらしい
- アセンブリ言語 (Assembly language): プログラミング言語
- アセンブリコード (Assembly code): アセンブリ言語で書かれたコード
- アセンブラ (Assembler): アセンブリ言語をマシン語に変換するプログラム
- アセンブル (Assemble): 動詞

例: アセンブリコード(*.s)をアセンブラ(as)でアセンブルする。

## 独立したアセンブリコードファイルを使う
```asm:sub.s
	.file	"sub.s"
	.text
	.global func1
	.type	func1, %function
func1:
	mov r1, #99
	str r1, [r0]	@ r0 is the first arg
	mov r0, #88
	bx	lr
	.end
```

```c:call.c
#include <stdio.h>
int main()
{
	int a;
	int ret;
	extern int func1(int *a);
	ret = func1(&a);
	printf("ret = %d, a = %d\n", ret, a);
}
```

```sh:ビルドコマンドと実行結果
as -o sub.o sub.s
gcc call.c sub.o
./a.out
ret = 88, a = 99
```

### 注意
- このサンプルでは考慮していないが、レジスタの退避などを自分で実装する必要がある
- 引数や戻り値もAAPCSに沿った実装が必要

## Cコード内でインラインアセンブラを使う
ダブルクオーテーションで囲ったり、エラーが見ずらいのが難点だけど、実際にはこの方法の方が便利だと思う。

```c:inline.c
#include <stdio.h>

int func1(int *a)
{
	int ret = 0;
	__asm__ __volatile__ (
		"mov r1, #99;"
		"str r1, [%1];"
		"mov %0, #88;"
		: "=r"(ret)		// Output operands
		: "r"(a)		// Input operands
		:				// Overwritten registers
	);
	return ret;
}

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
	int a, b, ret;

	ret = func1(&a);
	printf("ret = %d, a = %d\n", ret, a);

	ret = add(1, 2);
	printf("ret = %d\n", ret);

	ret = add_label(1, 2);
	printf("ret = %d\n", ret);

	a = 10; b = 0;
	copy(&a, &b);
	printf("a = %d b = %d\n", a, b);
}
```

```sh:ビルドコマンドと実行結果
gcc inline.c
./a.out
ret = 88, a = 99
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
#include <stdint.h>
#include <string.h>

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

int main()
{
	add();
}
```

```sh:ビルドコマンドと実行結果
gcc simd.c -mfpu=neon
./a.out
12 + 34 = 46
12 + 34 = 46
EE + 34 = FF
12 + 34 = 46
12 + 34 = 46
12 + 34 = 46
12 + 34 = 46
12 + 34 = 46
```

## NEONの組み込み関数を使う
ARM NEON Intrinsics (https://gcc.gnu.org/onlinedocs/gcc-4.3.2/gcc/ARM-NEON-Intrinsics.html)

```c:simd.c
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arm_neon.h>

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
	add_emb();
}
```

```sh:ビルドコマンドと実行結果
gcc simd.c -mfpu=neon
./a.out
12 + 34 = 46
12 + 34 = 46
EE + 34 = FF
12 + 34 = 46
12 + 34 = 46
12 + 34 = 46
12 + 34 = 46
12 + 34 = 46
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

# その他
- gccビルド時に`-march=native`オプションがあった方がいいかも
- `vld1.8`や`vld1_u8()`の1って何? 
    - Interleave pattern。数字には1,2,3,4が設定できる。指定間隔で飛び飛びでload/storeができるの。例えば3を設定したら、RGBR GBRG BRGBという並びを、RRRR GGGG BBBBにして計算しやすくできる。
    - https://community.arm.com/processors/b/blog/posts/coding-for-neon---part-1-load-and-stores

# 参考
https://www.aps-web.jp/academy/ca/14/
(↑図が分かりやすい。けど、こちらの記事はGCCじゃなくてARM製コンパイラを前提にしているっぽいので、コマンドは微妙に異なるので注意)
