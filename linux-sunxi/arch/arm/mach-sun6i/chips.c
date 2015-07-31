#include <mach/system.h>
#include <linux/module.h>

#define CCMU_REG_BASE   0xf1c20000
#define SSCTL_REG_BASE  0xf1c15000
#define RTC_REG_BASE    0xf1f00000
#undef uint32

typedef unsigned int uint32;
#undef mctl_read_w
#undef mctl_write_w
#define mctl_read_w(reg)        (*(volatile uint32 *)(reg))
#define mctl_write_w(reg, val)  (*(volatile uint32 *)(reg)) = (val)



static void enable_ss_clk(void)
{
    uint32 reg_val;

    //enable SS working clock
    reg_val = mctl_read_w(CCMU_REG_BASE + 0x9C); //CCM_SS_SCLK_CTRL
    //24MHz
    reg_val &= ~(0x3<<24);
    reg_val &= ~(0x3<<16);
    reg_val &= ~(0xf);
    reg_val |= 0x0<<16;
    reg_val |= 0;
    reg_val |= 0x1U<<31;
    mctl_write_w(CCMU_REG_BASE + 0x9C, reg_val);

    //enable SS AHB clock
    reg_val = mctl_read_w(CCMU_REG_BASE + 0x60);    //CCM_AHB1_GATE0_CTRL
    reg_val |= 0x1<<5;                              //SS AHB clock on
    mctl_write_w(CCMU_REG_BASE + 0x60, reg_val);
}

static void disable_ss_clk(void)
{
    uint32 reg_val;

    //disable SS working clock
    reg_val = mctl_read_w(CCMU_REG_BASE + 0x9C); //CCM_SS_SCLK_CTRL
    reg_val &= ~(0x1<<31);
    mctl_write_w(CCMU_REG_BASE + 0x9C, reg_val);

    //disable SS AHB clock
    reg_val = mctl_read_w(CCMU_REG_BASE + 0x60);    //CCM_AHB1_GATE0_CTRL
    reg_val &= ~(0x1<<5);                              //SS AHB clock on
    mctl_write_w(CCMU_REG_BASE + 0x60, reg_val);
}

enum sw_pac_id sw_get_pac_id(void)
{
    uint32 reg_val;
    uint32 id;
    enum sw_pac_id pac_id = PAC_ID_NULL;

    enable_ss_clk();
    reg_val = mctl_read_w(SSCTL_REG_BASE + 0x00);   //SS_CTL
    reg_val >>=16;
    reg_val &=0x3;
    disable_ss_clk();

    id = reg_val;
    if ((id == 0x0) || (id == 0x3))
        pac_id = PAC_ID_A31;
    else if(id == 0x1)
        pac_id = PAC_ID_A31S;
    else if(id == 0x2)
        pac_id = PAC_ID_A36;

    return pac_id;
}

enum sw_ic_ver sw_get_ic_ver(void)
{
    uint32 reg_val = 0x1;
    enum sw_ic_ver ic_ver = MAGIC_VER_NULL;

    mctl_write_w(RTC_REG_BASE + 0x20c, reg_val);
    reg_val = mctl_read_w(RTC_REG_BASE + 0x20c);
    if (reg_val == 0x1)
        ic_ver = MAGIC_VER_A;
    else if (reg_val == 0x0)
        ic_ver = MAGIC_VER_B;
    else if (reg_val == 0x2)
        ic_ver = MAGIC_VER_C;
    else if (reg_val == 0x100002)
        ic_ver = MAGIC_VER_D;
    else
        ic_ver = MAGIC_VER_D;

    return ic_ver;
}
EXPORT_SYMBOL(sw_get_ic_ver);
