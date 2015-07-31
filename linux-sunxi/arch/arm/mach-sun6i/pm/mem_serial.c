/**
 * serial.c - common operations
 * date:    2012-2-13 8:42:56
 * author:  Aaron<leafy.myeh@allwinnertech.com>
 * history: V0.1
 */

#include "pm_types.h"
#include "pm.h"

//------------------------------------------------------------------------------
//return value defines
//------------------------------------------------------------------------------
#define	OK		(0)
#define	FAIL		(-1)
#define TRUE		(1)
#define	FALSE		(0)
#define NULL		(0)

#define readb(addr)		(*((volatile unsigned char  *)(addr)))
#define readw(addr)		(*((volatile unsigned short *)(addr)))
#define readl(addr)		(*((volatile unsigned long  *)(addr)))
#define writeb(v, addr)		(*((volatile unsigned char  *)(addr)) = (unsigned char)(v))
#define writew(v, addr)		(*((volatile unsigned short *)(addr)) = (unsigned short)(v))
#define writel(v, addr)		(*((volatile unsigned long  *)(addr)) = (unsigned long)(v))

#define AW_CCU_UART_PA          (0x01c2006C)
#define AW_CCU_UART_RESET_PA    (0x01c202D8)
#define AW_UART_GPIO_PA         (0x01c20800 + 0xb4)            //UART0:sun8iw1 tx:PF2 rx:PF4

static __u32 backup_ccu_uart            = 0;
static __u32 backup_ccu_uart_reset      = 0;
static __u32 backup_gpio_uart           = 0;
static __u32 serial_inited_flag = 0;

static __u32 set_serial_clk(__u32 mmu_flag)
{
	__u32 			src_freq = 0;
	__u32 			p2clk;
	volatile unsigned int 	*reg;
	__ccmu_reg_list_t 	*ccu_reg;	
	__u32 port = 0;
	__u32 i = 0;

	ccu_reg = (__ccmu_reg_list_t   *)mem_clk_init(mmu_flag);
	//check uart clk src is ok or not.
	//the uart clk src need to be pll6 & clk freq == 600M?
	//so the baudrate == p2clk/(16*div)
	switch(ccu_reg->Apb2Div.ClkSrc){
		case 0:
			src_freq = 32000;	//32k
			break;
		case 1:
			src_freq = 24000000;	//24M
			break;
		case 2:
			src_freq = 600000000;	//600M
			break;
		default:
			break;
	}

	//calculate p2clk.
	p2clk = src_freq/((ccu_reg->Apb2Div.DivM + 1) * (1<<(ccu_reg->Apb2Div.DivN)));

	/*notice:
	**	not all the p2clk is able to create the specified baudrate.
	**	unproper p2clk may result in unacceptable baudrate, just because
	**	the uartdiv is not proper and the baudrate err exceed the acceptable range. 
	*/
	if(mmu_flag){
		//backup apb2 gating;
		backup_ccu_uart = *(volatile unsigned int *)(IO_ADDRESS(AW_CCU_UART_PA));
		//backup uart reset 
		backup_ccu_uart_reset = *(volatile unsigned int *)(IO_ADDRESS(AW_CCU_UART_RESET_PA));
		//backup gpio
		backup_gpio_uart = *(volatile unsigned int *)(IO_ADDRESS(AW_UART_GPIO_PA));

		//de-assert uart reset
		reg = (volatile unsigned int *)(IO_ADDRESS(AW_CCU_UART_RESET_PA));
		*reg &= ~(1 << (16 + port));
		for( i = 0; i < 100; i++ );
		*reg |=  (1 << (16 + port));
		change_runtime_env(1);
		delay_us(1);	
		//config uart clk: apb2 gating.
		reg = (volatile unsigned int *)(IO_ADDRESS(AW_CCU_UART_PA));
		*reg &= ~(1 << (16 + port));
		for( i = 0; i < 100; i++ );
		*reg |=  (1 << (16 + port));

	}else{
		//de-assert uart reset
		reg = (volatile unsigned int *)(AW_CCU_UART_RESET_PA);
		*reg &= ~(1 << (16 + port));
		for( i = 0; i < 100; i++ );
		*reg |=  (1 << (16 + port));
		change_runtime_env(0);
		delay_us(1);	
		//config uart clk
		reg = (volatile unsigned int *)(AW_CCU_UART_PA);
		*reg &= ~(1 << (16 + port));
		for( i = 0; i < 100; i++ );
		*reg |=  (1 << (16 + port));

	}

	return p2clk;
	
}


static void set_serial_gpio(__u32 mmu_flag)
{
	__u32 port = 0;
	__u32 i = 0;
	volatile unsigned int 	*reg;
	
	// config uart gpio
	// config tx gpio
	// fpga not need care gpio config;

	if(mmu_flag){
		reg = (volatile unsigned int *)(IO_ADDRESS(AW_UART_GPIO_PA));
	}else{
		reg = (volatile unsigned int *)(AW_UART_GPIO_PA);
	}
	*reg &= ~(0x707 << (8 + port));
	for( i = 0; i < 100; i++ );
	*reg |=  (0x404 << (8 + port));

    return	;
}



