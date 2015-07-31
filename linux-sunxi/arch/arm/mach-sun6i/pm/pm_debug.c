#include "pm_types.h"
#include "pm.h"

//for io-measure time
#define PORT_E_CONFIG (0xf1c20890)
#define PORT_E_DATA (0xf1c208a0)
#define PORT_CONFIG PORT_E_CONFIG
#define PORT_DATA PORT_E_DATA

volatile int  print_flag = 0;

void busy_waiting(void)
{
#if 1
	volatile __u32 loop_flag = 1;
	while(1 == loop_flag);
	
#endif
	return;
}

void fake_busy_waiting(void)
{
#if 1
	volatile __u32 loop_flag = 2;
	while(1 == loop_flag);
	
#endif
	return;
}

/*
 * notice: when resume, boot0 need to clear the flag, 
 * in case the data in dram be destoryed result in the system is re-resume in cycle.
*/
void save_mem_flag(void)
{
#if 0
	__u32 saved_flag = *(volatile __u32 *)(PERMANENT_REG);
	saved_flag &= 0xfffffffe;
	saved_flag |= 0x00000001;
	*(volatile __u32 *)(PERMANENT_REG) = saved_flag;
#endif
	return;
}

/*
 * before enter suspend, need to clear mem flag, 
 * in case the flag is failed to clear by resume code 
 * 
*/
void clear_mem_flag(void)
{
#if 0
	__u32 saved_flag = *(volatile __u32 *)(PERMANENT_REG);
	saved_flag &= 0xfffffffe;
	*(volatile __u32 *)(PERMANENT_REG) = saved_flag;
#endif
	return;
}


void save_mem_status(volatile __u32 val)
{
	*(volatile __u32 *)(STATUS_REG  + 0x04) = val;
	return;
}

__u32 get_mem_status(void)
{
	return *(volatile __u32 *)(STATUS_REG  + 0x04);
}

void save_mem_status_nommu(volatile __u32 val)
{
	*(volatile __u32 *)(STATUS_REG_PA  + 0x04) = val;
	return;
}

void save_cpux_mem_status_nommu(volatile __u32 val)
{
	*(volatile __u32 *)(STATUS_REG_PA  + 0x00) = val;
	return;
}




/*
 * notice: dependant with perf counter to delay.
 */
void io_init(void)
{
	//config port output
	*(volatile unsigned int *)(PORT_CONFIG)  = 0x111111;
	
	return;
}

void io_init_high(void)
{
	__u32 data;
	
	//set port to high
	data = *(volatile unsigned int *)(PORT_DATA);
	data |= 0x3f;
	*(volatile unsigned int *)(PORT_DATA) = data;

	return;
}

void io_init_low(void)
{
	__u32 data;

	data = *(volatile unsigned int *)(PORT_DATA);
	//set port to low
	data &= 0xffffffc0;
	*(volatile unsigned int *)(PORT_DATA) = data;

	return;
}

/*
 * set pa port to high, num range is 0-7;	
 */
void io_high(int num)
{
	__u32 data;
	data = *(volatile unsigned int *)(PORT_DATA);
	//pull low 10ms
	data &= (~(1<<num));
	*(volatile unsigned int *)(PORT_DATA) = data;
	delay_us(10000);
	//pull high
	data |= (1<<num);
	*(volatile unsigned int *)(PORT_DATA) = data;

	return;
}
