#ifndef __STM8_INFO__
#define __STM8_INFO__

#if 0
//TEST FOR pc7  LED0
#define PD_CFG3_REG_OFFSET 0x48
#define PD27_SELECT_POS 28
#define PD_DATA_REG_OFFSET 0x58
#define PD27_DATA_POS   7
#define PD_DRV1_REG_OFFSET 0X5c
#define PD27_DRV_POS 	14
#define PD_PULL1_REG_OFFSET 0X64
#define PD27_PULL_POS	14

//test for PD24 key sw3
#define PD_CFG3_REG_OFFSET 0X78
#define PD27_SELECT_POS 0
#define PD_DATA_REG_OFFSET 0X7C
#define PD27_DATA_POS   24
#define PD_DRV1_REG_OFFSET 0X84
#define PD27_DRV_POS 	16
#define PD_PULL1_REG_OFFSET 0X8c
#define PD27_PULL_POS	16

/*  by Hensen 2018.  //TO save time so no val check, please care about for that.
	name: port name, eg. PG6, PG7, PG8...
	func: multi sel val: 0 - input, 1 - output... 
	pull:  pull val: 0 - pull up/down disable, 1 - pull up... , 2-pull down
	drv: driver level val: 0 - level 0, 1 - level 1...
	data: data val: 0 - low, 1 - high, only vaild when mul_sel is input/output
*/
static inline void swim_pin_high(void){

	//func ,0x1, output
	reg_writel((reg_readl(pp->pd_cfg3_reg) & (~(0x7 << PD27_SELECT_POS))) | (0x1 << PD27_SELECT_POS), pp->pd_cfg3_reg);
	//date, 0x1
	reg_writel((reg_readl(pp->pd_data_reg) & (~(0x1 << PD27_DATA_POS))) | (0x1 << PD27_DATA_POS), pp->pd_data_reg);
	//drv level 0x3
	reg_writel((reg_readl(pp->pd_drv1_reg) & (~(0X3 << PD27_DRV_POS))) | (0x3 << PD27_DRV_POS), pp->pd_drv1_reg);
	//pull 0x1 pull up
	reg_writel((reg_readl(pp->pd_pull1_reg) & (~(0X3 << PD27_PULL_POS))) | (0x1 << PD27_PULL_POS), pp->pd_pull1_reg);

}
#endif

#include <linux/spinlock.h>
#include <asm/io.h>
#include <linux/time.h>

#define RST "PD26"
#define SWIM "PD27"
#define TIMER_BASE_ADDR 0x01C20C00
#define TMR_IRQ_EN_OFF	0X0
#define TMR_IRQ_STA_OFF	0X04
#define TMR_0_CTRL_OFF	0X10
#define TMR_0_INTV_OFF	0x14
#define TMR_0_CURRENT_OFF 0x18

#define PWM "PD28"
#define PWM_BASE_ADDR 0x01C21400
#define PWM_CH_CTRL_OFFSET 0x00
#define PWM_CH0_PERIOD_OFFSET 0x04
#define PWM_CH1_PERIOD_OFFSET 0x08

#define PWM0_RDY_POS 		28	//0: pwm0 period reg is ready to write, 1:busy
#define PWM0_BYPASS_POS 	9	//0: disable ch0 bypass, 1: ch0 OSC24Mhz  enable
#define PWM_CH0_PUL_START_POS	8	//0: no effect, 1: output 1 pulse( pulse width ref on period 0 register [15:0])
#define PWM_CHANNEL0_MODE_POS	7	//0:cycle mode, 1: pulse mode
#define SCLK_CH0_GATING_POS	6	//0: mask, 1: pass. gating the special clock for pwm0
#define PWM_CH0_ACT_STA_POS	5	//0:low level, 1: high level. pwm ch0 active state
#define PWM_CH0_EN_POS	4	//0:disable, 1:enable. pwm ch0 enable 
#define PWM_CH0_PRESCAL_POS	0	//PWM ch0 prescalar
#define PWM_CH0_PRESCAL_BITFIELDS_MASK	0xf
/* This bts should be setting before the PWM Channel 0 clock gate on.
 * 0000:/120	0001:/180	0010:/240	0011:/360
 * 0100:/480	0101:/		0110:/		0111:/
 * 1000:/12k	1001:/24k	1010:/36k	1011:/48k
 * 1100:/72k	1101:/		1110:/		1111:/1
*/
#define PWM_CH0_PRESCAL_VAL	0xf

/*Note: If the register need to modified dynamically, the PCLK should be faster than the PWM CLK 
 * 
 * (PWM CLK = 24MHz/pre-scale)
 *
*/
#define PWM_CH0_ENTIRE_CYS_POS	16	//0:1cycle, 1:2cycles, ..., n: n+1cycles(Number of the entire cycles in the PWM clock)
#define PWM_CH0_ENTIRE_CYS_BITFIELDS_MASK	0xffff
#define PWM_CH0_ENTIRE_ACT_CYS_POS	0	//	//0:1cycle, 1:2cycles, ..., n: n+1cycles(Number of the active cycles in the PWM clock)
#define PWM_CH0_ENTIRE_ACT_CYS_BITFIELDS_MASK	0xffff

#define CYCLE_MODE 0x0
#define PULSE_MODE 0x1
#define ENTIRE_CYS_CNT 0x42		//22*3 = 16*4 + 2 = 0x42
#define ACT_CYS_CNT0	0x3c	//20*3 = 60 = 0x3c
#define ACT_CYS_CNT1	0x6		//2*3 = 6 = 0x6
#define ACT_CYS_CNT3	0X3		//1*3 = 3 = 0X3
#define ACT_CYS_CNT2	0xc		//4*3 = 12 = 0xc
#define ACT_CYS_CNT_CLEAR 0x1
#define ACT_CYS_CNT_ZERO 0x0

