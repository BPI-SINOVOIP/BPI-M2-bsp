/*
 * Sunxi Camera CSI1 register define
 * Author:raymonxiu
*/
#ifndef _SUNXI_CSI1_REG_H_
#define _SUNXI_CSI1_REG_H_

#define  W(addr, val)   writel(val, addr)
#define  R(addr)        readl(addr)
#define  S(addr,bit)	writel(readl(addr)|bit,addr)
#define  C(addr,bit)	writel(readl(addr)&(~bit),addr)


#define CSI1_REGS_BASE        0X01cb3000
#define CSI1_REG_SIZE 				0x1000

#define SW_INTC_IRQNO_CSI1			AW_IRQ_CSI1

#define CSI_EN           (0x00)
#define CSI_IF_CFG       (0x04)
#define CSI_CAP          (0x08)
#define CSI_SYNC_CNT		 (0x0c)
#define CSI_FIFO_THRS		 (0x10)
                         
#define CSI_PTN_LEN			 (0x30)
#define CSI_PTN_ADDR		 (0x34)
                         
//CH                     
#define CSI_CFG					 (0x44)
#define CSI_SCALE        (0x4C)
#define CSI_BUF0_A       (0x50)
#define CSI_BUF1_A       (0x58)
#define CSI_BUF2_A       (0x60)
#define CSI_STATUS       (0x6C)
#define CSI_INT_EN       (0x70)
#define CSI_INT_STATUS   (0x74)
#define CSI_RESIZE_H     (0x80)
#define CSI_RESIZE_V     (0x84)
#define CSI_BUF_LENGTH   (0x88)
#define CSI_FLIP_SIZE		 (0x8C)
#define CSI_FRM_CLK_CNT	 (0x90)
#define CSI_ACC_CLK_CNT	 (0x94)

#define CSI_CH0					 (0x000)
#define CSI_CH1					 (0x100)
#define CSI_CH2					 (0x200)
#define CSI_CH3					 (0x300)

#endif  /* _CSI_H_ */
