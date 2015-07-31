/*
 * arch/arm/mach-sun6i/dram-freq/mdfs/mdfs.c
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 * Pan Nan <pannan@reuuimllatech.com>
 *
 * SUN6I dram frequency dynamic scaling driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <stdarg.h>
#include "mdfs.h"

static void serial_put_char(char c)
{
	while (!(readb(SUART_USR) & 2));
	writeb(c, SUART_THR);
}

__s32 serial_puts(const char *string)
{
	while (*string != '\0') {
		if (*string == '\n') {
			// if current character is '\n',
			// insert output with '\r'.
			serial_put_char('\r');
		}
		serial_put_char(*string++);
	}

	return 0;
}

size_t strlen(const char *s)
{
	const char *sc;

	for (sc = s; *sc != '\0'; ++sc)
	{
		/* nothing */
		;
	}
	return sc - s;
}

char *strcpy(char *dest, const char *src)
{
	char *tmp = dest;

	while ((*dest++ = *src++) != '\0')
	{
		/* nothing */
		;
	}
	return tmp;
}

char *itoa(int value, char *string, int radix)
{
	char stack[16];
	int  negative = 0;			//defualt is positive value
	int  i;
	int  j;
	char digit_string[] = "0123456789ABCDEF";

	if(value == 0)
	{
		//zero
		string[0] = '0';
		string[1] = '\0';
		return string;
	}

	if(value < 0)
	{
		//'value' is negative, convert to postive first
		negative = 1;
		value = -value ;
	}

	for(i = 0; value > 0; ++i)
	{
		// characters in reverse order are put in 'stack'.
		stack[i] = digit_string[value % radix];
		value /= radix;
	}

	//restore reversed order result to user string
    j = 0;
	if(negative)
	{
		//add sign at first charset.
		string[j++] = '-';
	}
	for(--i; i >= 0; --i, ++j)
	{
		string[j] = stack[i];
	}
	//must end with '\0'.
	string[j] = '\0';

	return string;
}

char *utoa(unsigned int value, char *string, int radix)
{
	char stack[16];
	int  i;
	int  j;
	char digit_string[] = "0123456789ABCDEF";

	if(value == 0)
	{
		//zero
		string[0] = '0';
		string[1] = '\0';
		return string;
	}

	for(i = 0; value > 0; ++i)
	{
		// characters in reverse order are put in 'stack'.
		stack[i] = digit_string[value % radix];
		value /= radix;
	}

	//restore reversed order result to user string
    for(--i, j = 0; i >= 0; --i, ++j)
	{
		string[j] = stack[i];
	}
	//must end with '\0'.
	string[j] = '\0';

	return string;
}


char *strncat(char *dest, const char *src, size_t count)
{
	char *tmp = dest;

	if (count)
	{
		while (*dest)
		{
			dest++;
		}
		while ((*dest++ = *src++) != 0)
		{
			if (--count == 0)
			{
				*dest = '\0';
				break;
			}
		}
	}
	return tmp;
}

__s32 print_align(char *string, __s32 len, __s32 align)
{
	//fill with space ' ' when align request,
	//the max align length is 16 byte.
	char fill_ch[] = "                ";
	if (len < align)
	{
		//fill at right
		strncat(string, fill_ch, align - len);
		return align - len;
	}
	//not fill anything
	return 0;
}

char debugger_buffer[256];
__s32 printk(const char *format, ...)
{
	va_list args;
	char 	string[16];	//align by cpu word
	char 	*pdest;
	char 	*psrc;
	__s32 	align;
	__s32		len = 0;

	pdest = debugger_buffer;
	va_start(args, format);
	while(*format)
	{
		if(*format == '%')
		{
			++format;
			if (('0' < (*format)) && ((*format) <= '9'))
			{
				//we just suport wide from 1 to 9.
				align = *format - '0';
				++format;
			}
			else
			{
				align = 0;
			}
			switch(*format)
			{
				case 'd':
				{
					//int
					itoa(va_arg(args, int), string, 10);
                    len = strlen(string);
                    len += print_align(string, len, align);
                    strcpy(pdest, string);
                    pdest += len;
                    break;
				}
				case 'x':
				case 'p':
				{
					//hex
					utoa(va_arg(args, int), string, 16);
					len = strlen(string);
					len += print_align(string, len, align);
					strcpy(pdest, string);
                    pdest += len;
                    break;
				}
				case 'u':
				{
					//unsigned int
					utoa(va_arg(args, int), string, 10);
                    len = strlen(string);
                    len += print_align(string, len, align);
                    strcpy(pdest, string);
					pdest += len;
					break;
				}
				case 'c':
				{
					//charset, aligned by cpu word
					*pdest = (char)va_arg(args, int);
					break;
				}
				case 's':
				{
					//string
					psrc = va_arg(args, char *);
					strcpy(pdest, psrc);
					pdest += strlen(psrc);
					break;
				}
				default :
				{
					//no-conversion
					*pdest++ = '%';
					*pdest++ = *format;
				}
			}
		}
		else
		{
			*pdest++ = *format;
		}
		//parse next token
		++format;
	}
	va_end(args);

	//must end with '\0'
	*pdest = '\0';
	pdest++;
	serial_puts(debugger_buffer);

	return (pdest - debugger_buffer);
}

void __div0(void)
{
	printk("Attempting division by 0!");
}

void mdfs_memcpy(void *dest, const void *src, int n)
{
    char *tmp = dest;
    const char *s = src;

    if (!dest || !src)
        return;

    while (n--)
        *tmp++ = *s++;

    return;
}

void mdfs_memset(void *s, int c, int n)
{
    char *xs = s;

    if (!s)
        return;

    while (n--)
        *xs++ = c;

    return;
}

