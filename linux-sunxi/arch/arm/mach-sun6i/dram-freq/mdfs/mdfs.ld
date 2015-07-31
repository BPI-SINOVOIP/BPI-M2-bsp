
OUTPUT_ARCH(arm)
ENTRY(mdfs_main)

SECTIONS
{
	__mdfs_start = . ;
	.text 0xf0000000 :
	{
		mdfs_entry.o(.text.startup)
		*(.text)
		*(.rodata)
	}

	.data   : { *(.data) }
	__bss_start = . ;
	.bss : { *(.sbss) *(.scommon) *(.bss) *(COMMON) }
	__bss_end = .;
	__mdfs_end = . ;
}

