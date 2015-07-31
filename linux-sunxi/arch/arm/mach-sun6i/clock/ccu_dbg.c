/*
 *  arch/arm/mach-sun6i/clock/ccu_dbg.c
 *
 * Copyright (c) 2012 Allwinner.
 * kevin.z.m (kevin@allwinnertech.com)
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "ccm_i.h"


#define print_clk_inf(x, y)     do{printk(#x"."#y":%d\n", aw_ccu_reg->x.y);}while(0)

static void print_module0_clock(char *name, volatile __ccmu_module0_clk_t *reg)
{
    printk("\n%s clk infor:(0x%x)\n", name, (unsigned int)reg);
    printk("%s.DivM:%d\n", name, reg->DivM);
    printk("%s.OutClkCtrl:%d\n", name, reg->OutClkCtrl);
    printk("%s.DivN:%d\n", name, reg->DivN);
    printk("%s.SampClkCtrl:%d\n", name, reg->SampClkCtrl);
    printk("%s.ClkSrc:%d\n", name, reg->ClkSrc);
    printk("%s.ClkGate:%d\n", name, reg->ClkGate);
}

static void print_module1_clock(char *name, volatile __ccmu_module1_clk_t *reg)
{
    printk("\n%s clk infor:(0x%x)\n", name,(unsigned int)reg);
    printk("%s.ClkSrc:%d\n", name, reg->ClkSrc);
    printk("%s.ClkGate:%d\n", name, reg->ClkGate);
}


static void print_disp_clock(char *name, volatile __ccmu_disp_clk_t *reg)
{
    printk("\n%s clk infor:(0x%x)\n", name,(unsigned int)reg);
    printk("%s.DivM:%d\n", name, reg->DivM);
    printk("%s.ClkSrc:%d\n", name, reg->ClkSrc);
    printk("%s.ClkGate:%d\n", name, reg->ClkGate);
}


static void print_mediapll_clock(char *name, volatile __ccmu_media_pll_t *reg)
{
    printk("\n%s clk infor:(0x%x)\n", name,(unsigned int)reg);
    printk("%s.FactorM:%d\n", name, reg->FactorM);
    printk("%s.FactorN:%d\n", name, reg->FactorN);
    printk("%s.SdmEn:%d\n", name, reg->SdmEn);
    printk("%s.ModeSel:%d\n", name, reg->ModeSel);
    printk("%s.FracMod:%d\n", name, reg->FracMod);
    printk("%s.Lock:%d\n", name, reg->Lock);
    printk("%s.CtlMode:%d\n", name, reg->CtlMode);
    printk("%s.PLLEn:%d\n", name, reg->PLLEn);
}


void clk_dbg_inf(void)
{
    printk("---------------------------------------------\n");
    printk("    dump clock information                   \n");
    printk("---------------------------------------------\n");

    printk("\nPLL1 clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Pll1Ctl);
    print_clk_inf(Pll1Ctl, FactorM  );
    print_clk_inf(Pll1Ctl, FactorK  );
    print_clk_inf(Pll1Ctl, FactorN  );
    print_clk_inf(Pll1Ctl, SigmaEn  );
    print_clk_inf(Pll1Ctl, Lock     );
    print_clk_inf(Pll1Ctl, PLLEn    );

    printk("\nPLL2 clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Pll2Ctl);
    print_clk_inf(Pll2Ctl, FactorM  );
    print_clk_inf(Pll2Ctl, FactorN  );
    print_clk_inf(Pll2Ctl, FactorP  );
    print_clk_inf(Pll2Ctl, SdmEn    );
    print_clk_inf(Pll2Ctl, Lock     );
    print_clk_inf(Pll2Ctl, PLLEn    );

    printk("\nPLL3 clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Pll3Ctl);
    print_clk_inf(Pll3Ctl, FactorM  );
    print_clk_inf(Pll3Ctl, FactorN  );
    print_clk_inf(Pll3Ctl, SdmEn    );
    print_clk_inf(Pll3Ctl, ModeSel  );
    print_clk_inf(Pll3Ctl, FracMod  );
    print_clk_inf(Pll3Ctl, Lock     );
    print_clk_inf(Pll3Ctl, CtlMode  );
    print_clk_inf(Pll3Ctl, PLLEn    );

    print_mediapll_clock("Pll4Ctl", &aw_ccu_reg->Pll4Ctl);

    printk("\nPll5Ctl clk infor(0x%x)\n", (unsigned int)&aw_ccu_reg->Pll5Ctl);
    print_clk_inf(Pll5Ctl, FactorM      );
    print_clk_inf(Pll5Ctl, FactorK      );
    print_clk_inf(Pll5Ctl, FactorN      );
    print_clk_inf(Pll5Ctl, PLLCfgUpdate );
    print_clk_inf(Pll5Ctl, SigmaDeltaEn );
    print_clk_inf(Pll5Ctl, Lock         );
    print_clk_inf(Pll5Ctl, PLLEn        );

    printk("\nPll6Ctl clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Pll6Ctl);
    print_clk_inf(Pll6Ctl, FactorM      );
    print_clk_inf(Pll6Ctl, FactorK      );
    print_clk_inf(Pll6Ctl, FactorN      );
    print_clk_inf(Pll6Ctl, Pll24MPdiv   );
    print_clk_inf(Pll6Ctl, Pll24MOutEn  );
    print_clk_inf(Pll6Ctl, PllClkOutEn  );
    print_clk_inf(Pll6Ctl, PLLBypass    );
    print_clk_inf(Pll6Ctl, Lock         );
    print_clk_inf(Pll6Ctl, PLLEn        );

    print_mediapll_clock("Pll7Ctl", &aw_ccu_reg->Pll7Ctl);
    print_mediapll_clock("Pll8Ctl", &aw_ccu_reg->Pll8Ctl);

    printk("\nMipiPllCtl clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->MipiPllCtl);
    print_clk_inf(MipiPllCtl, FactorM   );
    print_clk_inf(MipiPllCtl, FactorK   );
    print_clk_inf(MipiPllCtl, FactorN   );
    print_clk_inf(MipiPllCtl, VfbSel    );
    print_clk_inf(MipiPllCtl, FeedBackDiv   );
    print_clk_inf(MipiPllCtl, SdmEn     );
    print_clk_inf(MipiPllCtl, PllSrc    );
    print_clk_inf(MipiPllCtl, Ldo2En    );
    print_clk_inf(MipiPllCtl, Ldo1En    );
    print_clk_inf(MipiPllCtl, Sel625Or750   );
    print_clk_inf(MipiPllCtl, SDiv2     );
    print_clk_inf(MipiPllCtl, FracMode  );
    print_clk_inf(MipiPllCtl, Lock      );
    print_clk_inf(MipiPllCtl, PLLEn     );

    print_mediapll_clock("Pll9Ctl", &aw_ccu_reg->Pll9Ctl);
    print_mediapll_clock("Pll10Ctl", &aw_ccu_reg->Pll10Ctl);

    printk("\nSysClkDiv clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->SysClkDiv);
    print_clk_inf(SysClkDiv, AXIClkDiv   );
    print_clk_inf(SysClkDiv, CpuClkSrc   );

    printk("\nAhb1Div clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Ahb1Div);
    print_clk_inf(Ahb1Div, Ahb1Div      );
    print_clk_inf(Ahb1Div, Ahb1PreDiv   );
    print_clk_inf(Ahb1Div, Apb1Div      );
    print_clk_inf(Ahb1Div, Ahb1ClkSrc   );

    printk("\nApb2Div clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Apb2Div);
    print_clk_inf(Apb2Div, DivM      );
    print_clk_inf(Apb2Div, DivN      );
    print_clk_inf(Apb2Div, ClkSrc      );

    printk("\nAxiGate clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->AxiGate);
    print_clk_inf(AxiGate, Sdram      );

    printk("\nAhbGate0 clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->AhbGate0);
    print_clk_inf(AhbGate0, MipiCsi      );
    print_clk_inf(AhbGate0, MipiDsi );
    print_clk_inf(AhbGate0, Ss      );
    print_clk_inf(AhbGate0, Dma     );
    print_clk_inf(AhbGate0, Sd0     );
    print_clk_inf(AhbGate0, Sd1     );
    print_clk_inf(AhbGate0, Sd2     );
    print_clk_inf(AhbGate0, Sd3     );
    print_clk_inf(AhbGate0, Nand1   );
    print_clk_inf(AhbGate0, Nand0   );
    print_clk_inf(AhbGate0, Dram    );
    print_clk_inf(AhbGate0, Gmac    );
    print_clk_inf(AhbGate0, Ts      );
    print_clk_inf(AhbGate0, HsTmr   );
    print_clk_inf(AhbGate0, Spi0    );
    print_clk_inf(AhbGate0, Spi1    );
    print_clk_inf(AhbGate0, Spi2    );
    print_clk_inf(AhbGate0, Spi3    );
    print_clk_inf(AhbGate0, Otg     );
    print_clk_inf(AhbGate0, Ehci0   );
    print_clk_inf(AhbGate0, Ehci1   );
    print_clk_inf(AhbGate0, Ohci0   );
    print_clk_inf(AhbGate0, Ohci1   );
    print_clk_inf(AhbGate0, Ohci2   );

    printk("\nAhbGate1 clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->AhbGate1);
    print_clk_inf(AhbGate1, Ve      );
    print_clk_inf(AhbGate1, Lcd0);
    print_clk_inf(AhbGate1, Lcd1);
    print_clk_inf(AhbGate1, Csi0);
    print_clk_inf(AhbGate1, Csi1);
    print_clk_inf(AhbGate1, Hdmi);
    print_clk_inf(AhbGate1, Be0);
    print_clk_inf(AhbGate1, Be1);
    print_clk_inf(AhbGate1, Fe0);
    print_clk_inf(AhbGate1, Fe1);
    print_clk_inf(AhbGate1, Mp);
    print_clk_inf(AhbGate1, Gpu);
    print_clk_inf(AhbGate1, MsgBox);
    print_clk_inf(AhbGate1, SpinLock);
    print_clk_inf(AhbGate1, Deu0);
    print_clk_inf(AhbGate1, Deu1);
    print_clk_inf(AhbGate1, Drc0);
    print_clk_inf(AhbGate1, Drc1);
    print_clk_inf(AhbGate1, MtcAcc);

    printk("\nApb1Gate clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Apb1Gate);
    print_clk_inf(Apb1Gate, Adda      );
    print_clk_inf(Apb1Gate, Spdif);
    print_clk_inf(Apb1Gate, Pio);
    print_clk_inf(Apb1Gate, I2s0);
    print_clk_inf(Apb1Gate, I2s1);

    printk("\nApb2Gate clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Apb2Gate);
    print_clk_inf(Apb2Gate, Twi0      );
    print_clk_inf(Apb2Gate, Twi1);
    print_clk_inf(Apb2Gate, Twi2);
    print_clk_inf(Apb2Gate, Twi3);
    print_clk_inf(Apb2Gate, Uart0);
    print_clk_inf(Apb2Gate, Uart1);
    print_clk_inf(Apb2Gate, Uart2);
    print_clk_inf(Apb2Gate, Uart3);
    print_clk_inf(Apb2Gate, Uart4);
    print_clk_inf(Apb2Gate, Uart5);

    print_module0_clock("Nand0", &aw_ccu_reg->Nand0);
    print_module0_clock("Nand1", &aw_ccu_reg->Nand1);
    print_module0_clock("Sd0", &aw_ccu_reg->Sd0);
    print_module0_clock("Sd1", &aw_ccu_reg->Sd1);
    print_module0_clock("Sd2", &aw_ccu_reg->Sd2);
    print_module0_clock("Sd3", &aw_ccu_reg->Sd3);
    print_module0_clock("Ts", &aw_ccu_reg->Ts);
    print_module0_clock("Ss", &aw_ccu_reg->Ss);
    print_module0_clock("Spi0", &aw_ccu_reg->Spi0);
    print_module0_clock("Spi1", &aw_ccu_reg->Spi1);
    print_module0_clock("Spi2", &aw_ccu_reg->Spi2);
    print_module0_clock("Spi3", &aw_ccu_reg->Spi3);

    print_module1_clock("I2s0", &aw_ccu_reg->I2s0);
    print_module1_clock("I2s1", &aw_ccu_reg->I2s1);
    print_module1_clock("Spdif", &aw_ccu_reg->Spdif);

    printk("\nUsb clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Usb);
    print_clk_inf(Usb, UsbPhy0Rst      );
    print_clk_inf(Usb, UsbPhy1Rst);
    print_clk_inf(Usb, UsbPhy2Rst);
    print_clk_inf(Usb, Phy0Gate);
    print_clk_inf(Usb, Phy1Gate);
    print_clk_inf(Usb, Phy2Gate);
    print_clk_inf(Usb, Ohci0Gate);
    print_clk_inf(Usb, Ohci1Gate);
    print_clk_inf(Usb, Ohci2Gate);

    print_module0_clock("Mdfs", &aw_ccu_reg->Mdfs);

    printk("\nDramCfg clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->DramCfg);
    print_clk_inf(DramCfg, Div1M      );
    print_clk_inf(DramCfg, ClkSrc1);
    print_clk_inf(DramCfg, Div0M);
    print_clk_inf(DramCfg, ClkSrc0);
    print_clk_inf(DramCfg, SdrClkUpd);
    print_clk_inf(DramCfg, CtrlerRst);

    printk("\nDramGate clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->DramGate);
    print_clk_inf(DramGate, Ve      );
    print_clk_inf(DramGate, CsiIsp);
    print_clk_inf(DramGate, Ts);
    print_clk_inf(DramGate, Drc0);
    print_clk_inf(DramGate, Drc1);
    print_clk_inf(DramGate, Deu0);
    print_clk_inf(DramGate, Deu1);
    print_clk_inf(DramGate, Fe0);
    print_clk_inf(DramGate, Fe1);
    print_clk_inf(DramGate, Be0);
    print_clk_inf(DramGate, Be1);
    print_clk_inf(DramGate, Mp);

    print_disp_clock("Be0", &aw_ccu_reg->Be0);
    print_disp_clock("Be1", &aw_ccu_reg->Be1);
    print_disp_clock("Fe0", &aw_ccu_reg->Fe0);
    print_disp_clock("Fe1", &aw_ccu_reg->Fe1);
    print_disp_clock("Mp", &aw_ccu_reg->Mp);
    print_disp_clock("Lcd0Ch0", &aw_ccu_reg->Lcd0Ch0);
    print_disp_clock("Lcd0Ch1", &aw_ccu_reg->Lcd0Ch1);
    print_disp_clock("Lcd1Ch0", &aw_ccu_reg->Lcd1Ch0);
    print_disp_clock("Lcd1Ch1", &aw_ccu_reg->Lcd1Ch1);

    printk("\nCsi0 clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Csi0);
    print_clk_inf(Csi0, MClkDiv      );
    print_clk_inf(Csi0, MClkSrc);
    print_clk_inf(Csi0, MClkGate);
    print_clk_inf(Csi0, SClkDiv);
    print_clk_inf(Csi0, SClkSrc);
    print_clk_inf(Csi0, SClkGate);

    printk("\nCsi1 clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Csi1);
    print_clk_inf(Csi1, MClkDiv      );
    print_clk_inf(Csi1, MClkSrc);
    print_clk_inf(Csi1, MClkGate);
    print_clk_inf(Csi1, SClkDiv);
    print_clk_inf(Csi1, SClkSrc);
    print_clk_inf(Csi1, SClkGate);

    printk("\nVe clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Ve);
    print_clk_inf(Ve, ClkDiv      );
    print_clk_inf(Ve, ClkGate      );

    print_module1_clock("Adda", &aw_ccu_reg->Adda);
    print_module1_clock("Avs", (volatile __ccmu_module1_clk_t *)&aw_ccu_reg->Avs);

    printk("\nHdmi clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Hdmi);
    print_clk_inf(Hdmi, ClkDiv);
    print_clk_inf(Hdmi, ClkSrc);
    print_clk_inf(Hdmi, DDCGate);
    print_clk_inf(Hdmi, ClkGate);

    print_module1_clock("Ps", &aw_ccu_reg->Ps);
    print_module0_clock("MtcAcc", &aw_ccu_reg->MtcAcc);
    print_module0_clock("MBus0", &aw_ccu_reg->MBus0);
    print_module0_clock("MBus1", &aw_ccu_reg->MBus1);

    printk("\nMipiDsi clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->MipiDsi);
    print_clk_inf(MipiDsi, PClkDiv);
    print_clk_inf(MipiDsi, PClkSrc);
    print_clk_inf(MipiDsi, PClkGate);
    print_clk_inf(MipiDsi, SClkDiv);
    print_clk_inf(MipiDsi, SClkSrc);
    print_clk_inf(MipiDsi, SClkGate);

    printk("\nMipiCsi clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->MipiCsi);
    print_clk_inf(MipiCsi, PClkDiv);
    print_clk_inf(MipiCsi, PClkSrc);
    print_clk_inf(MipiCsi, PClkGate);
    print_clk_inf(MipiCsi, SClkDiv);
    print_clk_inf(MipiCsi, SClkSrc);
    print_clk_inf(MipiCsi, SClkGate);

    print_module0_clock("IepDrc0", &aw_ccu_reg->IepDrc0);
    print_module0_clock("IepDrc1", &aw_ccu_reg->IepDrc1);
    print_module0_clock("IepDeu0", &aw_ccu_reg->IepDeu0);
    print_module0_clock("IepDeu1", &aw_ccu_reg->IepDeu1);
    print_module0_clock("GpuCore", &aw_ccu_reg->GpuCore);
    print_module0_clock("GpuMem", &aw_ccu_reg->GpuMem);
    print_module0_clock("GpuHyd", &aw_ccu_reg->GpuHyd);

    printk("\nPllLock clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->PllLock);
    print_clk_inf(PllLock, LockTime);

    printk("\nAhbReset0 clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->AhbReset0);
    print_clk_inf(AhbReset0, MipiDsi);
    print_clk_inf(AhbReset0, Ss);
    print_clk_inf(AhbReset0, Dma);
    print_clk_inf(AhbReset0, Sd0);
    print_clk_inf(AhbReset0, Sd1);
    print_clk_inf(AhbReset0, Sd2);
    print_clk_inf(AhbReset0, Sd3);
    print_clk_inf(AhbReset0, Nand1);
    print_clk_inf(AhbReset0, Nand0);
    print_clk_inf(AhbReset0, Sdram);
    print_clk_inf(AhbReset0, Gmac);
    print_clk_inf(AhbReset0, Ts);
    print_clk_inf(AhbReset0, HsTmr);
    print_clk_inf(AhbReset0, Spi0);
    print_clk_inf(AhbReset0, Spi1);
    print_clk_inf(AhbReset0, Spi2);
    print_clk_inf(AhbReset0, Spi3);
    print_clk_inf(AhbReset0, Otg);
    print_clk_inf(AhbReset0, Ehci0);
    print_clk_inf(AhbReset0, Ehci1);
    print_clk_inf(AhbReset0, Ohci0);
    print_clk_inf(AhbReset0, Ohci1);
    print_clk_inf(AhbReset0, Ohci2);

    printk("\nAhbReset1 clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->AhbReset1);
    print_clk_inf(AhbReset1, Ve);
    print_clk_inf(AhbReset1, Lcd0);
    print_clk_inf(AhbReset1, Lcd1);
    print_clk_inf(AhbReset1, Csi0);
    print_clk_inf(AhbReset1, Csi1);
    print_clk_inf(AhbReset1, Hdmi);
    print_clk_inf(AhbReset1, Be0);
    print_clk_inf(AhbReset1, Be1);
    print_clk_inf(AhbReset1, Fe0);
    print_clk_inf(AhbReset1, Fe1);
    print_clk_inf(AhbReset1, Mp);
    print_clk_inf(AhbReset1, Gpu);
    print_clk_inf(AhbReset1, MsgBox);
    print_clk_inf(AhbReset1, SpinLock);
    print_clk_inf(AhbReset1, Deu0);
    print_clk_inf(AhbReset1, Deu1);
    print_clk_inf(AhbReset1, Drc0);
    print_clk_inf(AhbReset1, Drc1);
    print_clk_inf(AhbReset1, MtcAcc);

    printk("\nAhbReset2 clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->AhbReset2);
    print_clk_inf(AhbReset2, Lvds);

    printk("\nApb1Reset clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Apb1Reset);
    print_clk_inf(Apb1Reset, Adda);
    print_clk_inf(Apb1Reset, Spdif);
    print_clk_inf(Apb1Reset, Pio);
    print_clk_inf(Apb1Reset, I2s0);
    print_clk_inf(Apb1Reset, I2s1);

    printk("\nApb2Reset clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Apb2Reset);
    print_clk_inf(Apb2Reset, Twi0);
    print_clk_inf(Apb2Reset, Twi1);
    print_clk_inf(Apb2Reset, Twi2);
    print_clk_inf(Apb2Reset, Twi3);
    print_clk_inf(Apb2Reset, Uart0);
    print_clk_inf(Apb2Reset, Uart1);
    print_clk_inf(Apb2Reset, Uart2);
    print_clk_inf(Apb2Reset, Uart3);
    print_clk_inf(Apb2Reset, Uart4);
    print_clk_inf(Apb2Reset, Uart5);

    printk("\nClkOutA clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->ClkOutA);
    print_clk_inf(ClkOutA, DivM);
    print_clk_inf(ClkOutA, DivN);
    print_clk_inf(ClkOutA, ClkSrc);
    print_clk_inf(ClkOutA, ClkEn);

    printk("\nClkOutB clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->ClkOutB);
    print_clk_inf(ClkOutB, DivM);
    print_clk_inf(ClkOutB, DivN);
    print_clk_inf(ClkOutB, ClkSrc);
    print_clk_inf(ClkOutB, ClkEn);

    printk("\nClkOutC clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->ClkOutC);
    print_clk_inf(ClkOutC, DivM);
    print_clk_inf(ClkOutC, DivN);
    print_clk_inf(ClkOutC, ClkSrc);
    print_clk_inf(ClkOutC, ClkEn);
}
EXPORT_SYMBOL(clk_dbg_inf);

#ifdef CONFIG_PROC_FS

#define sprintf_clk_inf(buf, x, y)     do{seq_printf(buf, "\t"#x"."#y":%d\n", aw_ccu_reg->x.y);}while(0)

static void sprintf_module0_clock(struct seq_file *buf, char *name, volatile __ccmu_module0_clk_t *reg)
{
    seq_printf(buf, "\n%s clk infor:(0x%x)\n", name,(unsigned int)reg);
    seq_printf(buf, "%s.DivM:%d\n", name, reg->DivM);
    seq_printf(buf, "%s.OutClkCtrl:%d\n", name, reg->OutClkCtrl);
    seq_printf(buf, "%s.DivN:%d\n", name, reg->DivN);
    seq_printf(buf, "%s.SampClkCtrl:%d\n", name, reg->SampClkCtrl);
    seq_printf(buf, "%s.ClkSrc:%d\n", name, reg->ClkSrc);
    seq_printf(buf, "%s.ClkGate:%d\n", name, reg->ClkGate);
}

static void sprintf_module1_clock(struct seq_file *buf, char *name, volatile __ccmu_module1_clk_t *reg)
{
    seq_printf(buf, "\n%s clk infor:(0x%x)\n", name,(unsigned int)reg);
    seq_printf(buf, "%s.ClkSrc:%d\n", name, reg->ClkSrc);
    seq_printf(buf, "%s.ClkGate:%d\n", name, reg->ClkGate);
}


static void sprintf_disp_clock(struct seq_file *buf, char *name, volatile __ccmu_disp_clk_t *reg)
{
    seq_printf(buf, "\n%s clk infor:(0x%x)\n", name,(unsigned int)reg);
    seq_printf(buf, "%s.DivM:%d\n", name, reg->DivM);
    seq_printf(buf, "%s.ClkSrc:%d\n", name, reg->ClkSrc);
    seq_printf(buf, "%s.ClkGate:%d\n", name, reg->ClkGate);
}


static void sprintf_mediapll_clock(struct seq_file *buf, char *name, volatile __ccmu_media_pll_t *reg)
{
    seq_printf(buf, "\n%s clk infor:(0x%x)\n", name,(unsigned int)reg);
    seq_printf(buf, "%s.FactorM:%d\n", name, reg->FactorM);
    seq_printf(buf, "%s.FactorN:%d\n", name, reg->FactorN);
    seq_printf(buf, "%s.SdmEn:%d\n", name, reg->SdmEn);
    seq_printf(buf, "%s.ModeSel:%d\n", name, reg->ModeSel);
    seq_printf(buf, "%s.FracMod:%d\n", name, reg->FracMod);
    seq_printf(buf, "%s.Lock:%d\n", name, reg->Lock);
    seq_printf(buf, "%s.CtlMode:%d\n", name, reg->CtlMode);
    seq_printf(buf, "%s.PLLEn:%d\n", name, reg->PLLEn);
}


static int ccmu_stats_show(struct seq_file *m, void *unused)
{
    seq_printf(m, "---------------------------------------------\n");
    seq_printf(m, "clock information:                           \n");
    seq_printf(m, "---------------------------------------------\n");

    seq_printf(m, "\nPLL1 infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Pll1Ctl);
    sprintf_clk_inf(m, Pll1Ctl, FactorM  );
    sprintf_clk_inf(m, Pll1Ctl, FactorK  );
    sprintf_clk_inf(m, Pll1Ctl, FactorN  );
    sprintf_clk_inf(m, Pll1Ctl, SigmaEn  );
    sprintf_clk_inf(m, Pll1Ctl, Lock     );
    sprintf_clk_inf(m, Pll1Ctl, PLLEn    );

    seq_printf(m, "\nPLL2 clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Pll2Ctl);
    sprintf_clk_inf(m, Pll2Ctl, FactorM  );
    sprintf_clk_inf(m, Pll2Ctl, FactorN  );
    sprintf_clk_inf(m, Pll2Ctl, FactorP  );
    sprintf_clk_inf(m, Pll2Ctl, SdmEn    );
    sprintf_clk_inf(m, Pll2Ctl, Lock     );
    sprintf_clk_inf(m, Pll2Ctl, PLLEn    );

    seq_printf(m, "\nPLL3 clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Pll3Ctl);
    sprintf_clk_inf(m, Pll3Ctl, FactorM  );
    sprintf_clk_inf(m, Pll3Ctl, FactorN  );
    sprintf_clk_inf(m, Pll3Ctl, SdmEn    );
    sprintf_clk_inf(m, Pll3Ctl, ModeSel  );
    sprintf_clk_inf(m, Pll3Ctl, FracMod  );
    sprintf_clk_inf(m, Pll3Ctl, Lock     );
    sprintf_clk_inf(m, Pll3Ctl, CtlMode  );
    sprintf_clk_inf(m, Pll3Ctl, PLLEn    );

    sprintf_mediapll_clock(m, "Pll4Ctl", &aw_ccu_reg->Pll4Ctl);

    seq_printf(m, "\nPll5Ctl clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Pll5Ctl);
    sprintf_clk_inf(m, Pll5Ctl, FactorM      );
    sprintf_clk_inf(m, Pll5Ctl, FactorK      );
    sprintf_clk_inf(m, Pll5Ctl, FactorN      );
    sprintf_clk_inf(m, Pll5Ctl, PLLCfgUpdate );
    sprintf_clk_inf(m, Pll5Ctl, SigmaDeltaEn );
    sprintf_clk_inf(m, Pll5Ctl, Lock         );
    sprintf_clk_inf(m, Pll5Ctl, PLLEn        );

    seq_printf(m, "\nPll6Ctl clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Pll6Ctl);
    sprintf_clk_inf(m, Pll6Ctl, FactorM      );
    sprintf_clk_inf(m, Pll6Ctl, FactorK      );
    sprintf_clk_inf(m, Pll6Ctl, FactorN      );
    sprintf_clk_inf(m, Pll6Ctl, Pll24MPdiv   );
    sprintf_clk_inf(m, Pll6Ctl, Pll24MOutEn  );
    sprintf_clk_inf(m, Pll6Ctl, PllClkOutEn  );
    sprintf_clk_inf(m, Pll6Ctl, PLLBypass    );
    sprintf_clk_inf(m, Pll6Ctl, Lock         );
    sprintf_clk_inf(m, Pll6Ctl, PLLEn        );

    sprintf_mediapll_clock(m, "Pll7Ctl", &aw_ccu_reg->Pll7Ctl);
    sprintf_mediapll_clock(m, "Pll8Ctl", &aw_ccu_reg->Pll8Ctl);

    seq_printf(m, "\nMipiPllCtl clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->MipiPllCtl);
    sprintf_clk_inf(m, MipiPllCtl, FactorM   );
    sprintf_clk_inf(m, MipiPllCtl, FactorK   );
    sprintf_clk_inf(m, MipiPllCtl, FactorN   );
    sprintf_clk_inf(m, MipiPllCtl, VfbSel    );
    sprintf_clk_inf(m, MipiPllCtl, FeedBackDiv   );
    sprintf_clk_inf(m, MipiPllCtl, SdmEn     );
    sprintf_clk_inf(m, MipiPllCtl, PllSrc    );
    sprintf_clk_inf(m, MipiPllCtl, Ldo2En    );
    sprintf_clk_inf(m, MipiPllCtl, Ldo1En    );
    sprintf_clk_inf(m, MipiPllCtl, Sel625Or750   );
    sprintf_clk_inf(m, MipiPllCtl, SDiv2     );
    sprintf_clk_inf(m, MipiPllCtl, FracMode  );
    sprintf_clk_inf(m, MipiPllCtl, Lock      );
    sprintf_clk_inf(m, MipiPllCtl, PLLEn     );

    sprintf_mediapll_clock(m, "Pll9Ctl", &aw_ccu_reg->Pll9Ctl);
    sprintf_mediapll_clock(m, "Pll10Ctl", &aw_ccu_reg->Pll10Ctl);

    seq_printf(m, "\nSysClkDiv clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->SysClkDiv);
    sprintf_clk_inf(m, SysClkDiv, AXIClkDiv   );
    sprintf_clk_inf(m, SysClkDiv, CpuClkSrc   );

    seq_printf(m, "\nAhb1Div clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Ahb1Div);
    sprintf_clk_inf(m, Ahb1Div, Ahb1Div      );
    sprintf_clk_inf(m, Ahb1Div, Ahb1PreDiv   );
    sprintf_clk_inf(m, Ahb1Div, Apb1Div      );
    sprintf_clk_inf(m, Ahb1Div, Ahb1ClkSrc   );

    seq_printf(m, "\nApb2Div clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Apb2Div);
    sprintf_clk_inf(m, Apb2Div, DivM      );
    sprintf_clk_inf(m, Apb2Div, DivN      );
    sprintf_clk_inf(m, Apb2Div, ClkSrc      );

    seq_printf(m, "\nAxiGate clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->AxiGate);
    sprintf_clk_inf(m, AxiGate, Sdram      );

    seq_printf(m, "\nAhbGate0 clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->AhbGate0);
    sprintf_clk_inf(m, AhbGate0, MipiCsi      );
    sprintf_clk_inf(m, AhbGate0, MipiDsi );
    sprintf_clk_inf(m, AhbGate0, Ss      );
    sprintf_clk_inf(m, AhbGate0, Dma     );
    sprintf_clk_inf(m, AhbGate0, Sd0     );
    sprintf_clk_inf(m, AhbGate0, Sd1     );
    sprintf_clk_inf(m, AhbGate0, Sd2     );
    sprintf_clk_inf(m, AhbGate0, Sd3     );
    sprintf_clk_inf(m, AhbGate0, Nand1   );
    sprintf_clk_inf(m, AhbGate0, Nand0   );
    sprintf_clk_inf(m, AhbGate0, Dram    );
    sprintf_clk_inf(m, AhbGate0, Gmac    );
    sprintf_clk_inf(m, AhbGate0, Ts      );
    sprintf_clk_inf(m, AhbGate0, HsTmr   );
    sprintf_clk_inf(m, AhbGate0, Spi0    );
    sprintf_clk_inf(m, AhbGate0, Spi1    );
    sprintf_clk_inf(m, AhbGate0, Spi2    );
    sprintf_clk_inf(m, AhbGate0, Spi3    );
    sprintf_clk_inf(m, AhbGate0, Otg     );
    sprintf_clk_inf(m, AhbGate0, Ehci0   );
    sprintf_clk_inf(m, AhbGate0, Ehci1   );
    sprintf_clk_inf(m, AhbGate0, Ohci0   );
    sprintf_clk_inf(m, AhbGate0, Ohci1   );
    sprintf_clk_inf(m, AhbGate0, Ohci2   );

    seq_printf(m, "\nAhbGate1 clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->AhbGate1);
    sprintf_clk_inf(m, AhbGate1, Ve      );
    sprintf_clk_inf(m, AhbGate1, Lcd0);
    sprintf_clk_inf(m, AhbGate1, Lcd1);
    sprintf_clk_inf(m, AhbGate1, Csi0);
    sprintf_clk_inf(m, AhbGate1, Csi1);
    sprintf_clk_inf(m, AhbGate1, Hdmi);
    sprintf_clk_inf(m, AhbGate1, Be0);
    sprintf_clk_inf(m, AhbGate1, Be1);
    sprintf_clk_inf(m, AhbGate1, Fe0);
    sprintf_clk_inf(m, AhbGate1, Fe1);
    sprintf_clk_inf(m, AhbGate1, Mp);
    sprintf_clk_inf(m, AhbGate1, Gpu);
    sprintf_clk_inf(m, AhbGate1, MsgBox);
    sprintf_clk_inf(m, AhbGate1, SpinLock);
    sprintf_clk_inf(m, AhbGate1, Deu0);
    sprintf_clk_inf(m, AhbGate1, Deu1);
    sprintf_clk_inf(m, AhbGate1, Drc0);
    sprintf_clk_inf(m, AhbGate1, Drc1);
    sprintf_clk_inf(m, AhbGate1, MtcAcc);

    seq_printf(m, "\nApb1Gate clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Apb1Gate);
    sprintf_clk_inf(m, Apb1Gate, Adda      );
    sprintf_clk_inf(m, Apb1Gate, Spdif);
    sprintf_clk_inf(m, Apb1Gate, Pio);
    sprintf_clk_inf(m, Apb1Gate, I2s0);
    sprintf_clk_inf(m, Apb1Gate, I2s1);

    seq_printf(m, "\nApb2Gate clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Apb2Gate);
    sprintf_clk_inf(m, Apb2Gate, Twi0      );
    sprintf_clk_inf(m, Apb2Gate, Twi1);
    sprintf_clk_inf(m, Apb2Gate, Twi2);
    sprintf_clk_inf(m, Apb2Gate, Twi3);
    sprintf_clk_inf(m, Apb2Gate, Uart0);
    sprintf_clk_inf(m, Apb2Gate, Uart1);
    sprintf_clk_inf(m, Apb2Gate, Uart2);
    sprintf_clk_inf(m, Apb2Gate, Uart3);
    sprintf_clk_inf(m, Apb2Gate, Uart4);
    sprintf_clk_inf(m, Apb2Gate, Uart5);

    sprintf_module0_clock(m, "Nand0", &aw_ccu_reg->Nand0);
    sprintf_module0_clock(m, "Nand1", &aw_ccu_reg->Nand1);
    sprintf_module0_clock(m, "Sd0", &aw_ccu_reg->Sd0);
    sprintf_module0_clock(m, "Sd1", &aw_ccu_reg->Sd1);
    sprintf_module0_clock(m, "Sd2", &aw_ccu_reg->Sd2);
    sprintf_module0_clock(m, "Sd3", &aw_ccu_reg->Sd3);
    sprintf_module0_clock(m, "Ts", &aw_ccu_reg->Ts);
    sprintf_module0_clock(m, "Ss", &aw_ccu_reg->Ss);
    sprintf_module0_clock(m, "Spi0", &aw_ccu_reg->Spi0);
    sprintf_module0_clock(m, "Spi1", &aw_ccu_reg->Spi1);
    sprintf_module0_clock(m, "Spi2", &aw_ccu_reg->Spi2);
    sprintf_module0_clock(m, "Spi3", &aw_ccu_reg->Spi3);

    sprintf_module1_clock(m, "I2s0", &aw_ccu_reg->I2s0);
    sprintf_module1_clock(m, "I2s1", &aw_ccu_reg->I2s1);
    sprintf_module1_clock(m, "Spdif", &aw_ccu_reg->Spdif);

    seq_printf(m, "\nUsb clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Usb);
    sprintf_clk_inf(m, Usb, UsbPhy0Rst      );
    sprintf_clk_inf(m, Usb, UsbPhy1Rst);
    sprintf_clk_inf(m, Usb, UsbPhy2Rst);
    sprintf_clk_inf(m, Usb, Phy0Gate);
    sprintf_clk_inf(m, Usb, Phy1Gate);
    sprintf_clk_inf(m, Usb, Phy2Gate);
    sprintf_clk_inf(m, Usb, Ohci0Gate);
    sprintf_clk_inf(m, Usb, Ohci1Gate);
    sprintf_clk_inf(m, Usb, Ohci2Gate);

    sprintf_module0_clock(m, "Mdfs", &aw_ccu_reg->Mdfs);

    seq_printf(m, "\nDramCfg clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->DramCfg);
    sprintf_clk_inf(m, DramCfg, Div1M      );
    sprintf_clk_inf(m, DramCfg, ClkSrc1);
    sprintf_clk_inf(m, DramCfg, Div0M);
    sprintf_clk_inf(m, DramCfg, ClkSrc0);
    sprintf_clk_inf(m, DramCfg, SdrClkUpd);
    sprintf_clk_inf(m, DramCfg, CtrlerRst);

    seq_printf(m, "\nDramGate clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->DramGate);
    sprintf_clk_inf(m, DramGate, Ve      );
    sprintf_clk_inf(m, DramGate, CsiIsp);
    sprintf_clk_inf(m, DramGate, Ts);
    sprintf_clk_inf(m, DramGate, Drc0);
    sprintf_clk_inf(m, DramGate, Drc1);
    sprintf_clk_inf(m, DramGate, Deu0);
    sprintf_clk_inf(m, DramGate, Deu1);
    sprintf_clk_inf(m, DramGate, Fe0);
    sprintf_clk_inf(m, DramGate, Fe1);
    sprintf_clk_inf(m, DramGate, Be0);
    sprintf_clk_inf(m, DramGate, Be1);
    sprintf_clk_inf(m, DramGate, Mp);

    sprintf_disp_clock(m, "Be0", &aw_ccu_reg->Be0);
    sprintf_disp_clock(m, "Be1", &aw_ccu_reg->Be1);
    sprintf_disp_clock(m, "Fe0", &aw_ccu_reg->Fe0);
    sprintf_disp_clock(m, "Fe1", &aw_ccu_reg->Fe1);
    sprintf_disp_clock(m, "Mp", &aw_ccu_reg->Mp);
    sprintf_disp_clock(m, "Lcd0Ch0", &aw_ccu_reg->Lcd0Ch0);
    sprintf_disp_clock(m, "Lcd0Ch1", &aw_ccu_reg->Lcd0Ch1);
    sprintf_disp_clock(m, "Lcd1Ch0", &aw_ccu_reg->Lcd1Ch0);
    sprintf_disp_clock(m, "Lcd1Ch1", &aw_ccu_reg->Lcd1Ch1);

    seq_printf(m, "\nCsi0 clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Csi0);
    sprintf_clk_inf(m, Csi0, MClkDiv      );
    sprintf_clk_inf(m, Csi0, MClkSrc);
    sprintf_clk_inf(m, Csi0, MClkGate);
    sprintf_clk_inf(m, Csi0, SClkDiv);
    sprintf_clk_inf(m, Csi0, SClkSrc);
    sprintf_clk_inf(m, Csi0, SClkGate);

    seq_printf(m, "\nCsi1 clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Csi1);
    sprintf_clk_inf(m, Csi1, MClkDiv      );
    sprintf_clk_inf(m, Csi1, MClkSrc);
    sprintf_clk_inf(m, Csi1, MClkGate);
    sprintf_clk_inf(m, Csi1, SClkDiv);
    sprintf_clk_inf(m, Csi1, SClkSrc);
    sprintf_clk_inf(m, Csi1, SClkGate);

    seq_printf(m, "\nVe clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Ve);
    sprintf_clk_inf(m, Ve, ClkDiv      );
    sprintf_clk_inf(m, Ve, ClkGate      );

    sprintf_module1_clock(m, "Adda", &aw_ccu_reg->Adda);
    sprintf_module1_clock(m, "Avs", (volatile __ccmu_module1_clk_t *)&aw_ccu_reg->Avs);

    seq_printf(m, "\nHdmi clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Hdmi);
    sprintf_clk_inf(m, Hdmi, ClkDiv);
    sprintf_clk_inf(m, Hdmi, ClkSrc);
    sprintf_clk_inf(m, Hdmi, DDCGate);
    sprintf_clk_inf(m, Hdmi, ClkGate);

    sprintf_module1_clock(m, "Ps", &aw_ccu_reg->Ps);
    sprintf_module0_clock(m, "MtcAcc", &aw_ccu_reg->MtcAcc);
    sprintf_module0_clock(m, "MBus0", &aw_ccu_reg->MBus0);
    sprintf_module0_clock(m, "MBus1", &aw_ccu_reg->MBus1);

    seq_printf(m, "\nMipiDsi clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->MipiDsi);
    sprintf_clk_inf(m, MipiDsi, PClkDiv);
    sprintf_clk_inf(m, MipiDsi, PClkSrc);
    sprintf_clk_inf(m, MipiDsi, PClkGate);
    sprintf_clk_inf(m, MipiDsi, SClkDiv);
    sprintf_clk_inf(m, MipiDsi, SClkSrc);
    sprintf_clk_inf(m, MipiDsi, SClkGate);

    seq_printf(m, "\nMipiCsi clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->MipiCsi);
    sprintf_clk_inf(m, MipiCsi, PClkDiv);
    sprintf_clk_inf(m, MipiCsi, PClkSrc);
    sprintf_clk_inf(m, MipiCsi, PClkGate);
    sprintf_clk_inf(m, MipiCsi, SClkDiv);
    sprintf_clk_inf(m, MipiCsi, SClkSrc);
    sprintf_clk_inf(m, MipiCsi, SClkGate);

    sprintf_module0_clock(m, "IepDrc0", &aw_ccu_reg->IepDrc0);
    sprintf_module0_clock(m, "IepDrc1", &aw_ccu_reg->IepDrc1);
    sprintf_module0_clock(m, "IepDeu0", &aw_ccu_reg->IepDeu0);
    sprintf_module0_clock(m, "IepDeu1", &aw_ccu_reg->IepDeu1);
    sprintf_module0_clock(m, "GpuCore", &aw_ccu_reg->GpuCore);
    sprintf_module0_clock(m, "GpuMem", &aw_ccu_reg->GpuMem);
    sprintf_module0_clock(m, "GpuHyd", &aw_ccu_reg->GpuHyd);

    seq_printf(m, "\nPllLock clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->PllLock);
    sprintf_clk_inf(m, PllLock, LockTime);

    seq_printf(m, "\nAhbReset0 clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->AhbReset0);
    sprintf_clk_inf(m, AhbReset0, MipiDsi);
    sprintf_clk_inf(m, AhbReset0, Ss);
    sprintf_clk_inf(m, AhbReset0, Dma);
    sprintf_clk_inf(m, AhbReset0, Sd0);
    sprintf_clk_inf(m, AhbReset0, Sd1);
    sprintf_clk_inf(m, AhbReset0, Sd2);
    sprintf_clk_inf(m, AhbReset0, Sd3);
    sprintf_clk_inf(m, AhbReset0, Nand1);
    sprintf_clk_inf(m, AhbReset0, Nand0);
    sprintf_clk_inf(m, AhbReset0, Sdram);
    sprintf_clk_inf(m, AhbReset0, Gmac);
    sprintf_clk_inf(m, AhbReset0, Ts);
    sprintf_clk_inf(m, AhbReset0, HsTmr);
    sprintf_clk_inf(m, AhbReset0, Spi0);
    sprintf_clk_inf(m, AhbReset0, Spi1);
    sprintf_clk_inf(m, AhbReset0, Spi2);
    sprintf_clk_inf(m, AhbReset0, Spi3);
    sprintf_clk_inf(m, AhbReset0, Otg);
    sprintf_clk_inf(m, AhbReset0, Ehci0);
    sprintf_clk_inf(m, AhbReset0, Ehci1);
    sprintf_clk_inf(m, AhbReset0, Ohci0);
    sprintf_clk_inf(m, AhbReset0, Ohci1);
    sprintf_clk_inf(m, AhbReset0, Ohci2);

    seq_printf(m, "\nAhbReset1 clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->AhbReset1);
    sprintf_clk_inf(m, AhbReset1, Ve);
    sprintf_clk_inf(m, AhbReset1, Lcd0);
    sprintf_clk_inf(m, AhbReset1, Lcd1);
    sprintf_clk_inf(m, AhbReset1, Csi0);
    sprintf_clk_inf(m, AhbReset1, Csi1);
    sprintf_clk_inf(m, AhbReset1, Hdmi);
    sprintf_clk_inf(m, AhbReset1, Be0);
    sprintf_clk_inf(m, AhbReset1, Be1);
    sprintf_clk_inf(m, AhbReset1, Fe0);
    sprintf_clk_inf(m, AhbReset1, Fe1);
    sprintf_clk_inf(m, AhbReset1, Mp);
    sprintf_clk_inf(m, AhbReset1, Gpu);
    sprintf_clk_inf(m, AhbReset1, MsgBox);
    sprintf_clk_inf(m, AhbReset1, SpinLock);
    sprintf_clk_inf(m, AhbReset1, Deu0);
    sprintf_clk_inf(m, AhbReset1, Deu1);
    sprintf_clk_inf(m, AhbReset1, Drc0);
    sprintf_clk_inf(m, AhbReset1, Drc1);
    sprintf_clk_inf(m, AhbReset1, MtcAcc);

    seq_printf(m, "\nAhbReset2 clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->AhbReset2);
    sprintf_clk_inf(m, AhbReset2, Lvds);

    seq_printf(m, "\nApb1Reset clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Apb1Reset);
    sprintf_clk_inf(m, Apb1Reset, Adda);
    sprintf_clk_inf(m, Apb1Reset, Spdif);
    sprintf_clk_inf(m, Apb1Reset, Pio);
    sprintf_clk_inf(m, Apb1Reset, I2s0);
    sprintf_clk_inf(m, Apb1Reset, I2s1);

    seq_printf(m, "\nApb2Reset clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->Apb2Reset);
    sprintf_clk_inf(m, Apb2Reset, Twi0);
    sprintf_clk_inf(m, Apb2Reset, Twi1);
    sprintf_clk_inf(m, Apb2Reset, Twi2);
    sprintf_clk_inf(m, Apb2Reset, Twi3);
    sprintf_clk_inf(m, Apb2Reset, Uart0);
    sprintf_clk_inf(m, Apb2Reset, Uart1);
    sprintf_clk_inf(m, Apb2Reset, Uart2);
    sprintf_clk_inf(m, Apb2Reset, Uart3);
    sprintf_clk_inf(m, Apb2Reset, Uart4);
    sprintf_clk_inf(m, Apb2Reset, Uart5);

    seq_printf(m, "\nClkOutA clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->ClkOutA);
    sprintf_clk_inf(m, ClkOutA, DivM);
    sprintf_clk_inf(m, ClkOutA, DivN);
    sprintf_clk_inf(m, ClkOutA, ClkSrc);
    sprintf_clk_inf(m, ClkOutA, ClkEn);

    seq_printf(m, "\nClkOutB clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->ClkOutB);
    sprintf_clk_inf(m, ClkOutB, DivM);
    sprintf_clk_inf(m, ClkOutB, DivN);
    sprintf_clk_inf(m, ClkOutB, ClkSrc);
    sprintf_clk_inf(m, ClkOutB, ClkEn);

    seq_printf(m, "\nClkOutC clk infor:(0x%x)\n", (unsigned int)&aw_ccu_reg->ClkOutC);
    sprintf_clk_inf(m, ClkOutC, DivM);
    sprintf_clk_inf(m, ClkOutC, DivN);
    sprintf_clk_inf(m, ClkOutC, ClkSrc);
    sprintf_clk_inf(m, ClkOutC, ClkEn);

	return 0;
}


static int ccmu_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, ccmu_stats_show, NULL);
}

static const struct file_operations ccmu_dbg_fops = {
	.owner = THIS_MODULE,
	.open = ccmu_stats_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init ccu_dbg_init(void)
{
	proc_create("ccmu", S_IRUGO, NULL, &ccmu_dbg_fops);
	return 0;
}

static void  __exit ccu_dbg_exit(void)
{
	remove_proc_entry("ccmu", NULL);
}

core_initcall(ccu_dbg_init);
module_exit(ccu_dbg_exit);
#endif