int mdfs_start(struct aw_mdfs_info *mdfs)
{
    unsigned int reg_val, tmp, freq_div, dual_channel_en;
    unsigned int *mdfs_table;
    int i;

    dual_channel_en = mdfs->is_dual_channel;
    freq_div = mdfs->div;
    mdfs_table = &mdfs->table[0][0];

    /* wait for whether the past MDFS process has done */
    while (readl(SDR_COM_MDFSCR)&0x1);

    /* move to CFG */
    writel(0x1, SDR_SCTL);
    while ((readl(SDR_SSTAT) & 0x7) != 0x1);
    if (dual_channel_en) {
        writel(0x1, 0x1000 + SDR_SCTL);
        while ((readl(0x1000 + SDR_SSTAT) & 0x7) != 0x1);
    }

    /* pd_idle setting */
    reg_val = readl(SDR_MCFG);
    reg_val &= ~(0xff<<8);
    writel(reg_val, SDR_MCFG);
    if (dual_channel_en) {
        reg_val = readl(0x1000 + SDR_MCFG);
        reg_val &= ~(0xff<<8);
        writel(reg_val, 0x1000 + SDR_MCFG);
    }

    /* set Toggle 1us/100ns REG */
    reg_val  = (((readl(CCM_PLL5_DDR_CTRL)>>8)&0x1f)+1)*24;
    reg_val *= (((readl(CCM_PLL5_DDR_CTRL)>>4)&0x3) + 1);
    tmp = ((readl(CCM_PLL5_DDR_CTRL)>>0)&0x3) + 1;
    reg_val /= (((readl(CCM_PLL5_DDR_CTRL)>>0)&0x3) + 1);
    reg_val /= freq_div;
    writel(reg_val, SDR_TOGCNT1U);                  //1us
    if (dual_channel_en) {
        writel(reg_val, 0x1000 + SDR_TOGCNT1U);     //1us
    }
    reg_val /= 10;
    writel(reg_val, SDR_TOGCNT100N);                //100ns
    if (dual_channel_en) {
        writel(reg_val, 0x1000 + SDR_TOGCNT100N);   //100ns
    }

    /* move to GO */
    writel(0x2, SDR_SCTL);
    while ((readl(SDR_SSTAT) & 0x7) != 0x3);
    if (dual_channel_en) {
        writel(0x2, 0x1000 + SDR_SCTL);
        while ((readl(0x1000 + SDR_SSTAT) & 0x7) != 0x3);
    }

    /* set DRAM middle & destination clock */
    reg_val = readl(CCM_DRAMCLK_CFG_CTRL);
    reg_val &= ~(0x1<<16);
    reg_val &= ~(0xf<<8);
    reg_val |= (freq_div-1)<<8;
    writel(reg_val, CCM_DRAMCLK_CFG_CTRL);

    /* set Master enable and Ready mask */
    reg_val = 0x3ff00001;
    writel(reg_val, SDR_COM_MDFSMER);
    reg_val = 0xfffffff8;
    writel(reg_val, SDR_COM_MDFSMRMR);

    /* set MDFS timing parameter */
    reg_val = 0x258;                                //3us/5ns = 600
    writel(reg_val, SDR_COM_MDFSTR0);
    reg_val = 102400*freq_div;
    tmp = (((readl(CCM_PLL5_DDR_CTRL)>>8)&0x1f)+1)*24;
    reg_val /= tmp;
    reg_val ++;
    writel(reg_val, SDR_COM_MDFSTR1);               //512*200/sclk
    reg_val = 0x80;
    writel(reg_val, SDR_COM_MDFSTR2);               //fixed value : 128

    /* set MDFS DQS Gate Configuration */
    for (i=0;i<8;i++) {
        writel(*(mdfs_table + (8*(freq_div-1)) + i), SDR_COM_MDFSGCR + 4*i);
    }

    /* start hardware MDFS */
    reg_val = readl(SDR_COM_MDFSCR);
    reg_val >>= 2;
    reg_val &= ((0x1<<4) | (0x1<<8));
    reg_val |= 0x5 | (0x2u<<30);
    tmp = (((readl(CCM_PLL5_DDR_CTRL)>>8)&0x1f)+1)*24;
    tmp *= (((readl(CCM_PLL5_DDR_CTRL)>>4)&0x3) + 1);
    tmp /= (((readl(CCM_PLL5_DDR_CTRL)>>0)&0x3) + 1);
    if ((tmp/freq_div) < 200)
        reg_val |= 0x1<<10;
    if ((tmp/freq_div) <= 120)
        reg_val |= 0x1<<6;
    writel(reg_val, SDR_COM_MDFSCR);

	/* wait for whether the past MDFS process is done */
	while (readl(SDR_COM_MDFSCR)&0x1);

    /* move to CFG */
    writel(0x1, SDR_SCTL);
    while ((readl(SDR_SSTAT) & 0x7) != 0x1);
    if (dual_channel_en) {
        writel(0x1, 0x1000 + SDR_SCTL);
        while ((readl(0x1000 + SDR_SSTAT) & 0x7) != 0x1);
    }

    /* pd_idle setting */
    reg_val = readl(SDR_MCFG);
    reg_val |= (0x10<<8);
    writel(reg_val, SDR_MCFG);
    if (dual_channel_en) {
        reg_val = readl(0x1000 + SDR_MCFG);
        reg_val |= (0x10<<8);
        writel(reg_val, 0x1000 + SDR_MCFG);
    }

    /* move to GO */
    writel(0x2, SDR_SCTL);
    while ((readl(SDR_SSTAT) & 0x7) != 0x3);
    if (dual_channel_en) {
        writel(0x2, 0x1000 + SDR_SCTL);
        while ((readl(0x1000 + SDR_SSTAT) & 0x7) != 0x3);
    }

	return 0;
}