void serial_init_nommu(void)
{
	__u32 df;
	__u32 lcr;
	__u32 p2clk;

	set_serial_gpio(0);
	p2clk = set_serial_clk(0);

	/* set baudrate */
	df = (p2clk + (SUART_BAUDRATE<<3))/(SUART_BAUDRATE<<4);
	lcr = readl(SUART_LCR_PA);
	writel(1, SUART_HALT_PA);
	writel(lcr|0x80, SUART_LCR_PA);
	writel(df>>8, SUART_DLH_PA);
	writel(df&0xff, SUART_DLL_PA);
	writel(lcr&(~0x80), SUART_LCR_PA);
	writel(0, SUART_HALT_PA);

	/* set mode, Set Lin Control Register*/
	writel(3, SUART_LCR_PA);
	/* enable fifo */
	writel(0xe1, SUART_FCR_PA);

	// set init complete flag;
	serial_inited_flag = 1;
	return	;
}

static void serial_put_char_nommu(char c)
{
	while (!(readl(SUART_USR_PA) & 2));
	writel(c, SUART_THR_PA);

	return	;
}

static char serial_get_char_nommu(void)
{
	__u32 time = 0xffff;
	while(!(readl(SUART_USR_PA)&0x08) && time--);
	if (!time)
		return 0;
	return readl(SUART_RBR_PA);
}

__s32 serial_puts_nommu(const char *string)
{
	//ASSERT(string != NULL);
	
	if(0 == serial_inited_flag){
	    return FAIL;
	}

	while(*string != '\0')
	{
		if(*string == '\n')
		{
			// if current character is '\n', 
			// insert output with '\r'.
			serial_put_char_nommu('\r');
		}
		serial_put_char_nommu(*string++);
	}
	
	return OK;
}

__u32 serial_gets_nommu(char* buf, __u32 n)
{
	__u32 i;
	char c;
	
	if(0 == serial_inited_flag){
	    return FAIL;
	}

	for (i=0; i<n; i++) {
		c = serial_get_char_nommu();
		if (c == 0)
			break;
		buf[i] = c;
	}
	return i+1;
}

void serial_init(void)
{

	__u32 p2clk;
	__u32 df;
	__u32 lcr;

	set_serial_gpio(1);
	p2clk = set_serial_clk(1);
	
	/* set baudrate */
	df = (p2clk + (SUART_BAUDRATE<<3))/(SUART_BAUDRATE<<4);
	lcr = readl(SUART_LCR);
	writel(1, SUART_HALT);
	writel(lcr|0x80, SUART_LCR);
	writel(df>>8, SUART_DLH);
	writel(df&0xff, SUART_DLL);
	writel(lcr&(~0x80), SUART_LCR);
	writel(0, SUART_HALT);

	/* set mode, Set Lin Control Register*/
	writel(3, SUART_LCR);
	/* enable fifo */
	//writel(0xe1, SUART_FCR);
	writel(0xe1, SUART_FCR);

	//set init complete flag;
	serial_inited_flag = 1;
	return	;
}

void serial_exit(void)
{
	//restore apb2 gating;
	*(volatile unsigned int *)(IO_ADDRESS(AW_CCU_UART_PA)) = backup_ccu_uart;
	//restore uart reset 
	*(volatile unsigned int *)(IO_ADDRESS(AW_CCU_UART_RESET_PA)) = backup_ccu_uart_reset;
	//restore gpio
	*(volatile unsigned int *)(IO_ADDRESS(AW_UART_GPIO_PA)) = backup_gpio_uart;

	//clear init complete flag;
	serial_inited_flag = 0;
	return	;
}


static void serial_put_char(char c)
{

	while (!(readl(SUART_USR) & 2));
	writel(c, SUART_THR);

	return	;
}

static char serial_get_char(void)
{
	__u32 time = 0xffff;
	while(!(readl(SUART_USR)&0x08) && time--);
	if (!time)
		return 0;
	return (char)(readl(SUART_RBR));
}


/*
*********************************************************************************************************
*                                       	PUT A STRING
*
* Description: 	put out a string.
*
* Arguments  : 	string	: the string which we want to put out.
*
* Returns    : 	OK if put out string succeeded, others if failed.
*********************************************************************************************************
*/
__s32 serial_puts(const char *string)
{
	//ASSERT(string != NULL);
	
	if(0 == serial_inited_flag){
	    return FAIL;
	}

	while(*string != '\0')
	{
		if(*string == '\n')
		{
			// if current character is '\n', 
			// insert output with '\r'.
			serial_put_char('\r');
		}
		serial_put_char(*string++);
	}
	
	return OK;
}


__u32 serial_gets(char* buf, __u32 n)
{
	__u32 i;
	char c;
	
	if(0 == serial_inited_flag){
	    return FAIL;
	}

	for (i=0; i<n; i++) {
		c = serial_get_char();
		if (c == 0)
			break;
		buf[i] = c;
	}
	return i+1;
}
