#makefile for standby.bin

INCLUDE   = -I. \
	-I$(KDIR)/include \
	-I$(KDIR)/arch/arm/mach-sun6i/include
	
STANDBY_OBJ = 	common.o         	\
		standby.o        	\
		standby_clock.o  	\
		standby_ir.o     	\
		standby_key.o    	\
		standby_power.o  	\
		standby_twi.o    	\
		standby_usb.o    	\
		standby_delay.o  	\
		./ar100/standby_ar100.o 	\
		./ar100/ar100_hwmsgbox.o 	\
		./ar100/ar100_hwspinlock.o 	\
		./ar100/ar100_message_manager.o	\
		./../pm_debug.o       		\
		./../mem_timing.o      		\
		./../mem_mmu_pc.o               \
		./../mem_mmu_pc_asm.o           \
		./../mem_serial.o		\
		./../mem_printk.o  		\
		./../mem_divlib.o		\
		./../mem_divlibc.o		\
		./../mem_int.o			\
		./../mem_tmr.o			\
		./../mem_clk.o

RESUME1_OBJ =	./super/common.o             	\
		./super/super_twi.o        	\
		./super/super_delay.o      	\
		./super/super_clock.o      	\
		./super/super_power.o      	\
		./super/super_cpus.o      	\
		./super/resume/resume1.o 	\
		./super/resume/resume_head.o 	\
		./super/resume/resume1_c_part.o \
		./../pm_debug.o                 \
		./../mem_timing.o               \
		./../mem_mmu_pc.o               \
		./../mem_mmu_pc_asm.o           \
		./../mem_cpu.o                  \
		./../mem_serial.o		\
		./../mem_printk.o		\
		./../mem_divlib.o		\
		./../mem_divlibc.o		\
		./../mem_int.o			\
		./../mem_tmr.o			\
		./../mem_clk.o
		
STANDBY_OUPUTFILE = standby
RESUME1_OUPUTFILE = resume1

STANDBY_LD_FILE = standby.xn
RESUME1_LD_FILE = resume1_scatter.scat

CC = $(CROSS_COMPILE)gcc

#CFLAGS = -g -c -nostdlib -march=armv7-a -marm -fno-unwind-tables -fno-asynchronous-unwind-tables -mlittle-endian  --min_array_alignment=4 --no_unaligned_access -O2 
CFLAGS = -g -c -nostdlib -march=armv7-a -marm -D__STANDBY_MODULE__ -fno-unwind-tables -fno-asynchronous-unwind-tables -mlittle-endian -O2 --min_array_alignment=4 --no_unaligned_access

LD_FLAGS = -static 
LIBS = 

