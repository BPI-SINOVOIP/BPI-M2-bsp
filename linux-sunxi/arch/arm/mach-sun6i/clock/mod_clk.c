/*
 *  arch/arm/mach-sun6i/clock/ccmu/ccm_mod_clk.c
 *
 * Copyright 2012 (c) Allwinner
 * kevin (kevin@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <mach/clock.h>
#include "ccm_i.h"

struct module0_div_tbl{
    __u8    FactorM;
    __u8    FactorN;
    __u16   Div;
};
static struct module0_div_tbl module0_clk_div_tbl[] = {
    {0 , 0, 1  },
    {1 , 0, 2  },
    {2 , 0, 3  },
    {3 , 0, 4  },
    {4 , 0, 5  },
    {5 , 0, 6  },
    {6 , 0, 7  },
    {7 , 0, 8  },
    {8 , 0, 9  },
    {9 , 0, 10 },
    {10, 0, 11 },
    {11, 0, 12 },
    {12, 0, 13 },
    {13, 0, 14 },
    {14, 0, 15 },
    {15, 0, 16 },
    {8 , 1, 18 },
    {9 , 1, 20 },
    {10, 1, 22 },
    {11, 1, 24 },
    {12, 1, 26 },
    {13, 1, 28 },
    {14, 1, 30 },
    {15, 1, 32 },
    {8 , 2, 36 },
    {9 , 2, 40 },
    {10, 2, 44 },
    {11, 2, 48 },
    {12, 2, 52 },
    {13, 2, 56 },
    {14, 2, 60 },
    {15, 2, 64 },
    {8 , 3, 72 },
    {9 , 3, 80 },
    {10, 3, 88 },
    {11, 3, 96 },
    {12, 3, 104},
    {13, 3, 112},
    {14, 3, 120},
    {15, 3, 128},
};
static struct module0_div_tbl mmc_clk_div_tbl[] = {
    {0 , 0, 1  },
    {1 , 0, 2  },
    {2 , 0, 3  },
    {3 , 0, 4  },
    {4 , 0, 5  },
    {5 , 0, 6  },
    {6 , 0, 7  },
    {7 , 0, 8  },
    {9 , 0, 9  },
    {9 , 0, 10 },
    {11, 0, 11 },
    {11, 0, 12 },
    {13, 0, 13 },
    {13, 0, 14 },
    {15, 0, 15 },
    {15, 0, 16 },
    {9 , 1, 18 },
    {9 , 1, 20 },
    {11, 1, 22 },
    {11, 1, 24 },
    {13, 1, 26 },
    {13, 1, 28 },
    {15, 1, 30 },
    {15, 1, 32 },
    {9 , 2, 36 },
    {9 , 2, 40 },
    {11, 2, 44 },
    {11, 2, 48 },
    {13, 2, 52 },
    {13, 2, 56 },
    {15, 2, 60 },
    {15, 2, 64 },
    {9 , 3, 72 },
    {9 , 3, 80 },
    {11, 3, 88 },
    {11, 3, 96 },
    {13, 3, 104},
    {13, 3, 112},
    {15, 3, 120},
    {15, 3, 128},
};
struct onewire_div_tbl{
    __u8    FactorM;
    __u8    FactorN;
    __u16   Div;
};
static struct onewire_div_tbl onewire_clk_div_tbl[] = {
    {0,  0, 1   },
    {1,  0, 2   },
    {2,  0, 3   },
    {3,  0, 4   },
    {4,  0, 5   },
    {5,  0, 6   },
    {6,  0, 7   },
    {7,  0, 8   },
    {8,  0, 9   },
    {9,  0, 10  },
    {10, 0, 11  },
    {11, 0, 12  },
    {12, 0, 13  },
    {13, 0, 14  },
    {14, 0, 15  },
    {15, 0, 16  },
    {16, 0, 17  },
    {17, 0, 18  },
    {18, 0, 19  },
    {19, 0, 20  },
    {20, 0, 21  },
    {21, 0, 22  },
    {22, 0, 23  },
    {23, 0, 24  },
    {24, 0, 25  },
    {25, 0, 26  },
    {26, 0, 27  },
    {27, 0, 28  },
    {28, 0, 29  },
    {29, 0, 30  },
    {30, 0, 31  },
    {31, 0, 32  },
    {16, 1, 34  },
    {17, 1, 36  },
    {18, 1, 38  },
    {19, 1, 40  },
    {20, 1, 42  },
    {21, 1, 44  },
    {22, 1, 46  },
    {23, 1, 48  },
    {24, 1, 50  },
    {25, 1, 52  },
    {26, 1, 54  },
    {27, 1, 56  },
    {28, 1, 58  },
    {29, 1, 60  },
    {30, 1, 62  },
    {31, 1, 64  },
    {16, 2, 68  },
    {17, 2, 72  },
    {18, 2, 76  },
    {19, 2, 80  },
    {20, 2, 84  },
    {21, 2, 88  },
    {22, 2, 92  },
    {23, 2, 96  },
    {24, 2, 100 },
    {25, 2, 104 },
    {26, 2, 108 },
    {27, 2, 112 },
    {28, 2, 116 },
    {29, 2, 120 },
    {30, 2, 124 },
    {31, 2, 128 },
    {16, 3, 136 },
    {17, 3, 144 },
    {18, 3, 152 },
    {19, 3, 160 },
    {20, 3, 168 },
    {21, 3, 176 },
    {22, 3, 184 },
    {23, 3, 192 },
    {24, 3, 200 },
    {25, 3, 208 },
    {26, 3, 216 },
    {27, 3, 224 },
    {28, 3, 232 },
    {29, 3, 240 },
    {30, 3, 248 },
    {31, 3, 256 },
};

static inline __aw_ccu_clk_id_e _get_module0_clk_src(volatile __ccmu_module0_clk_t *reg)
{
    if(reg->ClkSrc == 0)
        return AW_SYS_CLK_HOSC;
    else if(reg->ClkSrc == 1)
        return AW_SYS_CLK_PLL6;
    else
        return AW_SYS_CLK_NONE;
}
static inline __s32 _set_module0_clk_src(volatile __ccmu_module0_clk_t *reg, __aw_ccu_clk_id_e parent)
{
    if(parent == AW_SYS_CLK_HOSC)
        reg->ClkSrc = 0;
    else if(parent == AW_SYS_CLK_PLL6)
        reg->ClkSrc = 1;
    else
        return -1;
    return 0;
}


static inline __s32 _get_module1_clk_src(volatile __ccmu_module1_clk_t *reg)
{
    if(reg->ClkSrc == 0)
        return AW_SYS_CLK_PLL2X8;
    else if(reg->ClkSrc == 3)
        return AW_SYS_CLK_PLL2;
    else
        return AW_SYS_CLK_NONE;
}
static inline __s32 _set_module1_clk_src(volatile __ccmu_module1_clk_t *reg, __aw_ccu_clk_id_e parent)
{
    if(parent == AW_SYS_CLK_PLL2X8)
        reg->ClkSrc = 0;
    else if(parent == AW_SYS_CLK_PLL2)
        reg->ClkSrc = 3;
    else
        return -1;
    return 0;
}


#define DE_BE_INDX          (0)
#define DE_FE_INDX          (1)
#define DE_MP_INDX          (2)
#define LCD_CH0_INDX        (3)
#define LCD_CH1_INDX        (4)


static inline __s32 _get_disp_clk_src(volatile __ccmu_disp_clk_t *reg, int index)
{
    switch(index){
        case DE_BE_INDX:
        case DE_FE_INDX:
        {
            if(reg->ClkSrc == 0)
                return AW_SYS_CLK_PLL3;
            else if(reg->ClkSrc == 1)
                return AW_SYS_CLK_PLL7;
            else if(reg->ClkSrc == 2)
                return AW_SYS_CLK_PLL6x2;
            else if(reg->ClkSrc == 3)
                return AW_SYS_CLK_PLL8;
            else if(reg->ClkSrc == 4)
                return AW_SYS_CLK_PLL9;
            else if(reg->ClkSrc == 5)
                return AW_SYS_CLK_PLL10;
            else
                return AW_SYS_CLK_NONE;
        }

        case DE_MP_INDX:
        {
            if(reg->ClkSrc == 0)
                return AW_SYS_CLK_PLL3;
            else if(reg->ClkSrc == 1)
                return AW_SYS_CLK_PLL7;
            else if(reg->ClkSrc == 2)
                return AW_SYS_CLK_PLL9;
            else if(reg->ClkSrc == 3)
                return AW_SYS_CLK_PLL10;
            else
                return AW_SYS_CLK_NONE;
        }

        case LCD_CH0_INDX:
        {
            if(reg->ClkSrc == 0)
                return AW_SYS_CLK_PLL3;
            else if(reg->ClkSrc == 1)
                return AW_SYS_CLK_PLL7;
            else if(reg->ClkSrc == 2)
                return AW_SYS_CLK_PLL3X2;
            else if(reg->ClkSrc == 3)
                return AW_SYS_CLK_PLL7X2;
            else if(reg->ClkSrc == 4)
                return AW_SYS_CLK_MIPIPLL;
            else
                return AW_SYS_CLK_NONE;
        }

        case LCD_CH1_INDX:
        {
            if(reg->ClkSrc == 0)
                return AW_SYS_CLK_PLL3;
            else if(reg->ClkSrc == 1)
                return AW_SYS_CLK_PLL7;
            else if(reg->ClkSrc == 2)
                return AW_SYS_CLK_PLL3X2;
            else if(reg->ClkSrc == 3)
                return AW_SYS_CLK_PLL7X2;
            else
                return AW_SYS_CLK_NONE;
        }

        default:
            return AW_SYS_CLK_NONE;
    }

    return AW_SYS_CLK_NONE;
}

static inline __s32 _set_disp_clk_src(volatile __ccmu_disp_clk_t *reg, __aw_ccu_clk_id_e parent, int index)
{
    switch(index){
        case DE_BE_INDX:
        case DE_FE_INDX:
        {
            if(parent == AW_SYS_CLK_PLL3)
                reg->ClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL7)
                reg->ClkSrc = 1;
            else if(parent == AW_SYS_CLK_PLL6x2)
                reg->ClkSrc = 2;
            else if(parent == AW_SYS_CLK_PLL8)
                reg->ClkSrc = 3;
            else if(parent == AW_SYS_CLK_PLL9)
                reg->ClkSrc = 4;
            else if(parent == AW_SYS_CLK_PLL10)
                reg->ClkSrc = 5;
            else
                return -1;
	    return 0;
        }

        case DE_MP_INDX:
        {
            if(parent == AW_SYS_CLK_PLL3)
                reg->ClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL7)
                reg->ClkSrc = 1;
            else if(parent == AW_SYS_CLK_PLL9)
                reg->ClkSrc = 2;
            else if(parent == AW_SYS_CLK_PLL10)
                reg->ClkSrc = 3;
            else
                return -1;
	    return 0;
        }

        case LCD_CH0_INDX:
        {
            if(parent == AW_SYS_CLK_PLL3)
                reg->ClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL7)
                reg->ClkSrc = 1;
            else if(parent == AW_SYS_CLK_PLL3X2)
                reg->ClkSrc = 2;
            else if(parent == AW_SYS_CLK_PLL7X2)
                reg->ClkSrc = 3;
            else if(parent == AW_SYS_CLK_MIPIPLL)
                reg->ClkSrc = 4;
            else
                return -1;
	    return 0;
        }

        case LCD_CH1_INDX:
        {
            if(parent == AW_SYS_CLK_PLL3)
                reg->ClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL7)
                reg->ClkSrc = 1;
            else if(parent == AW_SYS_CLK_PLL3X2)
                reg->ClkSrc = 2;
            else if(parent == AW_SYS_CLK_PLL7X2)
                reg->ClkSrc = 3;
            else
                return -1;
	    return 0;
        }

        default:
            return -1;
    }

     return -1;
}


static inline __aw_ccu_clk_onff_e _get_module_clk_status(volatile __ccmu_module_clk_t *reg)
{
    return reg->ClkGate? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
}
static inline __s32 _set_module_clk_status(volatile __ccmu_module0_clk_t *reg, __aw_ccu_clk_onff_e status)
{
    reg->ClkGate = (status == AW_CCU_CLK_OFF)? 0 : 1;
    return 0;
}


static inline __u32 _get_module0_clk_rate(volatile __ccmu_module0_clk_t *reg)
{
    return (1<<reg->DivN)*(reg->DivM+1);
}
static inline __s32 _set_module0_clk_rate(volatile __ccmu_module0_clk_t *reg, __s64 rate)
{
    int     low, high;

    low = 0;
    high = sizeof(module0_clk_div_tbl)/sizeof(struct module0_div_tbl) - 1;

    while(low<=high){
        if(module0_clk_div_tbl[(low+high)/2].Div < rate) {
            low = (low+high)/2+1;
        } else if(module0_clk_div_tbl[(low+high)/2].Div > rate) {
            high = (low+high)/2-1;
        } else {
            low = (low+high)/2;
            reg->DivM = module0_clk_div_tbl[low].FactorM;
            reg->DivN = module0_clk_div_tbl[low].FactorN;
            return 0;
        }
    }

    CCU_ERR("%s:%d, set rate of (%x) to %llu failed", __FILE__, __LINE__, (int)reg, rate);
    return -1;
}
static inline __s32 _set_mmc_clk_rate(volatile __ccmu_module0_clk_t *reg, __s64 rate)
{
    int     low, high;

    low = 0;
    high = sizeof(mmc_clk_div_tbl)/sizeof(struct module0_div_tbl) - 1;

    while(low<=high){
        if(mmc_clk_div_tbl[(low+high)/2].Div < rate) {
            low = (low+high)/2+1;
        } else if(mmc_clk_div_tbl[(low+high)/2].Div > rate) {
            high = (low+high)/2-1;
        } else {
            low = (low+high)/2;
            reg->DivM = mmc_clk_div_tbl[low].FactorM;
            reg->DivN = mmc_clk_div_tbl[low].FactorN;
            return 0;
        }
    }

    CCU_ERR("%s:%d, set rate of (%x) to %llu failed", __FILE__, __LINE__, (int)reg, rate);
    return -1;
}
static inline __s32 _set_onewire_clk_rate(volatile __ccmu_onewire_clk_reg0050_t *reg, __s64 rate)
{
    int     low, high;

    low = 0;
    high = sizeof(onewire_clk_div_tbl)/sizeof(struct onewire_div_tbl) - 1;

    while(low<=high){
        if(onewire_clk_div_tbl[(low+high)/2].Div < rate) {
            low = (low+high)/2+1;
        } else if(onewire_clk_div_tbl[(low+high)/2].Div > rate) {
            high = (low+high)/2-1;
        } else {
            low = (low+high)/2;
            reg->DivM = onewire_clk_div_tbl[low].FactorM;
            reg->DivN = onewire_clk_div_tbl[low].FactorN;
            return 0;
        }
    }

    CCU_ERR("%s:%d, set rate of (%x) to %llu failed", __FILE__, __LINE__, (int)reg, rate);
    return -1;
}


/*
*********************************************************************************************************
*                           mod_clk_get_parent
*
*Description: get clock parent for module clock;
*
*Arguments  : id        module clock id;
*
*Return     : parent clock id;
*
*Notes      :
*
*********************************************************************************************************
*/
static __aw_ccu_clk_id_e mod_clk_get_parent(__aw_ccu_clk_id_e id)
{
    switch(id)
    {
        case AW_MOD_CLK_NAND0:
            return _get_module0_clk_src(&aw_ccu_reg->Nand0);
        case AW_MOD_CLK_NAND1:
            return _get_module0_clk_src(&aw_ccu_reg->Nand1);
        case AW_MOD_CLK_SDC0:
            return _get_module0_clk_src(&aw_ccu_reg->Sd0);
        case AW_MOD_CLK_SDC1:
            return _get_module0_clk_src(&aw_ccu_reg->Sd1);
        case AW_MOD_CLK_SDC2:
            return _get_module0_clk_src(&aw_ccu_reg->Sd2);
        case AW_MOD_CLK_SDC3:
            return _get_module0_clk_src(&aw_ccu_reg->Sd3);
        case AW_MOD_CLK_TS:
            return _get_module0_clk_src(&aw_ccu_reg->Ts);
        case AW_MOD_CLK_SS:
            return _get_module0_clk_src(&aw_ccu_reg->Ss);
        case AW_MOD_CLK_SPI0:
            return _get_module0_clk_src(&aw_ccu_reg->Spi0);
        case AW_MOD_CLK_SPI1:
            return _get_module0_clk_src(&aw_ccu_reg->Spi1);
        case AW_MOD_CLK_SPI2:
            return _get_module0_clk_src(&aw_ccu_reg->Spi2);
        case AW_MOD_CLK_SPI3:
            return _get_module0_clk_src(&aw_ccu_reg->Spi3);
        case AW_MOD_CLK_I2S0:
            return _get_module1_clk_src(&aw_ccu_reg->I2s0);
        case AW_MOD_CLK_I2S1:
            return _get_module1_clk_src(&aw_ccu_reg->I2s1);
        case AW_MOD_CLK_SPDIF:
            return _get_module1_clk_src(&aw_ccu_reg->Spdif);
        case AW_MOD_CLK_MDFS:
        {
            if(aw_ccu_reg->Mdfs.ClkSrc == 0)
                return AW_SYS_CLK_PLL5;
            else if(aw_ccu_reg->Mdfs.ClkSrc == 1)
                return AW_SYS_CLK_PLL6;
            else
            {
                aw_ccu_reg->Mdfs.ClkSrc = 0;
                return AW_SYS_CLK_PLL5;
            }
        }
        case AW_MOD_CLK_DEBE0:
            return _get_disp_clk_src(&aw_ccu_reg->Be0, DE_BE_INDX);
        case AW_MOD_CLK_DEBE1:
            return _get_disp_clk_src(&aw_ccu_reg->Be1, DE_BE_INDX);
        case AW_MOD_CLK_DEFE0:
            return _get_disp_clk_src(&aw_ccu_reg->Fe0, DE_FE_INDX);
        case AW_MOD_CLK_DEFE1:
            return _get_disp_clk_src(&aw_ccu_reg->Fe1, DE_FE_INDX);
        case AW_MOD_CLK_DEMIX:
            return _get_disp_clk_src(&aw_ccu_reg->Mp, DE_MP_INDX);
        case AW_MOD_CLK_LCD0CH0:
            return _get_disp_clk_src(&aw_ccu_reg->Lcd0Ch0, LCD_CH0_INDX);
        case AW_MOD_CLK_LCD0CH1:
            return _get_disp_clk_src(&aw_ccu_reg->Lcd0Ch1, LCD_CH1_INDX);
        case AW_MOD_CLK_LCD1CH0:
            return _get_disp_clk_src(&aw_ccu_reg->Lcd1Ch0, LCD_CH0_INDX);
        case AW_MOD_CLK_LCD1CH1:
            return _get_disp_clk_src(&aw_ccu_reg->Lcd1Ch1, LCD_CH1_INDX);
        case AW_MOD_CLK_VE:
            return AW_SYS_CLK_PLL4;
        case AW_MOD_CLK_ADDA:
            return _get_module1_clk_src(&aw_ccu_reg->Adda);
        case AW_MOD_CLK_AVS:
            return AW_SYS_CLK_HOSC;
        case AW_MOD_CLK_PS:
            return _get_module1_clk_src(&aw_ccu_reg->Ps);
        case AW_MOD_CLK_MTCACC:
            return _get_module0_clk_src(&aw_ccu_reg->MtcAcc);
        case AW_MOD_CLK_MBUS0:
        {
            if(aw_ccu_reg->MBus0.ClkSrc == 0)
                return AW_SYS_CLK_HOSC;
            else if(aw_ccu_reg->MBus0.ClkSrc == 1)
                return AW_SYS_CLK_PLL6;
            else if(aw_ccu_reg->MBus0.ClkSrc == 2)
                return AW_SYS_CLK_PLL5;
            else
            {
                aw_ccu_reg->MBus0.ClkSrc = 2;
                return AW_SYS_CLK_PLL5;
            }
        }
        case AW_MOD_CLK_MBUS1:
        {
            if(aw_ccu_reg->MBus1.ClkSrc == 0)
                return AW_SYS_CLK_HOSC;
            else if(aw_ccu_reg->MBus1.ClkSrc == 1)
                return AW_SYS_CLK_PLL6;
            else if(aw_ccu_reg->MBus1.ClkSrc == 2)
                return AW_SYS_CLK_PLL5;
            else
            {
                aw_ccu_reg->MBus1.ClkSrc = 2;
                return AW_SYS_CLK_PLL5;
            }
        }
        case AW_MOD_CLK_IEPDRC0:
        {
            if(aw_ccu_reg->IepDrc0.ClkSrc == 0)
                return AW_SYS_CLK_PLL3;
            else if(aw_ccu_reg->IepDrc0.ClkSrc == 1)
                return AW_SYS_CLK_PLL7;
            else if(aw_ccu_reg->IepDrc0.ClkSrc == 3)
                return AW_SYS_CLK_PLL8;
            else if(aw_ccu_reg->IepDrc0.ClkSrc == 4)
                return AW_SYS_CLK_PLL9;
            else if(aw_ccu_reg->IepDrc0.ClkSrc == 5)
                return AW_SYS_CLK_PLL10;
            else

            {
                aw_ccu_reg->IepDrc0.ClkSrc = 0;
                return AW_SYS_CLK_PLL3;
            }
        }
        case AW_MOD_CLK_IEPDRC1:
        {
            if(aw_ccu_reg->IepDrc1.ClkSrc == 0)
                return AW_SYS_CLK_PLL3;
            else if(aw_ccu_reg->IepDrc1.ClkSrc == 1)
                return AW_SYS_CLK_PLL7;
            else if(aw_ccu_reg->IepDrc1.ClkSrc == 3)
                return AW_SYS_CLK_PLL8;
            else if(aw_ccu_reg->IepDrc1.ClkSrc == 4)
                return AW_SYS_CLK_PLL9;
            else if(aw_ccu_reg->IepDrc1.ClkSrc == 5)
                return AW_SYS_CLK_PLL10;
            else

            {
                aw_ccu_reg->IepDrc1.ClkSrc = 0;
                return AW_SYS_CLK_PLL3;
            }
        }
        case AW_MOD_CLK_IEPDEU0:
        {
            if(aw_ccu_reg->IepDeu0.ClkSrc == 0)
                return AW_SYS_CLK_PLL3;
            else if(aw_ccu_reg->IepDeu0.ClkSrc == 1)
                return AW_SYS_CLK_PLL7;
            else if(aw_ccu_reg->IepDeu0.ClkSrc == 3)
                return AW_SYS_CLK_PLL8;
            else if(aw_ccu_reg->IepDeu0.ClkSrc == 4)
                return AW_SYS_CLK_PLL9;
            else if(aw_ccu_reg->IepDeu0.ClkSrc == 5)
                return AW_SYS_CLK_PLL10;
            else

            {
                aw_ccu_reg->IepDeu0.ClkSrc = 0;
                return AW_SYS_CLK_PLL3;
            }
        }
        case AW_MOD_CLK_IEPDEU1:
        {
            if(aw_ccu_reg->IepDeu1.ClkSrc == 0)
                return AW_SYS_CLK_PLL3;
            else if(aw_ccu_reg->IepDeu1.ClkSrc == 1)
                return AW_SYS_CLK_PLL7;
            else if(aw_ccu_reg->IepDeu1.ClkSrc == 3)
                return AW_SYS_CLK_PLL8;
            else if(aw_ccu_reg->IepDeu1.ClkSrc == 4)
                return AW_SYS_CLK_PLL9;
            else if(aw_ccu_reg->IepDeu1.ClkSrc == 5)
                return AW_SYS_CLK_PLL10;
            else

            {
                aw_ccu_reg->IepDeu1.ClkSrc = 0;
                return AW_SYS_CLK_PLL3;
            }
        }
        case AW_MOD_CLK_GPUCORE:
        {
            if(aw_ccu_reg->GpuCore.ClkSrc == 0)
                return AW_SYS_CLK_PLL8;
            else if(aw_ccu_reg->GpuCore.ClkSrc == 2)
                return AW_SYS_CLK_PLL3;
            else if(aw_ccu_reg->GpuCore.ClkSrc == 3)
                return AW_SYS_CLK_PLL7;
            else if(aw_ccu_reg->GpuCore.ClkSrc == 4)
                return AW_SYS_CLK_PLL9;
            else if(aw_ccu_reg->GpuCore.ClkSrc == 5)
                return AW_SYS_CLK_PLL10;
            else

            {
                aw_ccu_reg->GpuCore.ClkSrc = 0;
                return AW_SYS_CLK_PLL8;
            }
        }
        case AW_MOD_CLK_GPUMEM:
        {
            if(aw_ccu_reg->GpuMem.ClkSrc == 0)
                return AW_SYS_CLK_PLL8;
            else if(aw_ccu_reg->GpuMem.ClkSrc == 2)
                return AW_SYS_CLK_PLL3;
            else if(aw_ccu_reg->GpuMem.ClkSrc == 3)
                return AW_SYS_CLK_PLL7;
            else if(aw_ccu_reg->GpuMem.ClkSrc == 4)
                return AW_SYS_CLK_PLL9;
            else if(aw_ccu_reg->GpuMem.ClkSrc == 5)
                return AW_SYS_CLK_PLL10;
            else

            {
                aw_ccu_reg->GpuMem.ClkSrc = 0;
                return AW_SYS_CLK_PLL8;
            }
        }
        case AW_MOD_CLK_GPUHYD:
        {
            if(aw_ccu_reg->GpuHyd.ClkSrc == 0)
                return AW_SYS_CLK_PLL8;
            else if(aw_ccu_reg->GpuHyd.ClkSrc == 2)
                return AW_SYS_CLK_PLL3;
            else if(aw_ccu_reg->GpuHyd.ClkSrc == 3)
                return AW_SYS_CLK_PLL7;
            else if(aw_ccu_reg->GpuHyd.ClkSrc == 4)
                return AW_SYS_CLK_PLL9;
            else if(aw_ccu_reg->GpuHyd.ClkSrc == 5)
                return AW_SYS_CLK_PLL10;
            else

            {
                aw_ccu_reg->GpuHyd.ClkSrc = 0;
                return AW_SYS_CLK_PLL8;
            }
        }
        case AW_MOD_CLK_HDMI:
            if(aw_ccu_reg->Hdmi.ClkSrc == 0)
                return AW_SYS_CLK_PLL3;
            else if(aw_ccu_reg->Hdmi.ClkSrc == 1)
                return AW_SYS_CLK_PLL7;
            else if(aw_ccu_reg->Hdmi.ClkSrc == 2)
                return AW_SYS_CLK_PLL3X2;
            else
                return AW_SYS_CLK_PLL7X2;

        case AW_MOD_CLK_CSI0S:
            if(aw_ccu_reg->Csi0.SClkSrc == 0)
                return AW_SYS_CLK_PLL3;
            else if(aw_ccu_reg->Csi0.SClkSrc == 1)
                return AW_SYS_CLK_PLL7;
            else if(aw_ccu_reg->Csi0.SClkSrc == 2)
                return AW_SYS_CLK_PLL3X2;
            else if(aw_ccu_reg->Csi0.SClkSrc == 3)
                return AW_SYS_CLK_PLL7X2;
            else if(aw_ccu_reg->Csi0.SClkSrc == 4)
                return AW_SYS_CLK_MIPIPLL;
            else if(aw_ccu_reg->Csi0.SClkSrc == 5)
                return AW_SYS_CLK_PLL4;
            else
                return AW_SYS_CLK_NONE;
        case AW_MOD_CLK_CSI0M:
            if(aw_ccu_reg->Csi0.MClkSrc == 0)
                return AW_SYS_CLK_PLL3;
            else if(aw_ccu_reg->Csi0.MClkSrc == 1)
                return AW_SYS_CLK_PLL7;
            else if(aw_ccu_reg->Csi0.MClkSrc == 5)
                return AW_SYS_CLK_HOSC;
            else
                return AW_SYS_CLK_NONE;
        case AW_MOD_CLK_CSI1S:
            if(aw_ccu_reg->Csi1.SClkSrc == 0)
                return AW_SYS_CLK_PLL3;
            else if(aw_ccu_reg->Csi1.SClkSrc == 1)
                return AW_SYS_CLK_PLL7;
            else if(aw_ccu_reg->Csi1.SClkSrc == 2)
                return AW_SYS_CLK_PLL3X2;
            else if(aw_ccu_reg->Csi1.SClkSrc == 3)
                return AW_SYS_CLK_PLL7X2;
            else if(aw_ccu_reg->Csi1.SClkSrc == 4)
                return AW_SYS_CLK_MIPIPLL;
            else if(aw_ccu_reg->Csi1.SClkSrc == 5)
                return AW_SYS_CLK_PLL4;
            else
                return AW_SYS_CLK_NONE;
        case AW_MOD_CLK_CSI1M:
            if(aw_ccu_reg->Csi1.MClkSrc == 0)
                return AW_SYS_CLK_PLL3;
            else if(aw_ccu_reg->Csi1.MClkSrc == 1)
                return AW_SYS_CLK_PLL7;
            else if(aw_ccu_reg->Csi1.MClkSrc == 5)
                return AW_SYS_CLK_HOSC;
            else
                return AW_SYS_CLK_NONE;

        case AW_MOD_CLK_MIPIDSIS:
            if(aw_ccu_reg->MipiDsi.SClkSrc == 0)
                return AW_SYS_CLK_PLL3;
            else if(aw_ccu_reg->MipiDsi.SClkSrc == 1)
                return AW_SYS_CLK_PLL7;
            else if(aw_ccu_reg->MipiDsi.SClkSrc == 2)
                return AW_SYS_CLK_PLL3X2;
            else
                return AW_SYS_CLK_PLL7X2;
        case AW_MOD_CLK_MIPIDSIP:
            if(aw_ccu_reg->MipiDsi.PClkSrc == 0)
                return AW_SYS_CLK_PLL3;
            else if(aw_ccu_reg->MipiDsi.PClkSrc == 1)
                return AW_SYS_CLK_PLL7;
            else if(aw_ccu_reg->MipiDsi.PClkSrc == 2)
                return AW_SYS_CLK_PLL3X2;
            else
                return AW_SYS_CLK_PLL7X2;
        case AW_MOD_CLK_MIPICSIS:
            if(aw_ccu_reg->MipiCsi.SClkSrc == 0)
                return AW_SYS_CLK_PLL3;
            else if(aw_ccu_reg->MipiCsi.SClkSrc == 1)
                return AW_SYS_CLK_PLL7;
            else if(aw_ccu_reg->MipiCsi.SClkSrc == 2)
                return AW_SYS_CLK_PLL3X2;
            else
                return AW_SYS_CLK_PLL7X2;
        case AW_MOD_CLK_MIPICSIP:
            if(aw_ccu_reg->MipiCsi.PClkSrc == 0)
                return AW_SYS_CLK_PLL3;
            else if(aw_ccu_reg->MipiCsi.PClkSrc == 1)
                return AW_SYS_CLK_PLL7;
            else if(aw_ccu_reg->MipiCsi.PClkSrc == 2)
                return AW_SYS_CLK_PLL3X2;
            else
                return AW_SYS_CLK_PLL7X2;

        case AW_MOD_CLK_TWI0:
        case AW_MOD_CLK_TWI1:
        case AW_MOD_CLK_TWI2:
        case AW_MOD_CLK_TWI3:
        case AW_MOD_CLK_UART0:
        case AW_MOD_CLK_UART1:
        case AW_MOD_CLK_UART2:
        case AW_MOD_CLK_UART3:
        case AW_MOD_CLK_UART4:
        case AW_MOD_CLK_UART5:
            return AW_SYS_CLK_APB2;

        case AW_MOD_CLK_R_1WIRE:
        {
            if(aw_cpus_reg->OneWire.ClkSrc == 0) {
                return AW_SYS_CLK_LOSC;
            } else if(aw_cpus_reg->OneWire.ClkSrc == 1) {
                return AW_SYS_CLK_HOSC;
            } else {
                aw_cpus_reg->OneWire.ClkSrc = 0;
                return AW_SYS_CLK_LOSC;
            }
        }
        case AW_MOD_CLK_R_CIR:
        {
            if(aw_cpus_reg->Cir.ClkSrc == 0) {
                return AW_SYS_CLK_LOSC;
            } else if(aw_cpus_reg->Cir.ClkSrc == 1) {
                return AW_SYS_CLK_HOSC;
            } else {
                aw_cpus_reg->Cir.ClkSrc = 0;
                return AW_SYS_CLK_LOSC;
            }
        }

        case AW_MOD_CLK_R_TWI:
        case AW_MOD_CLK_R_UART:
        case AW_MOD_CLK_R_P2WI:
        case AW_MOD_CLK_R_TMR:
        case AW_MOD_CLK_R_PIO:
            return AW_SYS_CLK_APB0;

        default:
            return AW_SYS_CLK_NONE;
    }
}


