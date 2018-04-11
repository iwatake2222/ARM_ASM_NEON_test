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

