/*
 * (C) Copyright 2007-2012
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef	_DMAC_I_H_
#define	_DMAC_I_H_

#define CFG_SW_DMA_NORMAL_MAX       8
#define CFG_SW_DMA_DEDICATE_MAX     8

#define DMAC_REGS_BASE				0x01c02000


#define CFG_SW_DMA_NORMAL_BASE              (DMAC_REGS_BASE + 0x100              )
#define CFS_SW_DMA_NORMAL0                  (CFG_SW_DMA_NORMAL_BASE + 0x20 * 0   )
#define CFS_SW_DMA_NORMAL1                  (CFG_SW_DMA_NORMAL_BASE + 0x20 * 1   )
#define CFS_SW_DMA_NORMAL2                  (CFG_SW_DMA_NORMAL_BASE + 0x20 * 2   )
#define CFS_SW_DMA_NORMAL3                  (CFG_SW_DMA_NORMAL_BASE + 0x20 * 3   )
#define CFS_SW_DMA_NORMAL4                  (CFG_SW_DMA_NORMAL_BASE + 0x20 * 4   )
#define CFS_SW_DMA_NORMAL5                  (CFG_SW_DMA_NORMAL_BASE + 0x20 * 5   )
#define CFS_SW_DMA_NORMAL6                  (CFG_SW_DMA_NORMAL_BASE + 0x20 * 6   )
#define CFS_SW_DMA_NORMAL7                  (CFG_SW_DMA_NORMAL_BASE + 0x20 * 7   )

#define CFG_SW_DMA_DEDICATE_BASE            (DMAC_REGS_BASE + 0x300               )
#define CFG_SW_DMA_DEDICATE0                (CFG_SW_DMA_DEDICATE_BASE + 0x20 * 0 )
#define CFG_SW_DMA_DEDICATE1                (CFG_SW_DMA_DEDICATE_BASE + 0x20 * 1 )
#define CFG_SW_DMA_DEDICATE2                (CFG_SW_DMA_DEDICATE_BASE + 0x20 * 2 )
#define CFG_SW_DMA_DEDICATE3                (CFG_SW_DMA_DEDICATE_BASE + 0x20 * 3 )
#define CFG_SW_DMA_DEDICATE4                (CFG_SW_DMA_DEDICATE_BASE + 0x20 * 4 )
#define CFG_SW_DMA_DEDICATE5                (CFG_SW_DMA_DEDICATE_BASE + 0x20 * 5 )
#define CFG_SW_DMA_DEDICATE6                (CFG_SW_DMA_DEDICATE_BASE + 0x20 * 6 )
#define CFG_SW_DMA_DEDICATE7                (CFG_SW_DMA_DEDICATE_BASE + 0x20 * 7 )

#define CFG_SW_DMA_OTHER_BASE               (DMAC_REGS_BASE + 0x300 + 0x18       )
#define CFG_SW_DMA_DEDICATE0_OTHER          (CFG_SW_DMA_OTHER_BASE + 0x20 * 0    )
#define CFG_SW_DMA_DEDICATE1_OTHER          (CFG_SW_DMA_OTHER_BASE + 0x20 * 1    )
#define CFG_SW_DMA_DEDICATE2_OTHER          (CFG_SW_DMA_OTHER_BASE + 0x20 * 2    )
#define CFG_SW_DMA_DEDICATE3_OTHER          (CFG_SW_DMA_OTHER_BASE + 0x20 * 3    )
#define CFG_SW_DMA_DEDICATE4_OTHER          (CFG_SW_DMA_OTHER_BASE + 0x20 * 4    )
#define CFG_SW_DMA_DEDICATE5_OTHER          (CFG_SW_DMA_OTHER_BASE + 0x20 * 5    )
#define CFG_SW_DMA_DEDICATE6_OTHER          (CFG_SW_DMA_OTHER_BASE + 0x20 * 6    )
#define CFG_SW_DMA_DEDICATE7_OTHER          (CFG_SW_DMA_OTHER_BASE + 0x20 * 7    )

struct sw_dma
{
    volatile unsigned int config;           /* DMA���ò���              */
    volatile unsigned int src_addr;         /* DMA����Դ��ַ            */
    volatile unsigned int dst_addr;         /* DMA����Ŀ�ĵ�ַ          */
    volatile unsigned int bytes;            /* DMA�����ֽ���            */
};

typedef volatile struct sw_dma *sw_dma_t;