/*
*********************************************************************************************************
*                           mod_clk_get_status
*
*Description: get module clock on/off status;
*
*Arguments  : id    module clock id;
*
*Return     : result;
*               AW_CCU_CLK_OFF, module clock is off;
*               AW_CCU_CLK_ON,  module clock is on;
*
*Notes      :
*
*********************************************************************************************************
*/
#define GET_CLK_STATUS(reg)     (((__ccmu_module_clk_t *)(reg))->ClkGate? AW_CCU_CLK_ON : AW_CCU_CLK_OFF)
static __aw_ccu_clk_onff_e mod_clk_get_status(__aw_ccu_clk_id_e id)
{
    switch(id)
    {
        case AW_MOD_CLK_NAND0:
            return GET_CLK_STATUS(&aw_ccu_reg->Nand0);
        case AW_MOD_CLK_NAND1:
            return GET_CLK_STATUS(&aw_ccu_reg->Nand1);
        case AW_MOD_CLK_SDC0:
            return GET_CLK_STATUS(&aw_ccu_reg->Sd0);
        case AW_MOD_CLK_SDC1:
            return GET_CLK_STATUS(&aw_ccu_reg->Sd1);
        case AW_MOD_CLK_SDC2:
            return GET_CLK_STATUS(&aw_ccu_reg->Sd2);
        case AW_MOD_CLK_SDC3:
            return GET_CLK_STATUS(&aw_ccu_reg->Sd3);
        case AW_MOD_CLK_TS:
            return GET_CLK_STATUS(&aw_ccu_reg->Ts);
        case AW_MOD_CLK_SS:
            return GET_CLK_STATUS(&aw_ccu_reg->Ss);
        case AW_MOD_CLK_SPI0:
            return GET_CLK_STATUS(&aw_ccu_reg->Spi0);
        case AW_MOD_CLK_SPI1:
            return GET_CLK_STATUS(&aw_ccu_reg->Spi1);
        case AW_MOD_CLK_SPI2:
            return GET_CLK_STATUS(&aw_ccu_reg->Spi2);
        case AW_MOD_CLK_SPI3:
            return GET_CLK_STATUS(&aw_ccu_reg->Spi3);
        case AW_MOD_CLK_I2S0:
            return GET_CLK_STATUS(&aw_ccu_reg->I2s0);
        case AW_MOD_CLK_I2S1:
            return GET_CLK_STATUS(&aw_ccu_reg->I2s1);
        case AW_MOD_CLK_SPDIF:
            return GET_CLK_STATUS(&aw_ccu_reg->Spdif);
        case AW_MOD_CLK_USBPHY0:
            return aw_ccu_reg->Usb.Phy0Gate? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_MOD_CLK_USBPHY1:
            return aw_ccu_reg->Usb.Phy1Gate? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_MOD_CLK_USBPHY2:
            return aw_ccu_reg->Usb.Phy2Gate? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_MOD_CLK_USBOHCI0:
            return aw_ccu_reg->Usb.Phy0Gate? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_MOD_CLK_USBOHCI1:
            return aw_ccu_reg->Usb.Phy0Gate? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_MOD_CLK_USBOHCI2:
            return aw_ccu_reg->Usb.Phy0Gate? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_MOD_CLK_MDFS:
            return GET_CLK_STATUS(&aw_ccu_reg->Mdfs);
        case AW_MOD_CLK_DEBE0:
            return GET_CLK_STATUS(&aw_ccu_reg->Be0);
        case AW_MOD_CLK_DEBE1:
            return GET_CLK_STATUS(&aw_ccu_reg->Be1);
        case AW_MOD_CLK_DEFE0:
            return GET_CLK_STATUS(&aw_ccu_reg->Fe0);
        case AW_MOD_CLK_DEFE1:
            return GET_CLK_STATUS(&aw_ccu_reg->Fe1);
        case AW_MOD_CLK_DEMIX:
            return GET_CLK_STATUS(&aw_ccu_reg->Mp);
        case AW_MOD_CLK_LCD0CH0:
            return GET_CLK_STATUS(&aw_ccu_reg->Lcd0Ch0);
        case AW_MOD_CLK_LCD0CH1:
            return GET_CLK_STATUS(&aw_ccu_reg->Lcd0Ch1);
        case AW_MOD_CLK_LCD1CH0:
            return GET_CLK_STATUS(&aw_ccu_reg->Lcd1Ch0);
        case AW_MOD_CLK_LCD1CH1:
            return GET_CLK_STATUS(&aw_ccu_reg->Lcd1Ch1);
        case AW_MOD_CLK_CSI0S:
            return aw_ccu_reg->Csi0.SClkGate? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_MOD_CLK_CSI0M:
            return aw_ccu_reg->Csi0.MClkGate? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_MOD_CLK_CSI1S:
            return aw_ccu_reg->Csi1.SClkGate? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_MOD_CLK_CSI1M:
            return aw_ccu_reg->Csi1.MClkGate? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_MOD_CLK_VE:
            return aw_ccu_reg->Ve.ClkGate? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_MOD_CLK_ADDA:
            return GET_CLK_STATUS(&aw_ccu_reg->Adda);
        case AW_MOD_CLK_AVS:
            return GET_CLK_STATUS(&aw_ccu_reg->Avs);
        case AW_MOD_CLK_DMIC:
            return GET_CLK_STATUS(&aw_ccu_reg->Dmic);
        case AW_MOD_CLK_HDMI:
            return GET_CLK_STATUS(&aw_ccu_reg->Hdmi);
        case AW_MOD_CLK_HDMI_DDC:
            return aw_ccu_reg->Hdmi.DDCGate? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_MOD_CLK_PS:
            return GET_CLK_STATUS(&aw_ccu_reg->Ps);
        case AW_MOD_CLK_MTCACC:
            return GET_CLK_STATUS(&aw_ccu_reg->MtcAcc);
        case AW_MOD_CLK_MBUS0:
            return GET_CLK_STATUS(&aw_ccu_reg->MBus0);
        case AW_MOD_CLK_MBUS1:
            return GET_CLK_STATUS(&aw_ccu_reg->MBus1);
        case AW_MOD_CLK_MIPIDSIS:
            return aw_ccu_reg->MipiDsi.SClkGate? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_MOD_CLK_MIPIDSIP:
            return aw_ccu_reg->MipiDsi.PClkGate? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_MOD_CLK_MIPICSIS:
            return aw_ccu_reg->MipiCsi.SClkGate? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_MOD_CLK_MIPICSIP:
            return aw_ccu_reg->MipiCsi.PClkGate? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_MOD_CLK_IEPDRC0:
            return GET_CLK_STATUS(&aw_ccu_reg->IepDrc0);
        case AW_MOD_CLK_IEPDRC1:
            return GET_CLK_STATUS(&aw_ccu_reg->IepDrc1);
        case AW_MOD_CLK_IEPDEU0:
            return GET_CLK_STATUS(&aw_ccu_reg->IepDeu0);
        case AW_MOD_CLK_IEPDEU1:
            return GET_CLK_STATUS(&aw_ccu_reg->IepDeu1);
        case AW_MOD_CLK_GPUCORE:
            return GET_CLK_STATUS(&aw_ccu_reg->GpuCore);
        case AW_MOD_CLK_GPUMEM:
            return GET_CLK_STATUS(&aw_ccu_reg->GpuMem);
        case AW_MOD_CLK_GPUHYD:
            return GET_CLK_STATUS(&aw_ccu_reg->GpuHyd);
        case AW_AXI_CLK_DRAM:
            return aw_ccu_reg->AxiGate.Sdram? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_MIPICSI:
            return aw_ccu_reg->AhbGate0.MipiCsi? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_MIPIDSI:
            return aw_ccu_reg->AhbGate0.MipiDsi? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_SS:
            return aw_ccu_reg->AhbGate0.Ss? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_DMA:
            return aw_ccu_reg->AhbGate0.Dma? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_SDMMC0:
            return aw_ccu_reg->AhbGate0.Sd0? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_SDMMC1:
            return aw_ccu_reg->AhbGate0.Sd1? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_SDMMC2:
            return aw_ccu_reg->AhbGate0.Sd2? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_SDMMC3:
            return aw_ccu_reg->AhbGate0.Sd3? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_NAND1:
            return aw_ccu_reg->AhbGate0.Nand1? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_NAND0:
            return aw_ccu_reg->AhbGate0.Nand0? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_SDRAM:
            return aw_ccu_reg->AhbGate0.Dram? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_GMAC:
            return aw_ccu_reg->AhbGate0.Gmac? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_TS:
            return aw_ccu_reg->AhbGate0.Ts? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_HSTMR:
            return aw_ccu_reg->AhbGate0.HsTmr? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_SPI0:
            return aw_ccu_reg->AhbGate0.Spi0? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_SPI1:
            return aw_ccu_reg->AhbGate0.Spi1? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_SPI2:
            return aw_ccu_reg->AhbGate0.Spi2? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_SPI3:
            return aw_ccu_reg->AhbGate0.Spi3? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_OTG:
            return aw_ccu_reg->AhbGate0.Otg? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_EHCI0:
            return aw_ccu_reg->AhbGate0.Ehci0? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_EHCI1:
            return aw_ccu_reg->AhbGate0.Ehci1? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_OHCI0:
            return aw_ccu_reg->AhbGate0.Ohci0? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_OHCI1:
            return aw_ccu_reg->AhbGate0.Ohci1? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_OHCI2:
            return aw_ccu_reg->AhbGate0.Ohci2? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_VE:
            return aw_ccu_reg->AhbGate1.Ve? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_LCD0:
            return aw_ccu_reg->AhbGate1.Lcd0? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_LCD1:
            return aw_ccu_reg->AhbGate1.Lcd1? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_CSI0:
            return aw_ccu_reg->AhbGate1.Csi0? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_CSI1:
            return aw_ccu_reg->AhbGate1.Csi1? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_HDMI:
            return aw_ccu_reg->AhbGate1.Hdmi? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_DEBE0:
            return aw_ccu_reg->AhbGate1.Be0? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_DEBE1:
            return aw_ccu_reg->AhbGate1.Be1? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_DEFE0:
            return aw_ccu_reg->AhbGate1.Fe0? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_DEFE1:
            return aw_ccu_reg->AhbGate1.Fe1? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_MP:
            return aw_ccu_reg->AhbGate1.Mp? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_GPU:
            return aw_ccu_reg->AhbGate1.Gpu? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_MSGBOX:
            return aw_ccu_reg->AhbGate1.MsgBox? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_SPINLOCK:
            return aw_ccu_reg->AhbGate1.SpinLock? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_DEU0:
            return aw_ccu_reg->AhbGate1.Deu0? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_DEU1:
            return aw_ccu_reg->AhbGate1.Deu1? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_DRC0:
            return aw_ccu_reg->AhbGate1.Drc0? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_DRC1:
            return aw_ccu_reg->AhbGate1.Drc1? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_AHB_CLK_MTCACC:
            return aw_ccu_reg->AhbGate1.MtcAcc? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_APB_CLK_ADDA:
            return aw_ccu_reg->Apb1Gate.Adda? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_APB_CLK_SPDIF:
            return aw_ccu_reg->Apb1Gate.Spdif? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_APB_CLK_PIO:
            return aw_ccu_reg->Apb1Gate.Pio? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_APB_CLK_I2S0:
            return aw_ccu_reg->Apb1Gate.I2s0? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_APB_CLK_I2S1:
            return aw_ccu_reg->Apb1Gate.I2s1? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_APB_CLK_TWI0:
            return aw_ccu_reg->Apb2Gate.Twi0? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_APB_CLK_TWI1:
            return aw_ccu_reg->Apb2Gate.Twi1? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_APB_CLK_TWI2:
            return aw_ccu_reg->Apb2Gate.Twi2? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_APB_CLK_TWI3:
            return aw_ccu_reg->Apb2Gate.Twi3? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_APB_CLK_UART0:
            return aw_ccu_reg->Apb2Gate.Uart0? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_APB_CLK_UART1:
            return aw_ccu_reg->Apb2Gate.Uart1? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_APB_CLK_UART2:
            return aw_ccu_reg->Apb2Gate.Uart2? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_APB_CLK_UART3:
            return aw_ccu_reg->Apb2Gate.Uart3? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_APB_CLK_UART4:
            return aw_ccu_reg->Apb2Gate.Uart4? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_APB_CLK_UART5:
            return aw_ccu_reg->Apb2Gate.Uart5? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_DRAM_CLK_VE:
            return aw_ccu_reg->DramGate.Ve? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_DRAM_CLK_CSI_ISP:
            return aw_ccu_reg->DramGate.CsiIsp? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_DRAM_CLK_TS:
            return aw_ccu_reg->DramGate.Ts? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_DRAM_CLK_DRC0:
            return aw_ccu_reg->DramGate.Drc0? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_DRAM_CLK_DRC1:
            return aw_ccu_reg->DramGate.Drc1? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_DRAM_CLK_DEU0:
            return aw_ccu_reg->DramGate.Deu0? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_DRAM_CLK_DEU1:
            return aw_ccu_reg->DramGate.Deu1? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_DRAM_CLK_DEFE0:
            return aw_ccu_reg->DramGate.Fe0? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_DRAM_CLK_DEFE1:
            return aw_ccu_reg->DramGate.Fe1? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_DRAM_CLK_DEBE0:
            return aw_ccu_reg->DramGate.Be0? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_DRAM_CLK_DEBE1:
            return aw_ccu_reg->DramGate.Be1? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_DRAM_CLK_MP:
            return aw_ccu_reg->DramGate.Mp? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;

        case AW_MOD_CLK_R_1WIRE:
            return (aw_cpus_reg->Apb0Gate.OneWire && aw_cpus_reg->OneWire.ClkGate)? \
                        AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_MOD_CLK_R_CIR:
            return (aw_cpus_reg->Apb0Gate.Cir && aw_cpus_reg->Cir.ClkGate)?     \
                        AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_MOD_CLK_R_TWI:
            return aw_cpus_reg->Apb0Gate.Twi? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_MOD_CLK_R_UART:
            return aw_cpus_reg->Apb0Gate.Uart? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_MOD_CLK_R_P2WI:
            return aw_cpus_reg->Apb0Gate.P2wi? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_MOD_CLK_R_TMR:
            return aw_cpus_reg->Apb0Gate.Tmr? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_MOD_CLK_R_PIO:
            return aw_cpus_reg->Apb0Gate.Pio? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;

        default:
            return AW_CCU_CLK_ON;
    }
}


