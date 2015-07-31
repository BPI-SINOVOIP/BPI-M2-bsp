#include <asm/delay.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <mach/hardware.h>
#include <mach/platform.h>
#include <mach/timer.h>
#include <linux/export.h>
#include <linux/jiffies.h>

/*
 * Since we calibrate only once at boot, this
 * function should be set once at boot and not changed
 */
#define TIME_NUM	1
#define MAX_COUNTER 0xffffffff
#define CYCLE_NSEC	42
#define TIMER_INTERVAL(x) (0x14 + (x) * 0x10)
#define TIMER_CURRENTVAL(x) (0x18 + (x) * 0x10)
#define TIMER_CTL(x)	(0x10 + (x) * 0x10)

static void aw_delay(unsigned long);
static unsigned long read_cur_counter(void);

void (*delay_fn)(unsigned long n) = __udelay;
EXPORT_SYMBOL(delay_fn);

/*
 * return nsec count.
 */

void use_time_delay(void)
{
#if (TIME_NUM != 0)
	unsigned long irq_reg;

	irq_reg = readl(IO_ADDRESS(AW_TIMER_BASE) + 0x00);
	writel(irq_reg & (~0x02), IO_ADDRESS(AW_TIMER_BASE) + 0x00);

	writel(MAX_COUNTER, IO_ADDRESS(AW_TIMER_BASE) + TIMER_INTERVAL(TIME_NUM));
	writel(0x07, IO_ADDRESS(AW_TIMER_BASE) + TIMER_CTL(TIME_NUM));
#endif

	delay_fn = aw_delay;
	printk("[aw_delay]: It is use use_time_delay function!\n");
}

static unsigned long read_cur_counter(void)
{
	return readl(IO_ADDRESS(AW_TIMER_BASE) + TIMER_CURRENTVAL(TIME_NUM));
}

static void aw_delay(unsigned long usec)
{
	unsigned long old, new, cur = 0;
	unsigned long loops_nsec = 1000 * usec;
	unsigned long start;

	/*
	 * Time currentval is down-counter.
	 */
	start = jiffies;
	old = read_cur_counter();
	for (;;) {
		new = read_cur_counter();
		if (new > old){
			cur += (MAX_COUNTER - new + old);
		} else {
			cur += (old - new);
		}
		old = new;
		if ((cur * CYCLE_NSEC) >= loops_nsec)
			break;
		/*
		 * If it throungh a period, shoule be out.
		 */
		if (jiffies - start > 0)
			break;
	}
}