struct sw_dma_other
{
    volatile unsigned int src_data_block_size :8;  
    volatile unsigned int src_wait_cyc		  :8;    
	volatile unsigned int dst_data_block_size :8;  
    volatile unsigned int dst_wait_cyc		  :8;     
};

typedef volatile struct sw_dma_other *sw_dma_other_t;

struct dma_irq_handler
{
	void                *m_data;
	void (*m_func)( void * data);
};

typedef struct sw_dma_channal_set
{
    unsigned int            used;           /* DMA�Ƿ�ʹ��            */
      signed int            channalNo;      /* DMAͨ�����              */
    sw_dma_t                channal;        /* DMAͨ��                  */
    sw_dma_other_t          other;          /* DMA��������              */
	struct dma_irq_handler  dma_func;
}
sw_dma_channal_set_t;


typedef struct __ndma_config_set
{
    unsigned int      src_drq_type     : 5;            //Դ��ַ�洢���ͣ���DRAM, SPI,NAND�ȣ��μ�  __ndma_drq_type_t
    unsigned int      src_addr_type    : 1;            //ԭ��ַ���ͣ�����������߲���  0:����ģʽ  1:���ֲ���
    unsigned int      src_secure       : 1;            //source secure  0:secure  1:not secure
    unsigned int      src_burst_length : 2;            //����һ��burst��� 0:1   1:4   2:8
    unsigned int      src_data_width   : 2;            //���ݴ����ȣ�0:һ�δ���8bit��1:һ�δ���16bit��2:һ�δ���32bit��3:����
    unsigned int      reserved0        : 5;
    unsigned int      dst_drq_type     : 5;            //Ŀ�ĵ�ַ�洢���ͣ���DRAM, SPI,NAND��
    unsigned int      dst_addr_type    : 1;            //Ŀ�ĵ�ַ���ͣ�����������߲���  0:����ģʽ  1:���ֲ���
    unsigned int      dst_secure       : 1;            //dest secure  0:secure  1:not secure
    unsigned int      dst_burst_length : 2;            //����һ��burst��� ��0��Ӧ��1����1��Ӧ��4,
    unsigned int      dst_data_width   : 2;            //���ݴ����ȣ�0:һ�δ���8bit��1:һ�δ���16bit��2:һ�δ���32bit��3:����
    unsigned int      wait_state       : 3;            //�ȴ�ʱ�Ӹ��� ѡ��Χ��0-7
    unsigned int      continuous_mode  : 1;            //ѡ����������ģʽ 0:����һ�μ����� 1:�������䣬��һ��DMA������������¿�ʼ����
    unsigned int      reserved1        : 1;
}
__ndma_config_t;

typedef struct __ddma_config_set
{
    unsigned int      src_drq_type     : 5;            //Դ��ַ�洢���ͣ���DRAM, SPI,NAND�ȣ��μ�  __ddma_src_type_t
    unsigned int      src_addr_type    : 2;            //ԭ��ַ���ͣ�����������߲���  0:����ģʽ  1:���ֲ���  2:Hģʽ  3:Vģʽ
    unsigned int      src_burst_length : 2;            //����һ��burst��� ��0��Ӧ��1����1��Ӧ��4,
    unsigned int      src_data_width   : 2;            //���ݴ����ȣ�0:һ�δ���8bit��1:һ�δ���16bit��2:һ�δ���32bit��3:����
    unsigned int      reserved0        : 5;
    unsigned int      dst_drq_type     : 5;            //Ŀ�ĵ�ַ�洢���ͣ���DRAM, SPI,NAND��, �μ�  __ddma_dst_type_t
    unsigned int      dst_addr_type    : 2;            //Ŀ�ĵ�ַ���ͣ�����������߲��� 0:����ģʽ  1:���ֲ���  2:Hģʽ  3:Vģʽ
    unsigned int      dst_burst_length : 2;            //����һ��burst��� ��0��Ӧ��1����1��Ӧ��4,
    unsigned int      dst_data_width   : 2;            //���ݴ����ȣ�0:һ�δ���8bit��1:һ�δ���16bit��2:һ�δ���32bit��3:����
    unsigned int      reserved1        : 3;
    unsigned int      continuous_mode  : 1;            //ѡ����������ģʽ 0:����һ�μ����� 1:�������䣬��һ��DMA������������¿�ʼ����
    unsigned int      reserved2        : 1;
}
__ddma_config_t;

#endif	/* _EGON2_DMAC_I_H_ */