/*
*********************************************************************************************************
*                           mod_clk_get_rate
*
*Description: get module clock rate;
*
*Arguments  : id    module clock id;
*
*Return     : module clock division;
*
*Notes      :
*
*********************************************************************************************************
*/
static __s64 mod_clk_get_rate(__aw_ccu_clk_id_e id)
{
    switch(id)
    {
        case AW_MOD_CLK_NAND0:
            return _get_module0_clk_rate(&aw_ccu_reg->Nand0);
        case AW_MOD_CLK_NAND1:
            return _get_module0_clk_rate(&aw_ccu_reg->Nand1);
        case AW_MOD_CLK_SDC0:
            return _get_module0_clk_rate(&aw_ccu_reg->Sd0);
        case AW_MOD_CLK_SDC1:
            return _get_module0_clk_rate(&aw_ccu_reg->Sd1);
        case AW_MOD_CLK_SDC2:
            return _get_module0_clk_rate(&aw_ccu_reg->Sd2);
        case AW_MOD_CLK_SDC3:
            return _get_module0_clk_rate(&aw_ccu_reg->Sd3);
        case AW_MOD_CLK_TS:
            return _get_module0_clk_rate(&aw_ccu_reg->Ts);
        case AW_MOD_CLK_SS:
            return _get_module0_clk_rate(&aw_ccu_reg->Ss);
        case AW_MOD_CLK_SPI0:
            return _get_module0_clk_rate(&aw_ccu_reg->Spi0);
        case AW_MOD_CLK_SPI1:
            return _get_module0_clk_rate(&aw_ccu_reg->Spi1);
        case AW_MOD_CLK_SPI2:
            return _get_module0_clk_rate(&aw_ccu_reg->Spi2);
        case AW_MOD_CLK_SPI3:
            return _get_module0_clk_rate(&aw_ccu_reg->Spi3);
        case AW_MOD_CLK_MDFS:
            return _get_module0_clk_rate(&aw_ccu_reg->Mdfs);
        case AW_MOD_CLK_DEBE0:
            return aw_ccu_reg->Be0.DivM+1;
        case AW_MOD_CLK_DEBE1:
            return aw_ccu_reg->Be1.DivM+1;
        case AW_MOD_CLK_DEFE0:
            return aw_ccu_reg->Fe0.DivM+1;
        case AW_MOD_CLK_DEFE1:
            return aw_ccu_reg->Fe1.DivM+1;
        case AW_MOD_CLK_DEMIX:
            return aw_ccu_reg->Mp.DivM+1;
        case AW_MOD_CLK_LCD0CH0:
            return aw_ccu_reg->Lcd0Ch0.DivM+1;
        case AW_MOD_CLK_LCD0CH1:
            return aw_ccu_reg->Lcd0Ch1.DivM+1;
        case AW_MOD_CLK_LCD1CH0:
            return aw_ccu_reg->Lcd1Ch0.DivM+1;
        case AW_MOD_CLK_LCD1CH1:
            return aw_ccu_reg->Lcd1Ch1.DivM+1;
        case AW_MOD_CLK_HDMI:
	    return aw_ccu_reg->Hdmi.ClkDiv+1;
        case AW_MOD_CLK_CSI0S:
            return aw_ccu_reg->Csi0.SClkDiv+1;
        case AW_MOD_CLK_CSI0M:
            return aw_ccu_reg->Csi0.MClkDiv+1;
        case AW_MOD_CLK_CSI1S:
            return aw_ccu_reg->Csi1.SClkDiv+1;
        case AW_MOD_CLK_CSI1M:
            return aw_ccu_reg->Csi1.MClkDiv+1;
        case AW_MOD_CLK_VE:
            return aw_ccu_reg->Ve.ClkDiv+1;
        case AW_MOD_CLK_MTCACC:
            return _get_module0_clk_rate(&aw_ccu_reg->MtcAcc);
        case AW_MOD_CLK_MBUS0:
            return _get_module0_clk_rate(&aw_ccu_reg->MBus0);
        case AW_MOD_CLK_MBUS1:
            return _get_module0_clk_rate(&aw_ccu_reg->MBus1);
        case AW_MOD_CLK_MIPIDSIS:
            return aw_ccu_reg->MipiDsi.SClkDiv+1;
        case AW_MOD_CLK_MIPIDSIP:
            return aw_ccu_reg->MipiDsi.PClkDiv+1;
        case AW_MOD_CLK_MIPICSIS:
            return aw_ccu_reg->MipiCsi.SClkDiv+1;
        case AW_MOD_CLK_MIPICSIP:
            return aw_ccu_reg->MipiCsi.PClkDiv+1;
        case AW_MOD_CLK_IEPDRC0:
            return _get_module0_clk_rate(&aw_ccu_reg->IepDrc0);
        case AW_MOD_CLK_IEPDRC1:
            return _get_module0_clk_rate(&aw_ccu_reg->IepDrc1);
        case AW_MOD_CLK_IEPDEU0:
            return _get_module0_clk_rate(&aw_ccu_reg->IepDeu0);
        case AW_MOD_CLK_IEPDEU1:
            return _get_module0_clk_rate(&aw_ccu_reg->IepDeu1);
        case AW_MOD_CLK_GPUCORE:
            return _get_module0_clk_rate(&aw_ccu_reg->GpuCore);
        case AW_MOD_CLK_GPUMEM:
            return _get_module0_clk_rate(&aw_ccu_reg->GpuMem);
        case AW_MOD_CLK_GPUHYD:
            return _get_module0_clk_rate(&aw_ccu_reg->GpuHyd);

        case AW_MOD_CLK_R_1WIRE:
	    return (1<<aw_cpus_reg->OneWire.DivN)*(aw_cpus_reg->OneWire.DivM+1);
        case AW_MOD_CLK_R_CIR:
            return _get_module0_clk_rate(&aw_cpus_reg->Cir);

        default:
            return 1;
    }
}


