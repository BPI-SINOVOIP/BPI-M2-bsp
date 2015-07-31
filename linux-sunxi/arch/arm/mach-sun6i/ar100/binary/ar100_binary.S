	.globl  ar100_binary_start
	.globl  ar100_binary_end
ar100_binary_start:
	.incbin "arch/arm/mach-sun6i/ar100/binary/ar100.code"
ar100_binary_end:
	.align  2