#define PULSE_STATE_LOW 0x0
#define PULSE_STATE_HIGH 0x1



#define RST_CHK_TIMEOUT 1000

#define PORT_IO_BASEADDR 0x01c20800

#define PD_CFG0_REG_OFFSET 0X6c
#define PD_CFG1_REG_OFFSET 0X70
#define PD_CFG2_REG_OFFSET 0X74
#define PD_CFG3_REG_OFFSET 0X78
#define PD28_SELECT_POS 16
#define PD27_SELECT_POS 12
#define PD26_SELECT_POS 8
#define PD_DATA_REG_OFFSET 0X7C
#define PD28_DATA_POS   28
#define PD27_DATA_POS   27
#define PD26_DATA_POS   26
#define PD_DRV0_REG_OFFSET 0X80
#define PD_DRV1_REG_OFFSET 0X84
#define PD28_DRV_POS 	24
#define PD27_DRV_POS 	22
#define PD26_DRV_POS 	20
#define PD_PULL0_REG_OFFSET 0X88
#define PD_PULL1_REG_OFFSET 0X8c
#define PD28_PULL_POS	24
#define PD27_PULL_POS	22
#define PD26_PULL_POS	20


#define STM8_SWIM_DEVICE_NAME "swim"
#define SWIM_CMD_LEN                    3
#define SWIM_CMD_SRST                   0x00
#define SWIM_CMD_ROTF                   0x01
#define SWIM_CMD_WOTF                   0x02

#define SWIM_MAX_RESEND_CNT             20
#define A83T_IOCTL_MAGIC 'H'
#define SWIM_IOCTL_RESET 				1
#define A83T_PWM_DUTY_CYCLE_IOCTL	_IOWR(A83T_IOCTL_MAGIC, 0, communication_info_t)
#define A83T_PWM_PULSE_IOCTL		_IOWR(A83T_IOCTL_MAGIC, 1, communication_info_t)
#define A83T_PWM_REG_CTRL_IOCTL		_IOWR(A83T_IOCTL_MAGIC, 2, communication_info_t)
#define A83T_PWM_REG_PERIOD_IOCTL	_IOWR(A83T_IOCTL_MAGIC, 3, communication_info_t)
#define A83T_SWIM_READ_IOCTL		_IOR(A83T_IOCTL_MAGIC, 4, communication_info_t)
#define A83T_SWIM_WRITE_IOCTL		_IOW(A83T_IOCTL_MAGIC, 5, communication_info_t)




#define SWIM_CSR_ADDR	0x7F80
#define DM_CSR2_ADDR	0x7f99
#define FLASH_CR2_ADDR	0x505b
#define FLASH_NCR2_ADDR	0x505c
#define FLASH_PUKR_ADDR	0x5062

#define CSR_STALL_CPU	0x08
#define FLASH_INIT		0x56
#define UNLOCK_FLASH	0xae
#define STANDARD_BLOCK	0x01
#define BLOCK_PROGRAMING_EN		0xfe

#define ST8S_PAGE_CNT 128
#define ST8S_PAGE_SIZE 64

typedef struct APP_WITH_KERNEL{
	unsigned int pwm_ch_ctrl;
	unsigned int prescal;
	unsigned int entire_cys;
	unsigned int act_cys;

	unsigned int pulse_state;
	unsigned int pulse_width;

	unsigned int bit;
	
	unsigned int addr;
	unsigned char buf[128];
	unsigned int count;
}communication_info_t;


typedef enum {
    SWIM_OK,
    SWIM_FAIL,
    SWIM_TIMEOUT
}swim_handle_t;


typedef enum {
	DEFAULT,
    HIGH,
    LOW,
}swim_level_t;
	
typedef struct SWIM_PRIV_INFO{
	spinlock_t spinlock;
	unsigned long irqflags;

	struct timespec ts[2];
	
	void __iomem*	tmr_base_vaddr;
	void __iomem*	tmr_irq_en;
	void __iomem*	tmr_irq_sta;
	void __iomem*	tmr_0_ctrl;
	void __iomem*	tmr_0_intv;
	void __iomem*	tmr_0_current;

	void __iomem*  port_io_vaddr;
	void __iomem*  pd_cfg3_reg;	//bit[14:12] -> PD27_SELECT:  000:input  001:output
	void __iomem*  pd_data_reg; //input status / output val
	void __iomem*  pd_drv1_reg; //level0, 1, 2, 3
	void __iomem*  pd_pull1_reg;//00:pull-up/down disable, 01:pull-up, 10:pull-down, 11:reserved

	unsigned int	pd_cfg3_reg_tmp;
	unsigned int	pd_data_reg_tmp;
	unsigned int	pd_drv1_reg_tmp;
	unsigned int	pd_pull1_reg_tmp;

	void __iomem*  pwm_base_vaddr;
	void __iomem*  pwm_ch_ctrl;
	void __iomem*  pwm_ch0_period;
	void __iomem*  pwm_ch1_period;


	unsigned int return_line;
	void* private_date;
}swim_priv_t;

#if 1	//if000 start
#define reg_readl(addr)		(*((volatile unsigned long  *)(addr)))
#define reg_writel(v, addr)		(*((volatile unsigned long  *)(addr)) = (unsigned long)(v))
#else
#define reg_readl(addr)		readl(addr)
#define reg_writel(v, addr)		writel(v, addr)
#endif	//if000 end

#define hensen_debug(fmt, args...) do{printk(KERN_ALERT "%s(%d)."fmt, __func__, __LINE__, ##args);\
										printk(KERN_ALERT "\n");\
									}while(0)


#endif