/*
*********************************************************************************************************
*                           mod_clk_set_parent
*
*Description: set clock parent id for module clock;
*
*Arguments  : id        module clock id;
*             parent    parent clock id;
*
*Return     : result;
*               0,  set parent successed;
*              !0,  set parent failed;
*Notes      :
*
*********************************************************************************************************
*/
static __s32 mod_clk_set_parent(__aw_ccu_clk_id_e id, __aw_ccu_clk_id_e parent)
{
    switch(id)
    {
        case AW_MOD_CLK_NAND0:
            return _set_module0_clk_src(&aw_ccu_reg->Nand0, parent);
        case AW_MOD_CLK_NAND1:
            return _set_module0_clk_src(&aw_ccu_reg->Nand1, parent);
        case AW_MOD_CLK_SDC0:
            return _set_module0_clk_src(&aw_ccu_reg->Sd0, parent);
        case AW_MOD_CLK_SDC1:
            return _set_module0_clk_src(&aw_ccu_reg->Sd1, parent);
        case AW_MOD_CLK_SDC2:
            return _set_module0_clk_src(&aw_ccu_reg->Sd2, parent);
        case AW_MOD_CLK_SDC3:
            return _set_module0_clk_src(&aw_ccu_reg->Sd3, parent);
        case AW_MOD_CLK_TS:
            return _set_module0_clk_src(&aw_ccu_reg->Ts, parent);
        case AW_MOD_CLK_SS:
            return _set_module0_clk_src(&aw_ccu_reg->Ss, parent);
        case AW_MOD_CLK_SPI0:
            return _set_module0_clk_src(&aw_ccu_reg->Spi0, parent);
        case AW_MOD_CLK_SPI1:
            return _set_module0_clk_src(&aw_ccu_reg->Spi1, parent);
        case AW_MOD_CLK_SPI2:
            return _set_module0_clk_src(&aw_ccu_reg->Spi2, parent);
        case AW_MOD_CLK_SPI3:
            return _set_module0_clk_src(&aw_ccu_reg->Spi3, parent);
        case AW_MOD_CLK_I2S0:
            return _set_module1_clk_src(&aw_ccu_reg->I2s0, parent);
        case AW_MOD_CLK_I2S1:
            return _set_module1_clk_src(&aw_ccu_reg->I2s1, parent);
        case AW_MOD_CLK_SPDIF:
            return _set_module1_clk_src(&aw_ccu_reg->Spdif, parent);
        case AW_MOD_CLK_MDFS:
        {
            if(parent == AW_SYS_CLK_PLL5)
                aw_ccu_reg->Mdfs.ClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL6)
                aw_ccu_reg->Mdfs.ClkSrc = 1;
            else
            {
                CCU_ERR("set clock source failed! set AW_MOD_CLK_MDFS to %d!\n", parent);
                return -1;
            }
            return 0;
        }
        case AW_MOD_CLK_DEBE0:
            return _set_disp_clk_src(&aw_ccu_reg->Be0, parent, DE_BE_INDX);
        case AW_MOD_CLK_DEBE1:
            return _set_disp_clk_src(&aw_ccu_reg->Be1, parent, DE_BE_INDX);
        case AW_MOD_CLK_DEFE0:
            return _set_disp_clk_src(&aw_ccu_reg->Fe0, parent, DE_FE_INDX);
        case AW_MOD_CLK_DEFE1:
            return _set_disp_clk_src(&aw_ccu_reg->Fe1, parent, DE_FE_INDX);
        case AW_MOD_CLK_DEMIX:
            return _set_disp_clk_src(&aw_ccu_reg->Mp, parent, DE_MP_INDX);
        case AW_MOD_CLK_LCD0CH0:
            return _set_disp_clk_src(&aw_ccu_reg->Lcd0Ch0, parent, LCD_CH0_INDX);
        case AW_MOD_CLK_LCD0CH1:
            return _set_disp_clk_src(&aw_ccu_reg->Lcd0Ch1, parent, LCD_CH1_INDX);
        case AW_MOD_CLK_LCD1CH0:
            return _set_disp_clk_src(&aw_ccu_reg->Lcd1Ch0, parent, LCD_CH0_INDX);
        case AW_MOD_CLK_LCD1CH1:
            return _set_disp_clk_src(&aw_ccu_reg->Lcd1Ch1, parent, LCD_CH1_INDX);
        case AW_MOD_CLK_ADDA:
            return _set_module1_clk_src(&aw_ccu_reg->Adda, parent);
        case AW_MOD_CLK_AVS:
            return parent == AW_SYS_CLK_HOSC? 0 : -1;
        case AW_MOD_CLK_PS:
            return _set_module1_clk_src(&aw_ccu_reg->Ps, parent);
        case AW_MOD_CLK_MTCACC:
            return _set_module0_clk_src(&aw_ccu_reg->MtcAcc, parent);
        case AW_MOD_CLK_MBUS0:
        {
            if(parent == AW_SYS_CLK_HOSC)
                aw_ccu_reg->MBus0.ClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL6)
                aw_ccu_reg->MBus0.ClkSrc = 1;
            else if(parent == AW_SYS_CLK_PLL5)
                aw_ccu_reg->MBus0.ClkSrc = 2;
            else
            {
                CCU_ERR("set clock source failed! set AW_MOD_CLK_MBUS0 to %d!\n", parent);
                return -1;
            }
            return 0;
        }
        case AW_MOD_CLK_MBUS1:
        {
            if(parent == AW_SYS_CLK_HOSC)
                aw_ccu_reg->MBus1.ClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL6)
                aw_ccu_reg->MBus1.ClkSrc = 1;
            else if(parent == AW_SYS_CLK_PLL5)
                aw_ccu_reg->MBus1.ClkSrc = 2;
            else
            {
                CCU_ERR("set clock source failed! set AW_MOD_CLK_MBUS1 to %d!\n", parent);
                return -1;
            }
            return 0;
        }
        case AW_MOD_CLK_IEPDRC0:
        {
            if(parent == AW_SYS_CLK_PLL3)
                aw_ccu_reg->IepDrc0.ClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL7)
                aw_ccu_reg->IepDrc0.ClkSrc = 1;
            else if(parent == AW_SYS_CLK_PLL8)
                aw_ccu_reg->IepDrc0.ClkSrc = 3;
            else if(parent == AW_SYS_CLK_PLL9)
                aw_ccu_reg->IepDrc0.ClkSrc = 4;
            else if(parent == AW_SYS_CLK_PLL10)
                aw_ccu_reg->IepDrc0.ClkSrc = 5;
            else
            {
                CCU_ERR("set clock source failed! set AW_MOD_CLK_IEPDRC0 to %d!\n", parent);
                return -1;
            }
            return 0;
        }
        case AW_MOD_CLK_IEPDRC1:
        {
            if(parent == AW_SYS_CLK_PLL3)
                aw_ccu_reg->IepDrc1.ClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL7)
                aw_ccu_reg->IepDrc1.ClkSrc = 1;
            else if(parent == AW_SYS_CLK_PLL8)
                aw_ccu_reg->IepDrc1.ClkSrc = 3;
            else if(parent == AW_SYS_CLK_PLL9)
                aw_ccu_reg->IepDrc1.ClkSrc = 4;
            else if(parent == AW_SYS_CLK_PLL10)
                aw_ccu_reg->IepDrc1.ClkSrc = 5;
            else
            {
                CCU_ERR("set clock source failed! set AW_MOD_CLK_IEPDRC1 to %d!\n", parent);
                return -1;
            }
            return 0;
        }
        case AW_MOD_CLK_IEPDEU0:
        {
            if(parent == AW_SYS_CLK_PLL3)
                aw_ccu_reg->IepDeu0.ClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL7)
                aw_ccu_reg->IepDeu0.ClkSrc = 1;
            else if(parent == AW_SYS_CLK_PLL8)
                aw_ccu_reg->IepDeu0.ClkSrc = 3;
            else if(parent == AW_SYS_CLK_PLL9)
                aw_ccu_reg->IepDeu0.ClkSrc = 4;
            else if(parent == AW_SYS_CLK_PLL10)
                aw_ccu_reg->IepDeu0.ClkSrc = 5;
            else
            {
                CCU_ERR("set clock source failed! set AW_MOD_CLK_IEPDEU0 to %d!\n", parent);
                return -1;
            }
            return 0;
        }
        case AW_MOD_CLK_IEPDEU1:
        {
            if(parent == AW_SYS_CLK_PLL3)
                aw_ccu_reg->IepDeu1.ClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL7)
                aw_ccu_reg->IepDeu1.ClkSrc = 1;
            else if(parent == AW_SYS_CLK_PLL8)
                aw_ccu_reg->IepDeu1.ClkSrc = 3;
            else if(parent == AW_SYS_CLK_PLL9)
                aw_ccu_reg->IepDeu1.ClkSrc = 4;
            else if(parent == AW_SYS_CLK_PLL10)
                aw_ccu_reg->IepDeu1.ClkSrc = 5;
            else
            {
                CCU_ERR("set clock source failed! set AW_MOD_CLK_IEPDEU1 to %d!\n", parent);
                return -1;
            }
            return 0;
        }
        case AW_MOD_CLK_GPUCORE:
        {
            if(parent == AW_SYS_CLK_PLL8)
                aw_ccu_reg->GpuCore.ClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL3)
                aw_ccu_reg->GpuCore.ClkSrc = 2;
            else if(parent == AW_SYS_CLK_PLL7)
                aw_ccu_reg->GpuCore.ClkSrc = 3;
            else if(parent == AW_SYS_CLK_PLL9)
                aw_ccu_reg->GpuCore.ClkSrc = 4;
            else if(parent == AW_SYS_CLK_PLL10)
                aw_ccu_reg->GpuCore.ClkSrc = 5;
            else
            {
                CCU_ERR("set clock source failed! set AW_MOD_CLK_GPUCORE to %d!\n", parent);
                return -1;
            }
            return 0;
        }
        case AW_MOD_CLK_GPUMEM:
        {
            if(parent == AW_SYS_CLK_PLL8)
                aw_ccu_reg->GpuMem.ClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL3)
                aw_ccu_reg->GpuMem.ClkSrc = 2;
            else if(parent == AW_SYS_CLK_PLL7)
                aw_ccu_reg->GpuMem.ClkSrc = 3;
            else if(parent == AW_SYS_CLK_PLL9)
                aw_ccu_reg->GpuMem.ClkSrc = 4;
            else if(parent == AW_SYS_CLK_PLL10)
                aw_ccu_reg->GpuMem.ClkSrc = 5;
            else
            {
                CCU_ERR("set clock source failed! set AW_MOD_CLK_GPUMEM to %d!\n", parent);
                return -1;
            }
            return 0;
        }
        case AW_MOD_CLK_GPUHYD:
        {
            if(parent == AW_SYS_CLK_PLL8)
                aw_ccu_reg->GpuHyd.ClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL3)
                aw_ccu_reg->GpuHyd.ClkSrc = 2;
            else if(parent == AW_SYS_CLK_PLL7)
                aw_ccu_reg->GpuHyd.ClkSrc = 3;
            else if(parent == AW_SYS_CLK_PLL9)
                aw_ccu_reg->GpuHyd.ClkSrc = 4;
            else if(parent == AW_SYS_CLK_PLL10)
                aw_ccu_reg->GpuHyd.ClkSrc = 5;
            else
            {
                CCU_ERR("set clock source failed! set AW_MOD_CLK_GPUHYD to %d!\n", parent);
                return -1;
            }
            return 0;
        }
        case AW_MOD_CLK_HDMI:
            if(parent == AW_SYS_CLK_PLL3)
                aw_ccu_reg->Hdmi.ClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL7)
                aw_ccu_reg->Hdmi.ClkSrc = 1;
            else if(parent == AW_SYS_CLK_PLL3X2)
                aw_ccu_reg->Hdmi.ClkSrc = 2;
            else if(parent == AW_SYS_CLK_PLL7X2)
                aw_ccu_reg->Hdmi.ClkSrc = 3;
            else
                return -1;
            return 0;
        case AW_MOD_CLK_CSI0S:
            if(parent == AW_SYS_CLK_PLL3)
                aw_ccu_reg->Csi0.SClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL7)
                aw_ccu_reg->Csi0.SClkSrc = 1;
            else if(parent == AW_SYS_CLK_PLL3X2)
                aw_ccu_reg->Csi0.SClkSrc = 2;
            else if(parent == AW_SYS_CLK_PLL7X2)
                aw_ccu_reg->Csi0.SClkSrc = 3;
            else if(parent == AW_SYS_CLK_MIPIPLL)
                aw_ccu_reg->Csi0.SClkSrc = 4;
            else if(parent == AW_SYS_CLK_PLL4)
                aw_ccu_reg->Csi0.SClkSrc = 5;
            else
                return -1;
            return 0;
        case AW_MOD_CLK_CSI0M:
            if(parent == AW_SYS_CLK_PLL3)
                aw_ccu_reg->Csi0.MClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL7)
                aw_ccu_reg->Csi0.MClkSrc = 1;
            else if(parent == AW_SYS_CLK_HOSC)
                aw_ccu_reg->Csi0.MClkSrc = 5;
            else
                return -1;
            return 0;
        case AW_MOD_CLK_CSI1S:
            if(parent == AW_SYS_CLK_PLL3)
                aw_ccu_reg->Csi1.SClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL7)
                aw_ccu_reg->Csi1.SClkSrc = 1;
            else if(parent == AW_SYS_CLK_PLL3X2)
                aw_ccu_reg->Csi1.SClkSrc = 2;
            else if(parent == AW_SYS_CLK_PLL7X2)
                aw_ccu_reg->Csi1.SClkSrc = 3;
            else if(parent == AW_SYS_CLK_MIPIPLL)
                aw_ccu_reg->Csi1.SClkSrc = 4;
            else if(parent == AW_SYS_CLK_PLL4)
                aw_ccu_reg->Csi1.SClkSrc = 5;
            else
                return -1;
            return 0;
        case AW_MOD_CLK_CSI1M:
            if(parent == AW_SYS_CLK_PLL3)
                aw_ccu_reg->Csi1.MClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL7)
                aw_ccu_reg->Csi1.MClkSrc = 1;
            else if(parent == AW_SYS_CLK_HOSC)
                aw_ccu_reg->Csi1.MClkSrc = 5;
            else
                return -1;
            return 0;
        case AW_MOD_CLK_MIPIDSIS:
            if(parent == AW_SYS_CLK_PLL3)
                aw_ccu_reg->MipiDsi.SClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL7)
                aw_ccu_reg->MipiDsi.SClkSrc = 1;
            else if(parent == AW_SYS_CLK_PLL3X2)
                aw_ccu_reg->MipiDsi.SClkSrc = 2;
            else if(parent == AW_SYS_CLK_PLL7X2)
                aw_ccu_reg->MipiDsi.SClkSrc = 3;
            else
                return -1;
            return 0;
        case AW_MOD_CLK_MIPIDSIP:
            if(parent == AW_SYS_CLK_PLL3)
                aw_ccu_reg->MipiDsi.PClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL7)
                aw_ccu_reg->MipiDsi.PClkSrc = 1;
            else if(parent == AW_SYS_CLK_PLL3X2)
                aw_ccu_reg->MipiDsi.PClkSrc = 2;
            else if(parent == AW_SYS_CLK_PLL7X2)
                aw_ccu_reg->MipiDsi.PClkSrc = 3;
            else
                return -1;
            return 0;
        case AW_MOD_CLK_MIPICSIS:
            if(parent == AW_SYS_CLK_PLL3)
                aw_ccu_reg->MipiCsi.SClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL7)
                aw_ccu_reg->MipiCsi.SClkSrc = 1;
            else if(parent == AW_SYS_CLK_PLL3X2)
                aw_ccu_reg->MipiCsi.SClkSrc = 2;
            else if(parent == AW_SYS_CLK_PLL7X2)
                aw_ccu_reg->MipiCsi.SClkSrc = 3;
            else
                return -1;
            return 0;
        case AW_MOD_CLK_MIPICSIP:
            if(parent == AW_SYS_CLK_PLL3)
                aw_ccu_reg->MipiCsi.PClkSrc = 0;
            else if(parent == AW_SYS_CLK_PLL7)
                aw_ccu_reg->MipiCsi.PClkSrc = 1;
            else if(parent == AW_SYS_CLK_PLL3X2)
                aw_ccu_reg->MipiCsi.PClkSrc = 2;
            else if(parent == AW_SYS_CLK_PLL7X2)
                aw_ccu_reg->MipiCsi.PClkSrc = 3;
            else
                return -1;
            return 0;

        case AW_MOD_CLK_R_1WIRE:
        {
            if(parent == AW_SYS_CLK_LOSC) {
                aw_cpus_reg->OneWire.ClkSrc = 0;
            } else if(parent == AW_SYS_CLK_HOSC) {
                aw_cpus_reg->OneWire.ClkSrc = 1;
            } else {
                return -1;
            }
            return 0;
        }
        case AW_MOD_CLK_R_CIR:
        {
            if(parent == AW_SYS_CLK_LOSC) {
                aw_cpus_reg->Cir.ClkSrc = 0;
            } else if(parent == AW_SYS_CLK_HOSC) {
                aw_cpus_reg->Cir.ClkSrc = 1;
            } else {
                return -1;
            }
            return 0;
        }

        case AW_MOD_CLK_R_TWI:
        case AW_MOD_CLK_R_UART:
        case AW_MOD_CLK_R_P2WI:
        case AW_MOD_CLK_R_TMR:
        case AW_MOD_CLK_R_PIO:
        {
            if(parent == AW_SYS_CLK_APB0){
                return 0;
            }

            return -1;
        }

        default:
            return 0;
    }
}


