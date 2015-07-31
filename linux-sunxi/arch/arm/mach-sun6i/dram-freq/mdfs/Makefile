#makefile for mdfs.code

INCLUDE   = -I. \
	-I$(KDIR)/include \
	-I$(KDIR)/arch/arm/mach-sun6i/include

MDFS_OBJ = mdfs_entry.o \
           mdfs.o   \
           mem_mmu_pc_asm.o \
           mem_divlib.o

CC = $(CROSS_COMPILE)gcc

CFLAGS = -g -c -nostdlib -march=armv7-a -marm -fno-unwind-tables -fno-asynchronous-unwind-tables -mlittle-endian -O2 --min_array_alignment=4 --no_unaligned_access

LD_FLAGS = -static 
#LIBS = -lgcc -L/home/pannan/fiber/lichee/buildroot/output/external-toolchain/lib/gcc/arm-linux-gnueabi/4.6.3

all:
	$(CC) $(INCLUDE) $(CFLAGS) mdfs_entry.c           		-o mdfs_entry.o
	$(CC) $(INCLUDE) $(CFLAGS) mdfs.c           			-o mdfs.o
	$(CC) $(INCLUDE) $(CFLAGS) mem_mmu_pc_asm.S				-o mem_mmu_pc_asm.o
	$(CC) $(INCLUDE) $(CFLAGS) mem_divlib.S					-o mem_divlib.o

	$(CROSS_COMPILE)ld -T mdfs.ld $(LD_FLAGS) $(LIBS) -EL  $(MDFS_OBJ) -o mdfs.elf  -Map mdfs.map
	$(CROSS_COMPILE)objdump -D mdfs.elf > mdfs.lst
	$(CROSS_COMPILE)objcopy -O binary mdfs.elf mdfs.bin

	rm -rf *.o ../*.o

	cp mdfs.bin mdfs.code

	@echo ----------------------------------------
	@echo         mdfs make success
	@echo ----------------------------------------
	@echo well done!

	
clean:
	-rm *.code *.map *.lst *.bin *.elf