all: standby resume1 
standby:
	$(CC) $(INCLUDE) $(CFLAGS) common.c           			-o common.o
	$(CC) $(INCLUDE) $(CFLAGS) standby.c          			-o standby.o
	$(CC) $(INCLUDE) $(CFLAGS) standby_clock.c    			-o standby_clock.o
	$(CC) $(INCLUDE) $(CFLAGS) standby_ir.c       			-o standby_ir.o
	$(CC) $(INCLUDE) $(CFLAGS) standby_key.c      			-o standby_key.o
	$(CC) $(INCLUDE) $(CFLAGS) standby_power.c    			-o standby_power.o
	$(CC) $(INCLUDE) $(CFLAGS) standby_twi.c      			-o standby_twi.o
	$(CC) $(INCLUDE) $(CFLAGS) standby_usb.c      			-o standby_usb.o
	$(CC) $(INCLUDE) $(CFLAGS) standby_delay.S    			-o standby_delay.o
	$(CC) $(INCLUDE) $(CFLAGS) ./ar100/standby_ar100.c    		-o ./ar100/standby_ar100.o
	$(CC) $(INCLUDE) $(CFLAGS) ./ar100/ar100_hwmsgbox.c   		-o ./ar100/ar100_hwmsgbox.o
	$(CC) $(INCLUDE) $(CFLAGS) ./ar100/ar100_hwspinlock.c 		-o ./ar100/ar100_hwspinlock.o
	$(CC) $(INCLUDE) $(CFLAGS) ./ar100/ar100_message_manager.c	-o ./ar100/ar100_message_manager.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../pm_debug.c    			-o ./../pm_debug.o 
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_timing.c    		-o ./../mem_timing.o 
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_mmu_pc.c                   -o ./../mem_mmu_pc.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_mmu_pc_asm.S               -o ./../mem_mmu_pc_asm.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_serial.c    -o 		./../mem_serial.o 
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_printk.c    -o 		./../mem_printk.o 
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_divlib.S    -o 		./../mem_divlib.o 
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_divlibc.c    -o 		./../mem_divlibc.o 
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_int.c    -o 		./../mem_int.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_tmr.c    -o 		./../mem_tmr.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_clk.c    -o 		./../mem_clk.o
	
	$(CROSS_COMPILE)ld -T $(STANDBY_LD_FILE) $(LD_FLAGS) $(LIBS) -EL  $(STANDBY_OBJ) -o $(STANDBY_OUPUTFILE).elf  -Map $(STANDBY_OUPUTFILE).map
	$(CROSS_COMPILE)objdump -D $(STANDBY_OUPUTFILE).elf > $(STANDBY_OUPUTFILE).lst
	$(CROSS_COMPILE)objcopy -O binary $(STANDBY_OUPUTFILE).elf $(STANDBY_OUPUTFILE).bin

	rm -rf *.o ../*.o

	cp standby.bin standby.code

	@echo ----------------------------------------
	@echo well done!
	
	
resume1:
	$(CC) $(INCLUDE) $(CFLAGS) ./super/common.c                    -o ./super/common.o
	$(CC) $(INCLUDE) $(CFLAGS) ./super/super_twi.c               -o ./super/super_twi.o
	$(CC) $(INCLUDE) $(CFLAGS) ./super/super_delay.S             -o ./super/super_delay.o
	$(CC) $(INCLUDE) $(CFLAGS) ./super/super_clock.c             -o ./super/super_clock.o
	$(CC) $(INCLUDE) $(CFLAGS) ./super/super_power.c             -o ./super/super_power.o
	$(CC) $(INCLUDE) $(CFLAGS) ./super/super_cpus.c              -o ./super/super_cpus.o	
	
	$(CC) $(INCLUDE) $(CFLAGS) ./super/resume/resume1_c_part.c     	-o ./super/resume/resume1_c_part.o
	$(CC) $(INCLUDE) $(CFLAGS) ./super/resume/resume_head.c     	-o ./super/resume/resume_head.o	
	$(CC) $(INCLUDE) $(CFLAGS) ./super/resume/resume1.S        -o ./super/resume/resume1.o

	
	$(CC) $(INCLUDE) $(CFLAGS) ./../pm_debug.c                     -o ./../pm_debug.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_timing.c    		-o ./../mem_timing.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_mmu_pc.c                   -o ./../mem_mmu_pc.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_mmu_pc_asm.S               -o ./../mem_mmu_pc_asm.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_cpu.c		       -o ./../mem_cpu.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_serial.c    		-o ./../mem_serial.o 
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_printk.c    		-o ./../mem_printk.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_divlib.S    -o 		./../mem_divlib.o 
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_divlibc.c    -o 		./../mem_divlibc.o 
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_int.c    	-o 		./../mem_int.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_tmr.c    -o 		./../mem_tmr.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_clk.c    -o 		./../mem_clk.o
	
	$(CROSS_COMPILE)ld -T $(RESUME1_LD_FILE)  -EL  $(RESUME1_OBJ) -o $(RESUME1_OUPUTFILE).elf  -Map $(RESUME1_OUPUTFILE).map
	$(CROSS_COMPILE)objdump -D $(RESUME1_OUPUTFILE).elf > $(RESUME1_OUPUTFILE).lst
	$(CROSS_COMPILE)objcopy -O binary $(RESUME1_OUPUTFILE).elf $(RESUME1_OUPUTFILE).bin

	rm -rf *.o ../*.o

	#cp resume1.bin ./super/resume/resume1.code
	./gen_check_code ./resume1.bin ./super/resume/resume1.code 
	
	@echo ----------------------------------------
	@echo well done!

	
clean:
	-rm *.map *.lst *.bin *.elf