/*
*********************************************************************************************************
*                           mod_clk_set_status
*
*Description: set module clock on/off status;
*
*Arguments  : id        module clock id;
*             status    module clock on/off status;
*
*Return     : result
*               0,  set module clock on/off status successed;
*              !0,  set module clock on/off status failed;
*
*Notes      :
*
*********************************************************************************************************
*/
#define STATUS_BIT(status)  ((status == AW_CCU_CLK_ON)? 1:0)
static __s32 mod_clk_set_status(__aw_ccu_clk_id_e id, __aw_ccu_clk_onff_e status)
{
    switch(id)
    {
        case AW_MOD_CLK_NAND0:
            aw_ccu_reg->Nand0.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_NAND1:
            aw_ccu_reg->Nand1.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_SDC0:
            aw_ccu_reg->Sd0.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_SDC1:
            aw_ccu_reg->Sd1.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_SDC2:
            aw_ccu_reg->Sd2.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_SDC3:
            aw_ccu_reg->Sd3.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_TS:
            aw_ccu_reg->Ts.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_SS:
            aw_ccu_reg->Ss.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_SPI0:
            aw_ccu_reg->Spi0.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_SPI1:
            aw_ccu_reg->Spi1.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_SPI2:
            aw_ccu_reg->Spi2.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_SPI3:
            aw_ccu_reg->Spi3.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_I2S0:
            aw_ccu_reg->I2s0.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_I2S1:
            aw_ccu_reg->I2s1.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_SPDIF:
            aw_ccu_reg->Spdif.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_USBPHY0:
            aw_ccu_reg->Usb.Phy0Gate = STATUS_BIT(status); break;
        case AW_MOD_CLK_USBPHY1:
            aw_ccu_reg->Usb.Phy1Gate = STATUS_BIT(status); break;
        case AW_MOD_CLK_USBPHY2:
            aw_ccu_reg->Usb.Phy2Gate = STATUS_BIT(status); break;
        case AW_MOD_CLK_USBOHCI0:
            aw_ccu_reg->Usb.Ohci0Gate = STATUS_BIT(status); break;
        case AW_MOD_CLK_USBOHCI1:
            aw_ccu_reg->Usb.Ohci1Gate = STATUS_BIT(status); break;
        case AW_MOD_CLK_USBOHCI2:
            aw_ccu_reg->Usb.Ohci2Gate = STATUS_BIT(status); break;
        case AW_MOD_CLK_MDFS:
            aw_ccu_reg->Mdfs.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_DEBE0:
            aw_ccu_reg->Be0.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_DEBE1:
            aw_ccu_reg->Be1.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_DEFE0:
            aw_ccu_reg->Fe0.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_DEFE1:
            aw_ccu_reg->Fe1.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_DEMIX:
            aw_ccu_reg->Mp.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_LCD0CH0:
            aw_ccu_reg->Lcd0Ch0.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_LCD0CH1:
            aw_ccu_reg->Lcd0Ch1.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_LCD1CH0:
            aw_ccu_reg->Lcd1Ch0.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_LCD1CH1:
            aw_ccu_reg->Lcd1Ch1.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_CSI0S:
            aw_ccu_reg->Csi0.SClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_CSI0M:
            aw_ccu_reg->Csi0.MClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_CSI1S:
            aw_ccu_reg->Csi1.SClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_CSI1M:
            aw_ccu_reg->Csi1.MClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_VE:
            aw_ccu_reg->Ve.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_ADDA:
            aw_ccu_reg->Adda.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_AVS:
            aw_ccu_reg->Avs.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_DMIC:
            aw_ccu_reg->Dmic.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_HDMI:
            aw_ccu_reg->Hdmi.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_HDMI_DDC:
            return aw_ccu_reg->Hdmi.DDCGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_PS:
            aw_ccu_reg->Ps.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_MTCACC:
            aw_ccu_reg->MtcAcc.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_MBUS0:
            aw_ccu_reg->MBus0.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_MBUS1:
            aw_ccu_reg->MBus1.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_MIPIDSIS:
            aw_ccu_reg->MipiDsi.SClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_MIPIDSIP:
            aw_ccu_reg->MipiDsi.PClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_MIPICSIS:
            aw_ccu_reg->MipiCsi.SClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_MIPICSIP:
            aw_ccu_reg->MipiCsi.PClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_IEPDRC0:
            aw_ccu_reg->IepDrc0.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_IEPDRC1:
            aw_ccu_reg->IepDrc1.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_IEPDEU0:
            aw_ccu_reg->IepDeu0.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_IEPDEU1:
            aw_ccu_reg->IepDeu1.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_GPUCORE:
            aw_ccu_reg->GpuCore.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_GPUMEM:
            aw_ccu_reg->GpuMem.ClkGate = STATUS_BIT(status); break;
        case AW_MOD_CLK_GPUHYD:
            aw_ccu_reg->GpuHyd.ClkGate = STATUS_BIT(status); break;
        case AW_AXI_CLK_DRAM:
            aw_ccu_reg->AxiGate.Sdram = STATUS_BIT(status); break;
        case AW_AHB_CLK_MIPICSI:
            aw_ccu_reg->AhbGate0.MipiCsi = STATUS_BIT(status); break;
        case AW_AHB_CLK_MIPIDSI:
            aw_ccu_reg->AhbGate0.MipiDsi = STATUS_BIT(status); break;
        case AW_AHB_CLK_SS:
            aw_ccu_reg->AhbGate0.Ss = STATUS_BIT(status); break;
        case AW_AHB_CLK_DMA:
            aw_ccu_reg->AhbGate0.Dma = STATUS_BIT(status); break;
        case AW_AHB_CLK_SDMMC0:
            aw_ccu_reg->AhbGate0.Sd0 = STATUS_BIT(status); break;
        case AW_AHB_CLK_SDMMC1:
            aw_ccu_reg->AhbGate0.Sd1 = STATUS_BIT(status); break;
        case AW_AHB_CLK_SDMMC2:
            aw_ccu_reg->AhbGate0.Sd2 = STATUS_BIT(status); break;
        case AW_AHB_CLK_SDMMC3:
            aw_ccu_reg->AhbGate0.Sd3 = STATUS_BIT(status); break;
        case AW_AHB_CLK_NAND1:
            aw_ccu_reg->AhbGate0.Nand1 = STATUS_BIT(status); break;
        case AW_AHB_CLK_NAND0:
            aw_ccu_reg->AhbGate0.Nand0 = STATUS_BIT(status); break;
        case AW_AHB_CLK_SDRAM:
            aw_ccu_reg->AhbGate0.Dram = STATUS_BIT(status); break;
        case AW_AHB_CLK_GMAC:
            aw_ccu_reg->AhbGate0.Gmac = STATUS_BIT(status); break;
        case AW_AHB_CLK_TS:
            aw_ccu_reg->AhbGate0.Ts = STATUS_BIT(status); break;
        case AW_AHB_CLK_HSTMR:
            aw_ccu_reg->AhbGate0.HsTmr = STATUS_BIT(status); break;
        case AW_AHB_CLK_SPI0:
            aw_ccu_reg->AhbGate0.Spi0 = STATUS_BIT(status); break;
        case AW_AHB_CLK_SPI1:
            aw_ccu_reg->AhbGate0.Spi1 = STATUS_BIT(status); break;
        case AW_AHB_CLK_SPI2:
            aw_ccu_reg->AhbGate0.Spi2 = STATUS_BIT(status); break;
        case AW_AHB_CLK_SPI3:
            aw_ccu_reg->AhbGate0.Spi3 = STATUS_BIT(status); break;
        case AW_AHB_CLK_OTG:
            aw_ccu_reg->AhbGate0.Otg = STATUS_BIT(status); break;
        case AW_AHB_CLK_EHCI0:
            aw_ccu_reg->AhbGate0.Ehci0 = STATUS_BIT(status); break;
        case AW_AHB_CLK_EHCI1:
            aw_ccu_reg->AhbGate0.Ehci1 = STATUS_BIT(status); break;
        case AW_AHB_CLK_OHCI0:
            aw_ccu_reg->AhbGate0.Ohci0 = STATUS_BIT(status); break;
        case AW_AHB_CLK_OHCI1:
            aw_ccu_reg->AhbGate0.Ohci1 = STATUS_BIT(status); break;
        case AW_AHB_CLK_OHCI2:
            aw_ccu_reg->AhbGate0.Ohci2 = STATUS_BIT(status); break;
        case AW_AHB_CLK_VE:
            aw_ccu_reg->AhbGate1.Ve = STATUS_BIT(status); break;
        case AW_AHB_CLK_LCD0:
            aw_ccu_reg->AhbGate1.Lcd0 = STATUS_BIT(status); break;
        case AW_AHB_CLK_LCD1:
            aw_ccu_reg->AhbGate1.Lcd1 = STATUS_BIT(status); break;
        case AW_AHB_CLK_CSI0:
            aw_ccu_reg->AhbGate1.Csi0 = STATUS_BIT(status); break;
        case AW_AHB_CLK_CSI1:
            aw_ccu_reg->AhbGate1.Csi1 = STATUS_BIT(status); break;
        case AW_AHB_CLK_HDMI:
            aw_ccu_reg->AhbGate1.Hdmi = STATUS_BIT(status); break;
        case AW_AHB_CLK_DEBE0:
            aw_ccu_reg->AhbGate1.Be0 = STATUS_BIT(status); break;
        case AW_AHB_CLK_DEBE1:
            aw_ccu_reg->AhbGate1.Be1 = STATUS_BIT(status); break;
        case AW_AHB_CLK_DEFE0:
            aw_ccu_reg->AhbGate1.Fe0 = STATUS_BIT(status); break;
        case AW_AHB_CLK_DEFE1:
            aw_ccu_reg->AhbGate1.Fe1 = STATUS_BIT(status); break;
        case AW_AHB_CLK_MP:
            aw_ccu_reg->AhbGate1.Mp = STATUS_BIT(status); break;
        case AW_AHB_CLK_GPU:
            aw_ccu_reg->AhbGate1.Gpu = STATUS_BIT(status); break;
        case AW_AHB_CLK_MSGBOX:
            aw_ccu_reg->AhbGate1.MsgBox = STATUS_BIT(status); break;
        case AW_AHB_CLK_SPINLOCK:
            aw_ccu_reg->AhbGate1.SpinLock = STATUS_BIT(status); break;
        case AW_AHB_CLK_DEU0:
            aw_ccu_reg->AhbGate1.Deu0 = STATUS_BIT(status); break;
        case AW_AHB_CLK_DEU1:
            aw_ccu_reg->AhbGate1.Deu1 = STATUS_BIT(status); break;
        case AW_AHB_CLK_DRC0:
            aw_ccu_reg->AhbGate1.Drc0 = STATUS_BIT(status); break;
        case AW_AHB_CLK_DRC1:
            aw_ccu_reg->AhbGate1.Drc1 = STATUS_BIT(status); break;
        case AW_AHB_CLK_MTCACC:
            aw_ccu_reg->AhbGate1.MtcAcc = STATUS_BIT(status); break;
        case AW_APB_CLK_ADDA:
            aw_ccu_reg->Apb1Gate.Adda = STATUS_BIT(status); break;
        case AW_APB_CLK_SPDIF:
            aw_ccu_reg->Apb1Gate.Spdif = STATUS_BIT(status); break;
        case AW_APB_CLK_PIO:
            aw_ccu_reg->Apb1Gate.Pio = STATUS_BIT(status); break;
        case AW_APB_CLK_I2S0:
            aw_ccu_reg->Apb1Gate.I2s0 = STATUS_BIT(status); break;
        case AW_APB_CLK_I2S1:
            aw_ccu_reg->Apb1Gate.I2s1 = STATUS_BIT(status); break;
        case AW_APB_CLK_TWI0:
            aw_ccu_reg->Apb2Gate.Twi0 = STATUS_BIT(status); break;
        case AW_APB_CLK_TWI1:
            aw_ccu_reg->Apb2Gate.Twi1 = STATUS_BIT(status); break;
        case AW_APB_CLK_TWI2:
            aw_ccu_reg->Apb2Gate.Twi2 = STATUS_BIT(status); break;
        case AW_APB_CLK_TWI3:
            aw_ccu_reg->Apb2Gate.Twi3 = STATUS_BIT(status); break;
        case AW_APB_CLK_UART0:
            aw_ccu_reg->Apb2Gate.Uart0 = STATUS_BIT(status); break;
        case AW_APB_CLK_UART1:
            aw_ccu_reg->Apb2Gate.Uart1 = STATUS_BIT(status); break;
        case AW_APB_CLK_UART2:
            aw_ccu_reg->Apb2Gate.Uart2 = STATUS_BIT(status); break;
        case AW_APB_CLK_UART3:
            aw_ccu_reg->Apb2Gate.Uart3 = STATUS_BIT(status); break;
        case AW_APB_CLK_UART4:
            aw_ccu_reg->Apb2Gate.Uart4 = STATUS_BIT(status); break;
        case AW_APB_CLK_UART5:
            aw_ccu_reg->Apb2Gate.Uart5 = STATUS_BIT(status); break;
        case AW_DRAM_CLK_VE:
            aw_ccu_reg->DramGate.Ve = STATUS_BIT(status); break;
        case AW_DRAM_CLK_CSI_ISP:
            aw_ccu_reg->DramGate.CsiIsp = STATUS_BIT(status); break;
        case AW_DRAM_CLK_TS:
            aw_ccu_reg->DramGate.Ts = STATUS_BIT(status); break;
        case AW_DRAM_CLK_DRC0:
            aw_ccu_reg->DramGate.Drc0 = STATUS_BIT(status); break;
        case AW_DRAM_CLK_DRC1:
            aw_ccu_reg->DramGate.Drc1 = STATUS_BIT(status); break;
        case AW_DRAM_CLK_DEU0:
            aw_ccu_reg->DramGate.Deu0 = STATUS_BIT(status); break;
        case AW_DRAM_CLK_DEU1:
            aw_ccu_reg->DramGate.Deu1 = STATUS_BIT(status); break;
        case AW_DRAM_CLK_DEFE0:
            aw_ccu_reg->DramGate.Fe0 = STATUS_BIT(status); break;
        case AW_DRAM_CLK_DEFE1:
            aw_ccu_reg->DramGate.Fe1 = STATUS_BIT(status); break;
        case AW_DRAM_CLK_DEBE0:
            aw_ccu_reg->DramGate.Be0 = STATUS_BIT(status); break;
        case AW_DRAM_CLK_DEBE1:
            aw_ccu_reg->DramGate.Be1 = STATUS_BIT(status); break;
        case AW_DRAM_CLK_MP:
            aw_ccu_reg->DramGate.Mp = STATUS_BIT(status); break;

        case AW_MOD_CLK_R_1WIRE:
        {
            aw_cpus_reg->Apb0Gate.OneWire = STATUS_BIT(status);
            aw_cpus_reg->OneWire.ClkGate = STATUS_BIT(status);
            break;
        }
        case AW_MOD_CLK_R_CIR:
        {
            aw_cpus_reg->Apb0Gate.Cir = STATUS_BIT(status);
            aw_cpus_reg->Cir.ClkGate = STATUS_BIT(status);
            break;
        }
        case AW_MOD_CLK_R_TWI:
            aw_cpus_reg->Apb0Gate.Twi = STATUS_BIT(status); break;
        case AW_MOD_CLK_R_UART:
            aw_cpus_reg->Apb0Gate.Uart = STATUS_BIT(status); break;
        case AW_MOD_CLK_R_P2WI:
            aw_cpus_reg->Apb0Gate.P2wi = STATUS_BIT(status); break;
        case AW_MOD_CLK_R_TMR:
            aw_cpus_reg->Apb0Gate.Tmr = STATUS_BIT(status); break;
        case AW_MOD_CLK_R_PIO:
            aw_cpus_reg->Apb0Gate.Pio = STATUS_BIT(status); break;

        default:
            break;
    }

    return 0;
}


