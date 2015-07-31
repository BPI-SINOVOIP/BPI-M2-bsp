#include "pm_types.h"
#include "pm_i.h"

static __ccmu_reg_list_t   CmuReg;
static int i = 0;
/*
*********************************************************************************************************
*                                       MEM CCU INITIALISE
*
* Description: mem interrupt initialise.
*
* Arguments  : none.
*
* Returns    : 0/-1;
*********************************************************************************************************
*/
__s32 mem_ccu_save(__ccmu_reg_list_t *pReg)
{
	
	i = 1;
	while(i < 11){
		if(0 != i || 4 != i || 5 != i){
			//donot need care pll1 & pll5 & pll6, it is dangerous to 
			//write the reg after enable the pllx.
			CmuReg.PllxBias[i]	= pReg->PllxBias[i];
		}
		i++;
	}
	
	//CmuReg.Pll1Tun	= pReg->Pll1Tun;
	//CmuReg.Pll5Tun	= pReg->Pll5Tun;
	CmuReg.MipiPllTun	= pReg->MipiPllTun;

	// CmuReg.Pll1Ctl      	= pReg->Pll1Ctl;
	CmuReg.Pll2Ctl      	= pReg->Pll2Ctl;
	CmuReg.Pll3Ctl      	= pReg->Pll3Ctl;
	CmuReg.Pll4Ctl      	= pReg->Pll4Ctl;
	CmuReg.Pll6Ctl      	= pReg->Pll6Ctl;
	CmuReg.Pll7Ctl      	= pReg->Pll7Ctl;
	CmuReg.Pll8Ctl      	= pReg->Pll8Ctl;
	CmuReg.MipiPllCtl   	= pReg->MipiPllCtl;
	CmuReg.Pll9Ctl      	= pReg->Pll9Ctl;
	CmuReg.Pll10Ctl      	= pReg->Pll10Ctl;

	CmuReg.SysClkDiv    	= pReg->SysClkDiv;
	CmuReg.Ahb1Div  	= pReg->Ahb1Div;
	CmuReg.Apb2Div   	= pReg->Apb2Div;
	CmuReg.AxiGate      	= pReg->AxiGate;
	CmuReg.AhbGate0     	= pReg->AhbGate0;
	CmuReg.AhbGate1     	= pReg->AhbGate1;
	CmuReg.Apb1Gate     	= pReg->Apb1Gate;
	CmuReg.Apb2Gate    	= pReg->Apb2Gate;
	CmuReg.Nand0		= pReg->Nand0;
	CmuReg.Nand1      	= pReg->Nand1;

	CmuReg.Sd0	    	= pReg->Sd0;
	CmuReg.Sd1  		= pReg->Sd1;
	CmuReg.Sd2    		= pReg->Sd2;
	CmuReg.Sd3    		= pReg->Sd3;
	CmuReg.Ts		= pReg->Ts;
	CmuReg.Ss        	= pReg->Ss;
	CmuReg.Spi0      	= pReg->Spi0;
	CmuReg.Spi1      	= pReg->Spi1;
	CmuReg.Spi2      	= pReg->Spi2;
	CmuReg.Spi3      	= pReg->Spi3;

	CmuReg.I2s0       	= pReg->I2s0;
	CmuReg.I2s1       	= pReg->I2s1;
	CmuReg.Spdif	    	= pReg->Spdif;
	CmuReg.Usb	      	= pReg->Usb;
	CmuReg.Mdfs		= pReg->Mdfs;
	CmuReg.DramGate     	= pReg->DramGate;
	
	CmuReg.Be0     		= pReg->Be0;
	CmuReg.Be1    		= pReg->Be1;
	CmuReg.Fe0    		= pReg->Fe0;
	CmuReg.Fe1    		= pReg->Fe1;
	CmuReg.Mp      		= pReg->Mp;
	CmuReg.Lcd0Ch0   	= pReg->Lcd0Ch0;
	CmuReg.Lcd1Ch0   	= pReg->Lcd1Ch0;
	CmuReg.Lcd0Ch1   	= pReg->Lcd0Ch1;
	CmuReg.Lcd1Ch1   	= pReg->Lcd1Ch1;
	CmuReg.Csi0      	= pReg->Csi0;
	CmuReg.Csi1      	= pReg->Csi1;
	CmuReg.Ve        	= pReg->Ve;
	CmuReg.Adda      	= pReg->Adda;
	CmuReg.Avs       	= pReg->Avs;
	CmuReg.Hdmi      	= pReg->Hdmi;
	CmuReg.Ps     	 	= pReg->Ps;
	CmuReg.MtcAcc    	= pReg->MtcAcc;

	CmuReg.MBus0    	= pReg->MBus0;
	CmuReg.MBus1    	= pReg->MBus1;
	CmuReg.MipiDsi    	= pReg->MipiDsi;
	CmuReg.MipiCsi    	= pReg->MipiCsi;
	CmuReg.IepDrc0		= pReg->IepDrc0;
	CmuReg.IepDrc1		= pReg->IepDrc1;
	CmuReg.IepDeu0		= pReg->IepDeu0;
	CmuReg.IepDeu1		= pReg->IepDeu1;

	CmuReg.GpuCore		= pReg->GpuCore;
	CmuReg.GpuMem		= pReg->GpuMem;
	CmuReg.GpuHyd		= pReg->GpuHyd;

	CmuReg.PllLock		= pReg->PllLock;
	CmuReg.Pll1Lock		= pReg->Pll1Lock;
	
	CmuReg.AhbReset0	= pReg->AhbReset0;
	CmuReg.AhbReset1	= pReg->AhbReset1;
	CmuReg.AhbReset2	= pReg->AhbReset2;

	CmuReg.Apb1Reset	= pReg->Apb1Reset;
	CmuReg.Apb2Reset	= pReg->Apb2Reset;

	CmuReg.ClkOutA		= pReg->ClkOutA;
	CmuReg.ClkOutB		= pReg->ClkOutB;
	CmuReg.ClkOutC		= pReg->ClkOutC;

	return 0;
}

