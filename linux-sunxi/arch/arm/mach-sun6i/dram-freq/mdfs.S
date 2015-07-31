	.globl	mdfs_bin_start
mdfs_bin_start:
	.incbin	"arch/arm/mach-sun6i/dram-freq/mdfs/mdfs.code"
	.globl	mdfs_bin_end
mdfs_bin_end:
	.align	2