/*
*********************************************************************************************************
*                           mod_clk_set_rate
*
*Description: set module clock division;
*
*Arguments  : id    module clock id;
*             rate  module clock division;
*
*Return     : result
*               0,  set module clock rate successed;
*              !0,  set module clock rate failed;
*
*Notes      :
*
*********************************************************************************************************
*/
static __s32 mod_clk_set_rate(__aw_ccu_clk_id_e id, __s64 rate)
{
    switch(id)
    {
        case AW_MOD_CLK_NAND0:
            return _set_module0_clk_rate(&aw_ccu_reg->Nand0, rate);
        case AW_MOD_CLK_NAND1:
            return _set_module0_clk_rate(&aw_ccu_reg->Nand1, rate);
        case AW_MOD_CLK_SDC0:
            return _set_mmc_clk_rate(&aw_ccu_reg->Sd0, rate);
        case AW_MOD_CLK_SDC1:
            return _set_mmc_clk_rate(&aw_ccu_reg->Sd1, rate);
        case AW_MOD_CLK_SDC2:
            return _set_mmc_clk_rate(&aw_ccu_reg->Sd2, rate);
        case AW_MOD_CLK_SDC3:
            return _set_mmc_clk_rate(&aw_ccu_reg->Sd3, rate);
        case AW_MOD_CLK_TS:
            return _set_module0_clk_rate(&aw_ccu_reg->Ts, rate);
        case AW_MOD_CLK_SS:
            return _set_module0_clk_rate(&aw_ccu_reg->Ss, rate);
        case AW_MOD_CLK_SPI0:
            return _set_module0_clk_rate(&aw_ccu_reg->Spi0, rate);
        case AW_MOD_CLK_SPI1:
            return _set_module0_clk_rate(&aw_ccu_reg->Spi1, rate);
        case AW_MOD_CLK_SPI2:
            return _set_module0_clk_rate(&aw_ccu_reg->Spi2, rate);
        case AW_MOD_CLK_SPI3:
            return _set_module0_clk_rate(&aw_ccu_reg->Spi3, rate);
        case AW_MOD_CLK_MDFS:
            return _set_module0_clk_rate(&aw_ccu_reg->Mdfs, rate);
        case AW_MOD_CLK_DEBE0:
            aw_ccu_reg->Be0.DivM = rate-1;  break;
        case AW_MOD_CLK_DEBE1:
            aw_ccu_reg->Be1.DivM = rate-1;  break;
        case AW_MOD_CLK_DEFE0:
            aw_ccu_reg->Fe0.DivM = rate-1;  break;
        case AW_MOD_CLK_DEFE1:
            aw_ccu_reg->Fe1.DivM = rate-1;  break;
        case AW_MOD_CLK_DEMIX:
            aw_ccu_reg->Mp.DivM = rate-1;   break;
        case AW_MOD_CLK_LCD0CH0:
            aw_ccu_reg->Lcd0Ch0.DivM = rate-1;  break;
        case AW_MOD_CLK_LCD0CH1:
            aw_ccu_reg->Lcd0Ch1.DivM = rate-1;  break;
        case AW_MOD_CLK_HDMI:
            aw_ccu_reg->Hdmi.ClkDiv  = rate-1;  break;
        case AW_MOD_CLK_LCD1CH0:
            aw_ccu_reg->Lcd1Ch0.DivM = rate-1;  break;
        case AW_MOD_CLK_LCD1CH1:
            aw_ccu_reg->Lcd1Ch1.DivM = rate-1;  break;
        case AW_MOD_CLK_CSI0S:
            aw_ccu_reg->Csi0.SClkDiv = rate-1;  break;
        case AW_MOD_CLK_CSI0M:
            aw_ccu_reg->Csi0.MClkDiv = rate-1;  break;
        case AW_MOD_CLK_CSI1S:
            aw_ccu_reg->Csi1.SClkDiv = rate-1;  break;
        case AW_MOD_CLK_CSI1M:
            aw_ccu_reg->Csi1.MClkDiv = rate-1;  break;
        case AW_MOD_CLK_VE:
            aw_ccu_reg->Ve.ClkDiv = rate-1;  break;
        case AW_MOD_CLK_MTCACC:
            return _set_module0_clk_rate(&aw_ccu_reg->MtcAcc, rate);
        case AW_MOD_CLK_MBUS0:
            return _set_module0_clk_rate(&aw_ccu_reg->MBus0, rate);
        case AW_MOD_CLK_MBUS1:
            return _set_module0_clk_rate(&aw_ccu_reg->MBus1, rate);
        case AW_MOD_CLK_MIPIDSIS:
            aw_ccu_reg->MipiDsi.SClkDiv = rate-1;  break;
        case AW_MOD_CLK_MIPIDSIP:
            aw_ccu_reg->MipiDsi.PClkDiv = rate-1;  break;
        case AW_MOD_CLK_MIPICSIS:
            aw_ccu_reg->MipiCsi.SClkDiv = rate-1;  break;
        case AW_MOD_CLK_MIPICSIP:
            aw_ccu_reg->MipiCsi.PClkDiv = rate-1;  break;
        case AW_MOD_CLK_IEPDRC0:
            return _set_module0_clk_rate(&aw_ccu_reg->IepDrc0, rate);
        case AW_MOD_CLK_IEPDRC1:
            return _set_module0_clk_rate(&aw_ccu_reg->IepDrc1, rate);
        case AW_MOD_CLK_IEPDEU0:
            return _set_module0_clk_rate(&aw_ccu_reg->IepDeu0, rate);
        case AW_MOD_CLK_IEPDEU1:
            return _set_module0_clk_rate(&aw_ccu_reg->IepDeu1, rate);
        case AW_MOD_CLK_GPUCORE:
            return _set_module0_clk_rate(&aw_ccu_reg->GpuCore, rate);
        case AW_MOD_CLK_GPUMEM:
            return _set_module0_clk_rate(&aw_ccu_reg->GpuMem, rate);
        case AW_MOD_CLK_GPUHYD:
            return _set_module0_clk_rate(&aw_ccu_reg->GpuHyd, rate);

        case AW_MOD_CLK_R_1WIRE:
            return _set_onewire_clk_rate(&aw_cpus_reg->OneWire, rate);
        case AW_MOD_CLK_R_CIR:
            return _set_module0_clk_rate(&aw_cpus_reg->Cir, rate);

        default:
            return 0;
    }
    return 0;
}




