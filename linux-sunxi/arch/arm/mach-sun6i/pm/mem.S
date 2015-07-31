	.align	4
	.globl	resume1_bin_start
resume1_bin_start:
	.incbin	"arch/arm/mach-sun6i/pm/standby/super/resume/resume1.code"
	.globl	resume1_bin_end
resume1_bin_end:
	.align	4
	
	