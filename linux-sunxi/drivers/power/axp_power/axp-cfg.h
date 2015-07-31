#ifndef __LINUX_AXP_CFG_H_
#define __LINUX_AXP_CFG_H_

#define AXP22_ADDR			0x68 >> 1
#define BATRDC				  100 //initial rdc


#define AXP22LDO1			3000



#define AXP22_VOL_MAX		  1   // capability buffer length
#define AXP22_TIME_MAX		20
#define AXP22_AVER_MAX		10
#define AXP22_RDC_COUNT		10

#define ABS(x)				((x) >0 ? (x) : -(x) )

#define INTCHGCUR				300000								//set initial charging current limite
#define SUSCHGCUR				1000000								//set suspend charging current limite
#define RESCHGCUR				INTCHGCUR							//set resume charging current limite
#define CLSCHGCUR				SUSCHGCUR							//set shutdown charging current limite
#define INTCHGVOL				4200000								//set initial charing target voltage
#define INTCHGENDRATE		10										//set initial charing end current	rate
#define INTCHGENABLED		1										  //set initial charing enabled
#define INTADCFREQ			25										//set initial adc frequency
#define INTADCFREQC			100										//set initial coulomb adc coufrequency
#define INTCHGPRETIME		50										//set initial pre-charging time
#define INTCHGCSTTIME		480										//set initial pre-charging time
#define BATMAXVOL				4200000								//set battery max design volatge
#define BATMINVOL				3500000								//set battery min design volatge

#ifdef	CONFIG_AW_AXP22
#define OCVREG0				0x00								//2.99V
#define OCVREG1				0x00								//3.13V
#define OCVREG2				0x00								//3.27V
#define OCVREG3				0x00								//3.34V
#define OCVREG4				0x00								//3.41V
#define OCVREG5				0x00								//3.48V
#define OCVREG6				0x00								//3.52V
#define OCVREG7				0x00								//3.55V
#define OCVREG8				0x04								//3.57V
#define OCVREG9				0x05								//3.59V
#define OCVREGA				0x06								//3.61V
#define OCVREGB				0x07								//3.63V
#define OCVREGC				0x0a								//3.64V
#define OCVREGD				0x0d								//3.66V
#define OCVREGE				0x1a								//3.7V 
#define OCVREGF				0x24								//3.73V
#define OCVREG10		 	0x29                //3.77V
#define OCVREG11		 	0x2e                //3.78V
#define OCVREG12		 	0x32                //3.8V 
#define OCVREG13		 	0x35                //3.84V
#define OCVREG14		 	0x39                //3.85V
#define OCVREG15		 	0x3d                //3.87V
#define OCVREG16		 	0x43                //3.91V
#define OCVREG17		 	0x49                //3.94V
#define OCVREG18		 	0x4f                //3.98V
#define OCVREG19		 	0x54                //4.01V
#define OCVREG1A		 	0x58                //4.05V
#define OCVREG1B		 	0x5c                //4.08V
#define OCVREG1C		 	0x5e                //4.1V 
#define OCVREG1D		 	0x60                //4.12V
#define OCVREG1E		 	0x62                //4.14V
#define OCVREG1F		 	0x64                //4.15V
#endif

extern int pmu_used;
extern int pmu_twi_id;
extern int pmu_twi_addr;
extern int pmu_battery_rdc;
extern int pmu_battery_cap;
extern int pmu_batdeten;
extern int pmu_runtime_chgcur;
extern int pmu_earlysuspend_chgcur;
extern int pmu_suspend_chgcur;
extern int pmu_resume_chgcur;
extern int pmu_shutdown_chgcur;
extern int pmu_init_chgvol;
extern int pmu_init_chgend_rate;
extern int pmu_init_chg_enabled;
extern int pmu_init_chg_enabled;
extern int pmu_init_adc_freq;
extern int pmu_init_adc_freqc;
extern int pmu_init_chg_pretime;
extern int pmu_init_chg_csttime;
extern int pmu_batt_cap_correct;
extern int pmu_bat_regu_en;


extern int pmu_bat_para1;
extern int pmu_bat_para2;
extern int pmu_bat_para3;
extern int pmu_bat_para4;
extern int pmu_bat_para5;
extern int pmu_bat_para6;
extern int pmu_bat_para7;
extern int pmu_bat_para8;
extern int pmu_bat_para9;
extern int pmu_bat_para10;
extern int pmu_bat_para11;
extern int pmu_bat_para12;
extern int pmu_bat_para13;
extern int pmu_bat_para14;
extern int pmu_bat_para15;
extern int pmu_bat_para16;
extern int pmu_bat_para17;
extern int pmu_bat_para18;
extern int pmu_bat_para19;
extern int pmu_bat_para20;
extern int pmu_bat_para21;
extern int pmu_bat_para22;
extern int pmu_bat_para23;
extern int pmu_bat_para24;
extern int pmu_bat_para25;
extern int pmu_bat_para26;
extern int pmu_bat_para27;
extern int pmu_bat_para28;
extern int pmu_bat_para29;
extern int pmu_bat_para30;
extern int pmu_bat_para31;
extern int pmu_bat_para32;

extern int pmu_usbvol_limit;
extern int pmu_usbvol;
extern int pmu_usbcur_limit;
extern int pmu_usbcur;
extern int pmu_usbvol_pc;
extern int pmu_usbcur_pc;

extern int pmu_pwroff_vol;
extern int pmu_pwron_vol;

extern int dcdc1_vol;
extern int dcdc2_vol;
extern int dcdc3_vol;
extern int dcdc4_vol;
extern int dcdc5_vol;
extern int aldo2_vol;
extern int aldo3_vol;

extern  int pmu_pekoff_time;           
extern  int pmu_pekoff_func;          
extern  int pmu_pekoff_en;			    
extern  int pmu_peklong_time;          
extern  int pmu_pekon_time;         
extern  int pmu_pwrok_time;

extern  int pmu_battery_warning_level1;
extern  int pmu_battery_warning_level2;
        
extern  int pmu_restvol_time;          
extern  int pmu_ocv_cou_adjust_time;   
extern  int pmu_chgled_func;           
extern  int pmu_chgled_type;           
extern  int pmu_vbusen_func;			    
extern  int pmu_reset;           
extern  int pmu_IRQ_wakeup;
extern  int pmu_hot_shutdowm;           
extern  int pmu_inshort;

extern int pmu_temp_protect_en;
extern int pmu_charge_ltf;
extern int pmu_charge_htf;
extern int pmu_discharge_ltf;
extern int pmu_discharge_htf;
extern int pmu_temp_para1;
extern int pmu_temp_para2;
extern int pmu_temp_para3;
extern int pmu_temp_para4;
extern int pmu_temp_para5;
extern int pmu_temp_para6;
extern int pmu_temp_para7;
extern int pmu_temp_para8;
extern int pmu_temp_para9;
extern int pmu_temp_para10;
extern int pmu_temp_para11;
extern int pmu_temp_para12;
extern int pmu_temp_para13;
extern int pmu_temp_para14;
extern int pmu_temp_para15;
extern int pmu_temp_para16;

extern int axp_script_parser_fetch(char *main, char *sub, u32 *val, u32 size);

#endif