/*
*********************************************************************************************************
*                           mod_clk_get_rate_hz
*
*Description: get module clock rate based on hz;
*
*Arguments  : id    module clock id;
*
*Return     : module clock division;
*
*Notes      :
*
*********************************************************************************************************
*/
static __u64 mod_clk_get_rate_hz(__aw_ccu_clk_id_e id)
{
    __u64   parent_rate = sys_clk_ops.get_rate(mod_clk_get_parent(id));
    __u64   div = mod_clk_get_rate(id);

    if(!div) {
        div = 1;
    }

    do_div(parent_rate, div);
    return parent_rate;
}


/*
*********************************************************************************************************
*                           mod_clk_set_rate_hz
*
*Description: set module clock rate based on hz;
*
*Arguments  : id    module clock id;
*             rate  module clock division;
*
*Return     : result
*               0,  set module clock rate successed;
*              !0,  set module clock rate failed;
*
*Notes      :
*
*********************************************************************************************************
*/
static int mod_clk_set_rate_hz(__aw_ccu_clk_id_e id, __u64 rate)
{
    __u64   parent_rate = sys_clk_ops.get_rate(mod_clk_get_parent(id));

    if(!rate) {
        return -1;
    }

    parent_rate += rate - 1;
    do_div(parent_rate, rate);
    return mod_clk_set_rate(id, parent_rate);
}


/*
*********************************************************************************************************
*                           mod_clk_get_reset
*
*Description: get module clock reset status;
*
*Arguments  : id    module clock id;
*
*Return     : result,
*               AW_CCU_CLK_RESET,   module clock reset valid;
*               AW_CCU_CLK_NRESET,  module clock reset invalid;
*
*Notes      :
*
*********************************************************************************************************
*/
#define GET_RESET(reg)  ((reg == 1)? AW_CCU_CLK_NRESET : AW_CCU_CLK_RESET)
static __aw_ccu_clk_reset_e mod_clk_get_reset(__aw_ccu_clk_id_e id)
{
    switch(id)
    {
        case AW_MOD_CLK_NAND0:
            return GET_RESET(aw_ccu_reg->AhbReset0.Nand0);
        case AW_MOD_CLK_NAND1:
            return GET_RESET(aw_ccu_reg->AhbReset0.Nand1);
        case AW_MOD_CLK_SDC0:
            return GET_RESET(aw_ccu_reg->AhbReset0.Sd0);
        case AW_MOD_CLK_SDC1:
            return GET_RESET(aw_ccu_reg->AhbReset0.Sd1);
        case AW_MOD_CLK_SDC2:
            return GET_RESET(aw_ccu_reg->AhbReset0.Sd2);
        case AW_MOD_CLK_SDC3:
            return GET_RESET(aw_ccu_reg->AhbReset0.Sd3);
        case AW_MOD_CLK_TS:
            return GET_RESET(aw_ccu_reg->AhbReset0.Ts);
        case AW_MOD_CLK_SS:
            return GET_RESET(aw_ccu_reg->AhbReset0.Ss);
        case AW_MOD_CLK_SPI0:
            return GET_RESET(aw_ccu_reg->AhbReset0.Spi0);
        case AW_MOD_CLK_SPI1:
            return GET_RESET(aw_ccu_reg->AhbReset0.Spi1);
        case AW_MOD_CLK_SPI2:
            return GET_RESET(aw_ccu_reg->AhbReset0.Spi2);
        case AW_MOD_CLK_SPI3:
            return GET_RESET(aw_ccu_reg->AhbReset0.Spi3);
        case AW_MOD_CLK_I2S0:
            return GET_RESET(aw_ccu_reg->Apb1Reset.I2s0);
        case AW_MOD_CLK_I2S1:
            return GET_RESET(aw_ccu_reg->Apb1Reset.I2s1);
        case AW_MOD_CLK_SPDIF:
            return GET_RESET(aw_ccu_reg->Apb1Reset.Spdif);
        case AW_MOD_CLK_USBPHY0:
            return GET_RESET(aw_ccu_reg->Usb.UsbPhy0Rst);
        case AW_MOD_CLK_USBPHY1:
            return GET_RESET(aw_ccu_reg->Usb.UsbPhy1Rst);
        case AW_MOD_CLK_USBPHY2:
            return GET_RESET(aw_ccu_reg->Usb.UsbPhy2Rst);
        case AW_MOD_CLK_USBEHCI0:
            return GET_RESET(aw_ccu_reg->AhbReset0.Ehci0);
        case AW_MOD_CLK_USBEHCI1:
            return GET_RESET(aw_ccu_reg->AhbReset0.Ehci1);
        case AW_MOD_CLK_USBOHCI0:
            return GET_RESET(aw_ccu_reg->AhbReset0.Ohci0);
        case AW_MOD_CLK_USBOHCI1:
            return GET_RESET(aw_ccu_reg->AhbReset0.Ohci1);
        case AW_MOD_CLK_USBOHCI2:
            return GET_RESET(aw_ccu_reg->AhbReset0.Ohci2);
        case AW_MOD_CLK_DEBE0:
            return GET_RESET(aw_ccu_reg->AhbReset1.Be0);
        case AW_MOD_CLK_DEBE1:
            return GET_RESET(aw_ccu_reg->AhbReset1.Be1);
        case AW_MOD_CLK_DEFE0:
            return GET_RESET(aw_ccu_reg->AhbReset1.Fe0);
        case AW_MOD_CLK_DEFE1:
            return GET_RESET(aw_ccu_reg->AhbReset1.Fe1);
        case AW_MOD_CLK_DEMIX:
            return GET_RESET(aw_ccu_reg->AhbReset1.Mp);
        case AW_MOD_CLK_LCD0CH0:
        case AW_MOD_CLK_LCD0CH1:
            return GET_RESET(aw_ccu_reg->AhbReset1.Lcd0);
        case AW_MOD_CLK_LCD1CH0:
        case AW_MOD_CLK_LCD1CH1:
            return GET_RESET(aw_ccu_reg->AhbReset1.Lcd1);
        case AW_MOD_CLK_CSI0S:
        case AW_MOD_CLK_CSI0M:
            return GET_RESET(aw_ccu_reg->AhbReset1.Csi0);
        case AW_MOD_CLK_CSI1S:
        case AW_MOD_CLK_CSI1M:
            return GET_RESET(aw_ccu_reg->AhbReset1.Csi1);
        case AW_MOD_CLK_VE:
            return GET_RESET(aw_ccu_reg->AhbReset1.Ve);
        case AW_MOD_CLK_ADDA:
            return GET_RESET(aw_ccu_reg->Apb1Reset.Adda);
        case AW_MOD_CLK_HDMI:
            return GET_RESET(aw_ccu_reg->AhbReset1.Hdmi);
        case AW_MOD_CLK_MTCACC:
            return GET_RESET(aw_ccu_reg->AhbReset1.MtcAcc);
        case AW_MOD_CLK_MIPIDSIS:
        case AW_MOD_CLK_MIPIDSIP:
            return GET_RESET(aw_ccu_reg->AhbReset0.MipiDsi);
        case AW_MOD_CLK_MIPICSIS:
        case AW_MOD_CLK_MIPICSIP:
            return AW_CCU_CLK_NRESET;
        case AW_MOD_CLK_IEPDRC0:
            return GET_RESET(aw_ccu_reg->AhbReset1.Drc0);
        case AW_MOD_CLK_IEPDRC1:
            return GET_RESET(aw_ccu_reg->AhbReset1.Drc1);
        case AW_MOD_CLK_IEPDEU0:
            return GET_RESET(aw_ccu_reg->AhbReset1.Deu0);
        case AW_MOD_CLK_IEPDEU1:
            return GET_RESET(aw_ccu_reg->AhbReset1.Deu1);
        case AW_MOD_CLK_GPUCORE:
        case AW_MOD_CLK_GPUMEM:
        case AW_MOD_CLK_GPUHYD:
            return GET_RESET(aw_ccu_reg->AhbReset1.Gpu);
        case AW_MOD_CLK_TWI0:
            return GET_RESET(aw_ccu_reg->Apb2Reset.Twi0);
        case AW_MOD_CLK_TWI1:
            return GET_RESET(aw_ccu_reg->Apb2Reset.Twi1);
        case AW_MOD_CLK_TWI2:
            return GET_RESET(aw_ccu_reg->Apb2Reset.Twi2);
        case AW_MOD_CLK_TWI3:
            return GET_RESET(aw_ccu_reg->Apb2Reset.Twi3);
        case AW_MOD_CLK_UART0:
            return GET_RESET(aw_ccu_reg->Apb2Reset.Uart0);
        case AW_MOD_CLK_UART1:
            return GET_RESET(aw_ccu_reg->Apb2Reset.Uart1);
        case AW_MOD_CLK_UART2:
            return GET_RESET(aw_ccu_reg->Apb2Reset.Uart2);
        case AW_MOD_CLK_UART3:
            return GET_RESET(aw_ccu_reg->Apb2Reset.Uart3);
        case AW_MOD_CLK_UART4:
            return GET_RESET(aw_ccu_reg->Apb2Reset.Uart4);
        case AW_MOD_CLK_UART5:
            return GET_RESET(aw_ccu_reg->Apb2Reset.Uart5);
        case AW_MOD_CLK_GMAC:
            return GET_RESET(aw_ccu_reg->AhbReset0.Gmac);
        case AW_MOD_CLK_DRAM:
            return GET_RESET(aw_ccu_reg->AhbReset0.Sdram);
        case AW_MOD_CLK_USBOTG:
            return GET_RESET(aw_ccu_reg->AhbReset0.Otg);
        case AW_MOD_CLK_DMA:
            return GET_RESET(aw_ccu_reg->AhbReset0.Dma);
        case AW_MOD_CLK_HSTMR:
            return GET_RESET(aw_ccu_reg->AhbReset0.HsTmr);
        case AW_MOD_CLK_MSGBOX:
            return GET_RESET(aw_ccu_reg->AhbReset1.MsgBox);
        case AW_MOD_CLK_SPINLOCK:
            return GET_RESET(aw_ccu_reg->AhbReset1.SpinLock);
        case AW_MOD_CLK_LVDS:
            return GET_RESET(aw_ccu_reg->AhbReset2.Lvds);

        case AW_MOD_CLK_R_1WIRE:
            return GET_RESET(aw_cpus_reg->ModReset.OneWire);
        case AW_MOD_CLK_R_CIR:
            return GET_RESET(aw_cpus_reg->ModReset.Cir);
        case AW_MOD_CLK_R_TWI:
            return GET_RESET(aw_cpus_reg->ModReset.Twi);
        case AW_MOD_CLK_R_UART:
            return GET_RESET(aw_cpus_reg->ModReset.Uart);
        case AW_MOD_CLK_R_P2WI:
            return GET_RESET(aw_cpus_reg->ModReset.P2wi);
        case AW_MOD_CLK_R_TMR:
            return GET_RESET(aw_cpus_reg->ModReset.Tmr);

        default:
            return AW_CCU_CLK_NRESET;
    }
}