__s32 mem_ccu_restore(__ccmu_reg_list_t *pReg)
{
	i = 1;
	while(i < 11){
		if(0 != i || 4 != i || 5 != i){
			//donot need care pll1 & pll5 & pll6, it is dangerous to 
			//write the reg after enable the pllx.
			pReg->PllxBias[i]	= CmuReg.PllxBias[i];
		}
		i++;
	}
	
	//pReg->Pll1Tun		= CmuReg.Pll1Tun;
	//pReg->Pll5Tun		= CmuReg.Pll5Tun;
	pReg->MipiPllTun	= CmuReg.MipiPllTun;

	// pReg->Pll1Ctl      	= CmuReg.Pll1Ctl;
	pReg->Pll2Ctl      	= CmuReg.Pll2Ctl;
	pReg->Pll3Ctl      	= CmuReg.Pll3Ctl;
	pReg->Pll4Ctl      	= CmuReg.Pll4Ctl;
	pReg->Pll6Ctl      	= CmuReg.Pll6Ctl;
	pReg->Pll7Ctl      	= CmuReg.Pll7Ctl;
	pReg->Pll8Ctl      	= CmuReg.Pll8Ctl;
	pReg->MipiPllCtl   	= CmuReg.MipiPllCtl;
	pReg->Pll9Ctl     	= CmuReg.Pll9Ctl;
	pReg->Pll10Ctl    	= CmuReg.Pll10Ctl;

	pReg->SysClkDiv    	= CmuReg.SysClkDiv;
	pReg->Ahb1Div  		= CmuReg.Ahb1Div;
	pReg->Apb2Div   	= CmuReg.Apb2Div;
	pReg->AxiGate      	= CmuReg.AxiGate;
	pReg->AhbGate0     	= CmuReg.AhbGate0;
	pReg->AhbGate1     	= CmuReg.AhbGate1;
	pReg->Apb1Gate     	= CmuReg.Apb1Gate;
	pReg->Apb2Gate     	= CmuReg.Apb2Gate;
	pReg->Nand0		= CmuReg.Nand0;
	pReg->Nand1      	= CmuReg.Nand1;

	pReg->Sd0	    	= CmuReg.Sd0;
	pReg->Sd1  		= CmuReg.Sd1;
	pReg->Sd2    		= CmuReg.Sd2;
	pReg->Sd3    		= CmuReg.Sd3;
	pReg->Ts		= CmuReg.Ts;
	pReg->Ss        	= CmuReg.Ss;
	pReg->Spi0      	= CmuReg.Spi0;
	pReg->Spi1      	= CmuReg.Spi1;
	pReg->Spi2      	= CmuReg.Spi2;
	pReg->Spi3      	= CmuReg.Spi3;

	pReg->I2s0       	= CmuReg.I2s0;
	pReg->I2s1       	= CmuReg.I2s1;
	pReg->Spdif	    	= CmuReg.Spdif;
	pReg->Usb	      	= CmuReg.Usb;
	pReg->Mdfs		= CmuReg.Mdfs;
	pReg->DramGate    	= CmuReg.DramGate;

	pReg->Be0     		= CmuReg.Be0;
	pReg->Be1    		= CmuReg.Be1;
	pReg->Fe0    		= CmuReg.Fe0;
	pReg->Fe1    		= CmuReg.Fe1;
	pReg->Mp      		= CmuReg.Mp;
	pReg->Lcd0Ch0   	= CmuReg.Lcd0Ch0;
	pReg->Lcd1Ch0   	= CmuReg.Lcd1Ch0;
	pReg->Lcd0Ch1   	= CmuReg.Lcd0Ch1;
	pReg->Lcd1Ch1   	= CmuReg.Lcd1Ch1;
	pReg->Csi0      	= CmuReg.Csi0;
	pReg->Csi1      	= CmuReg.Csi1;
	pReg->Ve        	= CmuReg.Ve;
	pReg->Adda      	= CmuReg.Adda;
	pReg->Avs       	= CmuReg.Avs;
	pReg->Hdmi      	= CmuReg.Hdmi;
	pReg->Ps     	 	= CmuReg.Ps;
	pReg->MtcAcc    	= CmuReg.MtcAcc;

	pReg->MBus0    		= CmuReg.MBus0;
	pReg->MBus1    		= CmuReg.MBus1;
	pReg->MipiDsi    	= CmuReg.MipiDsi;
	pReg->MipiCsi    	= CmuReg.MipiCsi;
	pReg->IepDrc0		= CmuReg.IepDrc0;
	pReg->IepDrc1		= CmuReg.IepDrc1;
	pReg->IepDeu0		= CmuReg.IepDeu0;
	pReg->IepDeu1		= CmuReg.IepDeu1;

	pReg->GpuCore		= CmuReg.GpuCore;
	pReg->GpuMem		= CmuReg.GpuMem;
	pReg->GpuHyd		= CmuReg.GpuHyd;

	pReg->PllLock		= CmuReg.PllLock;
	pReg->Pll1Lock		= CmuReg.Pll1Lock;

	pReg->AhbReset0		= CmuReg.AhbReset0;
	pReg->AhbReset1		= CmuReg.AhbReset1;
	pReg->AhbReset2		= CmuReg.AhbReset2;

	pReg->Apb1Reset		= CmuReg.Apb1Reset;
	pReg->Apb2Reset		= CmuReg.Apb2Reset;

	pReg->ClkOutA		= CmuReg.ClkOutA;
	pReg->ClkOutB		= CmuReg.ClkOutB;
	pReg->ClkOutC		= CmuReg.ClkOutC;

	return 0;
}