/*
*********************************************************************************************************
*                           mod_clk_set_reset
*
*Description: set module clock reset status
*
*Arguments  : id    module clock id;
*             reset module clock reset status;
*
*Return     : result;
*               0,  set module clock reset status successed;
*              !0,  set module clock reset status failed;
*
*Notes      :
*
*********************************************************************************************************
*/
#define SET_RESET(reg, reset)  {reg = (reset==AW_CCU_CLK_NRESET)? 1:0;}
static __s32 mod_clk_set_reset(__aw_ccu_clk_id_e id, __aw_ccu_clk_reset_e reset)
{
    switch(id)
    {
        case AW_MOD_CLK_NAND0:
            SET_RESET(aw_ccu_reg->AhbReset0.Nand0, reset);  break;
        case AW_MOD_CLK_NAND1:
            SET_RESET(aw_ccu_reg->AhbReset0.Nand1, reset);  break;
        case AW_MOD_CLK_SDC0:
            SET_RESET(aw_ccu_reg->AhbReset0.Sd0, reset);    break;
        case AW_MOD_CLK_SDC1:
            SET_RESET(aw_ccu_reg->AhbReset0.Sd1, reset);    break;
        case AW_MOD_CLK_SDC2:
            SET_RESET(aw_ccu_reg->AhbReset0.Sd2, reset);    break;
        case AW_MOD_CLK_SDC3:
            SET_RESET(aw_ccu_reg->AhbReset0.Sd3, reset);    break;
        case AW_MOD_CLK_TS:
            SET_RESET(aw_ccu_reg->AhbReset0.Ts, reset);     break;
        case AW_MOD_CLK_SS:
            SET_RESET(aw_ccu_reg->AhbReset0.Ss, reset);     break;
        case AW_MOD_CLK_SPI0:
            SET_RESET(aw_ccu_reg->AhbReset0.Spi0, reset);   break;
        case AW_MOD_CLK_SPI1:
            SET_RESET(aw_ccu_reg->AhbReset0.Spi1, reset);   break;
        case AW_MOD_CLK_SPI2:
            SET_RESET(aw_ccu_reg->AhbReset0.Spi2, reset);   break;
        case AW_MOD_CLK_SPI3:
            SET_RESET(aw_ccu_reg->AhbReset0.Spi3, reset);   break;
        case AW_MOD_CLK_I2S0:
            SET_RESET(aw_ccu_reg->Apb1Reset.I2s0, reset);   break;
        case AW_MOD_CLK_I2S1:
            SET_RESET(aw_ccu_reg->Apb1Reset.I2s1, reset);   break;
        case AW_MOD_CLK_SPDIF:
            SET_RESET(aw_ccu_reg->Apb1Reset.Spdif, reset);  break;
        case AW_MOD_CLK_USBPHY0:
            SET_RESET(aw_ccu_reg->Usb.UsbPhy0Rst, reset);   break;
        case AW_MOD_CLK_USBPHY1:
            SET_RESET(aw_ccu_reg->Usb.UsbPhy1Rst, reset);   break;
        case AW_MOD_CLK_USBPHY2:
            SET_RESET(aw_ccu_reg->Usb.UsbPhy2Rst, reset);   break;
        case AW_MOD_CLK_USBEHCI0:
            SET_RESET(aw_ccu_reg->AhbReset0.Ehci0, reset);  break;
        case AW_MOD_CLK_USBEHCI1:
            SET_RESET(aw_ccu_reg->AhbReset0.Ehci1, reset);  break;
        case AW_MOD_CLK_USBOHCI0:
            SET_RESET(aw_ccu_reg->AhbReset0.Ohci0, reset);  break;
        case AW_MOD_CLK_USBOHCI1:
            SET_RESET(aw_ccu_reg->AhbReset0.Ohci1, reset);  break;
        case AW_MOD_CLK_USBOHCI2:
            SET_RESET(aw_ccu_reg->AhbReset0.Ohci2, reset);  break;
        case AW_MOD_CLK_DEBE0:
            SET_RESET(aw_ccu_reg->AhbReset1.Be0, reset);    break;
        case AW_MOD_CLK_DEBE1:
            SET_RESET(aw_ccu_reg->AhbReset1.Be1, reset);    break;
        case AW_MOD_CLK_DEFE0:
            SET_RESET(aw_ccu_reg->AhbReset1.Fe0, reset);    break;
        case AW_MOD_CLK_DEFE1:
            SET_RESET(aw_ccu_reg->AhbReset1.Fe1, reset);    break;
        case AW_MOD_CLK_DEMIX:
            SET_RESET(aw_ccu_reg->AhbReset1.Mp, reset);     break;
        case AW_MOD_CLK_LCD0CH0:
        case AW_MOD_CLK_LCD0CH1:
            SET_RESET(aw_ccu_reg->AhbReset1.Lcd0, reset);   break;
        case AW_MOD_CLK_LCD1CH0:
        case AW_MOD_CLK_LCD1CH1:
            SET_RESET(aw_ccu_reg->AhbReset1.Lcd1, reset);   break;
        case AW_MOD_CLK_CSI0S:
        case AW_MOD_CLK_CSI0M:
            SET_RESET(aw_ccu_reg->AhbReset1.Csi0, reset);   break;
        case AW_MOD_CLK_CSI1S:
        case AW_MOD_CLK_CSI1M:
            SET_RESET(aw_ccu_reg->AhbReset1.Csi1, reset);   break;
        case AW_MOD_CLK_VE:
            SET_RESET(aw_ccu_reg->AhbReset1.Ve, reset);     break;
        case AW_MOD_CLK_ADDA:
            SET_RESET(aw_ccu_reg->Apb1Reset.Adda, reset);   break;
        case AW_MOD_CLK_HDMI:
            SET_RESET(aw_ccu_reg->AhbReset1.Hdmi, reset);   break;
        case AW_MOD_CLK_MTCACC:
            SET_RESET(aw_ccu_reg->AhbReset1.MtcAcc, reset); break;
        case AW_MOD_CLK_MIPIDSIS:
        case AW_MOD_CLK_MIPIDSIP:
            SET_RESET(aw_ccu_reg->AhbReset0.MipiDsi, reset);break;
        case AW_MOD_CLK_MIPICSIS:
        case AW_MOD_CLK_MIPICSIP:
            break;
        case AW_MOD_CLK_IEPDRC0:
            SET_RESET(aw_ccu_reg->AhbReset1.Drc0, reset);   break;
        case AW_MOD_CLK_IEPDRC1:
            SET_RESET(aw_ccu_reg->AhbReset1.Drc1, reset);   break;
        case AW_MOD_CLK_IEPDEU0:
            SET_RESET(aw_ccu_reg->AhbReset1.Deu0, reset);   break;
        case AW_MOD_CLK_IEPDEU1:
            SET_RESET(aw_ccu_reg->AhbReset1.Deu1, reset);   break;
        case AW_MOD_CLK_GPUCORE:
        case AW_MOD_CLK_GPUMEM:
        case AW_MOD_CLK_GPUHYD:
            SET_RESET(aw_ccu_reg->AhbReset1.Gpu, reset);    break;
        case AW_MOD_CLK_TWI0:
            SET_RESET(aw_ccu_reg->Apb2Reset.Twi0, reset);   break;
        case AW_MOD_CLK_TWI1:
            SET_RESET(aw_ccu_reg->Apb2Reset.Twi1, reset);   break;
        case AW_MOD_CLK_TWI2:
            SET_RESET(aw_ccu_reg->Apb2Reset.Twi2, reset);   break;
        case AW_MOD_CLK_TWI3:
            SET_RESET(aw_ccu_reg->Apb2Reset.Twi3, reset);   break;
        case AW_MOD_CLK_UART0:
            SET_RESET(aw_ccu_reg->Apb2Reset.Uart0, reset);  break;
        case AW_MOD_CLK_UART1:
            SET_RESET(aw_ccu_reg->Apb2Reset.Uart1, reset);  break;
        case AW_MOD_CLK_UART2:
            SET_RESET(aw_ccu_reg->Apb2Reset.Uart2, reset);  break;
        case AW_MOD_CLK_UART3:
            SET_RESET(aw_ccu_reg->Apb2Reset.Uart3, reset);  break;
        case AW_MOD_CLK_UART4:
            SET_RESET(aw_ccu_reg->Apb2Reset.Uart4, reset);  break;
        case AW_MOD_CLK_UART5:
            SET_RESET(aw_ccu_reg->Apb2Reset.Uart5, reset);  break;
        case AW_MOD_CLK_GMAC:
            SET_RESET(aw_ccu_reg->AhbReset0.Gmac, reset);   break;
        case AW_MOD_CLK_DRAM:
            SET_RESET(aw_ccu_reg->AhbReset0.Sdram, reset);  break;
        case AW_MOD_CLK_USBOTG:
            SET_RESET(aw_ccu_reg->AhbReset0.Otg, reset);    break;
        case AW_MOD_CLK_DMA:
            SET_RESET(aw_ccu_reg->AhbReset0.Dma, reset);    break;
        case AW_MOD_CLK_HSTMR:
            SET_RESET(aw_ccu_reg->AhbReset0.HsTmr, reset);  break;
        case AW_MOD_CLK_MSGBOX:
            SET_RESET(aw_ccu_reg->AhbReset1.MsgBox, reset); break;
        case AW_MOD_CLK_SPINLOCK:
            SET_RESET(aw_ccu_reg->AhbReset1.SpinLock,reset);break;
        case AW_MOD_CLK_LVDS:
            SET_RESET(aw_ccu_reg->AhbReset2.Lvds, reset);   break;

        case AW_MOD_CLK_R_1WIRE:
            SET_RESET(aw_cpus_reg->ModReset.OneWire, reset);    break;
        case AW_MOD_CLK_R_CIR:
            SET_RESET(aw_cpus_reg->ModReset.Cir, reset);    break;
        case AW_MOD_CLK_R_TWI:
            SET_RESET(aw_cpus_reg->ModReset.Twi, reset);    break;
        case AW_MOD_CLK_R_UART:
            SET_RESET(aw_cpus_reg->ModReset.Uart, reset);    break;
        case AW_MOD_CLK_R_P2WI:
            SET_RESET(aw_cpus_reg->ModReset.P2wi, reset);    break;
        case AW_MOD_CLK_R_TMR:
            SET_RESET(aw_cpus_reg->ModReset.Tmr, reset);    break;

        default:
            break;
    }
    return 0;
}



/*
*********************************************************************************************************
*                           mod_clk_round_rate
*
*Description: round a rate of the given clock to a valid value;
*
*Arguments  : id    system clock id;
*             rate  clock rate for system clock;
*
*Return     : result
*
*Notes      :
*
*********************************************************************************************************
*/
static __u64 mod_clk_round_rate(__aw_ccu_clk_id_e id, __u64 rate)
{
    if(!rate) {
        rate = 1;
    }

    switch(id)
    {
        case AW_MOD_CLK_NAND0:
        case AW_MOD_CLK_NAND1:
        case AW_MOD_CLK_SDC0:
        case AW_MOD_CLK_SDC1:
        case AW_MOD_CLK_SDC2:
        case AW_MOD_CLK_SDC3:
        case AW_MOD_CLK_TS:
        case AW_MOD_CLK_SS:
        case AW_MOD_CLK_SPI0:
        case AW_MOD_CLK_SPI1:
        case AW_MOD_CLK_SPI2:
        case AW_MOD_CLK_SPI3:
        case AW_MOD_CLK_MDFS:
        case AW_MOD_CLK_MTCACC:
        case AW_MOD_CLK_MBUS0:
        case AW_MOD_CLK_MBUS1:
        case AW_MOD_CLK_IEPDRC0:
        case AW_MOD_CLK_IEPDRC1:
        case AW_MOD_CLK_IEPDEU0:
        case AW_MOD_CLK_IEPDEU1:
        case AW_MOD_CLK_GPUCORE:
        case AW_MOD_CLK_GPUMEM:
        case AW_MOD_CLK_GPUHYD:
        {
            int     low, high;
            __u64   parent_rate = sys_clk_ops.get_rate(mod_clk_get_parent(id));
            __u64   tmp_rate;

            tmp_rate = parent_rate + rate-1;
            do_div(tmp_rate, rate);

            low = 0;
            high = sizeof(module0_clk_div_tbl)/sizeof(struct module0_div_tbl) - 1;

            while(low<=high){
                if(module0_clk_div_tbl[(low+high)/2].Div < tmp_rate)
                {
                    low = (low+high)/2+1;
                } else if(module0_clk_div_tbl[(low+high)/2].Div > tmp_rate){
                    high = (low+high)/2-1;
                }
                else
                    break;
            }

            tmp_rate = (1<<module0_clk_div_tbl[(low+high)/2].FactorN) * (module0_clk_div_tbl[(low+high)/2].FactorM + 1);
            do_div(parent_rate, tmp_rate);
            return parent_rate;
        }

        case AW_MOD_CLK_DEBE0:
        case AW_MOD_CLK_DEBE1:
        case AW_MOD_CLK_DEFE0:
        case AW_MOD_CLK_DEFE1:
        case AW_MOD_CLK_DEMIX:
        case AW_MOD_CLK_LCD0CH0:
        case AW_MOD_CLK_LCD0CH1:
        case AW_MOD_CLK_LCD1CH0:
        case AW_MOD_CLK_LCD1CH1:
        case AW_MOD_CLK_HDMI:
        case AW_MOD_CLK_CSI0S:
        case AW_MOD_CLK_CSI0M:
        case AW_MOD_CLK_CSI1S:
        case AW_MOD_CLK_CSI1M:
        case AW_MOD_CLK_VE:
        case AW_MOD_CLK_MIPIDSIS:
        case AW_MOD_CLK_MIPIDSIP:
        case AW_MOD_CLK_MIPICSIS:
        case AW_MOD_CLK_MIPICSIP:
        {
            __u64   parent_rate = sys_clk_ops.get_rate(mod_clk_get_parent(id));
            __u64   tmp_rate = parent_rate+rate-1;

            do_div(tmp_rate, rate);

            if( tmp_rate > 16) {
                do_div(parent_rate, 16);
                return parent_rate;
            }
            else {
                tmp_rate = parent_rate+rate-1;
                do_div(tmp_rate, rate);
                do_div(parent_rate, tmp_rate);
                return parent_rate;
            }
        }

        default:
            return rate;
    }

    return rate;
}


__clk_ops_t mod_clk_ops = {
    .set_status = mod_clk_set_status,
    .get_status = mod_clk_get_status,
    .set_parent = mod_clk_set_parent,
    .get_parent = mod_clk_get_parent,
    .get_rate = mod_clk_get_rate_hz,
    .set_rate = mod_clk_set_rate_hz,
    .round_rate = mod_clk_round_rate,
    .get_reset = mod_clk_get_reset,
    .set_reset = mod_clk_set_reset,
};
