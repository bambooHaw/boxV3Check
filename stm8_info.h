#ifndef __STM8_INFO__
#define __STM8_INFO__

#include <linux/spinlock.h>

#define hensen_debug(fmt, args...) do{printk(KERN_ALERT "%s(%d)."fmt, __func__, __LINE__, ##args);\
										printk(KERN_ALERT "\n");\
									}while(0)


#define RST "PD26"
#define SWIM "PD27"
#define TIMER_BASE_ADDR 0x01C20C00
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
#define ENTIRE_CYS_CNT 0x40		//22*3, 22= 2+20 = 20+2
#define ACT_CYS_CNT0	0x3a	//20*3
#define ACT_CYS_CNT1	0x6		//2*3
#define PULSE_STATE_LOW 0x0
#define PULSE_STATE_HIGH 0x1



#define RST_CHK_TIMEOUT 65500

#define PORT_IO_BASEADDR 0x01c20800

#if 1
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

#else
//test for PD24 key sw3
#define PD_CFG3_REG_OFFSET 0X78
#define PD27_SELECT_POS 0
#define PD_DATA_REG_OFFSET 0X7C
#define PD27_DATA_POS   24
#define PD_DRV1_REG_OFFSET 0X84
#define PD27_DRV_POS 	16
#define PD_PULL1_REG_OFFSET 0X8c
#define PD27_PULL_POS	16

#endif

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

swim_pin_input(swim_priv_t* priv, unsigned char* pin_name){
	swim_pin_set(priv, pin_name, 0, 0, 0, 0);
	return ('7' == pin_name[3]) ? ((readl(priv->pd_data_reg) >> PD27_DATA_POS) & 0x1) : ((readl(priv->pd_data_reg) >> PD26_DATA_POS) & 0x1);

}

static inline int swim_pin_output(swim_priv_t * priv, unsigned char * pin_name, unsigned int val){
	return (LOW==val) ? swim_pin_set(priv, pin_name, 1, 2, 0, 0) : swim_pin_set(priv, pin_name, 1, 1, 3, 1);

}


static void __iomem* pwm_get_iomem(swim_priv_t* priv){

	void __iomem* vaddr = ioremap(PWM_BASE_ADDR, PAGE_ALIGN(PWM_CH1_PERIOD_OFFSET));
	if(!vaddr){
		printk(KERN_ERR "Error: ioremap for %s failed!\n", __func__);
		return NULL;
	}
	priv-> = vaddr;


}


static void swim_send_bit(swim_priv_t* priv, unsigned char bit){
    // way at low speed
    if(bit){
        swim_pin_output(priv, SWIM, LOW);
        a83t_ndelay(priv, 250); // 2*(1/8M) = 250ns
        swim_pin_output(priv, SWIM, HIGH);
        a83t_ndelay(priv, 2500); // 20*(1/8M) = 2500ns
    }
    else{
        swim_pin_output(priv, SWIM, LOW);
        a83t_ndelay(priv, 2500); // 2*(1/8M) = 2500ns
        swim_pin_output(priv, SWIM, HIGH);
        a83t_ndelay(priv, 250); // 20*(1/8M) = 250ns
    }
}


static char swim_rvc_bit(swim_priv_t* priv){
    unsigned int i;
    unsigned char cnt = 0, flag = 1;
    
    // way at low speed
#define ACK_CHK_TIMEOUT 1000
    for (i=0; i<ACK_CHK_TIMEOUT; i++){
        a83t_ndelay(priv, 125);
        if (swim_pin_input(priv, SWIM) == LOW){
            flag = 0;
            cnt++;
        }
        if(flag == 0 && swim_pin_input(priv, SWIM) == HIGH)return (cnt <= 8) ? 1 : 0;
    }
	
    return -1;
}

static void swim_send_ack(swim_priv_t* priv, unsigned char ack){
    a83t_ndelay(priv, 2750);
    swim_send_bit(priv, ack);
}

static char swim_rvc_ack(swim_priv_t* priv){
    return swim_rvc_bit(priv);
}



static swim_handle_t swim_send_unit(swim_priv_t* priv, unsigned char data, unsigned char len, unsigned i
#endif

typedef struct APP_WITH_KERNEL{
	unsigned int pwm_ch_ctrl;
	unsigned int prescal;
	unsigned int entire_cys;
	unsigned int act_cys;

	unsigned int pulse_state;
	unsigned int pulse_width;

	unsigned int bit;
}communication_info_t;


#define STM8_SWIM_DEVICE_NAME "swim"
#define SWIM_CMD_LEN                    3
#define SWIM_CMD_SRST                   0x00
#define SWIM_CMD_ROTF                   0x01
#define SWIM_CMD_WOTF                   0x02

#define SWIM_MAX_RESEND_CNT             20
#define A83T_IOCTL_MAGIC 'H'
#define SWIM_IOCTL_RESET 				1
#define A83T_PWM_DUTY_CYCLE_IOCTL		_IOWR(A83T_IOCTL_MAGIC, 0, communication_info_t)
#define A83T_PWM_PULSE_IOCTL		_IOWR(A83T_IOCTL_MAGIC, 1, communication_info_t)
#define A83T_PWM_REG_CTRL_IOCTL		_IOWR(A83T_IOCTL_MAGIC, 2, communication_info_t)
#define A83T_PWM_REG_PERIOD_IOCTL		_IOWR(A83T_IOCTL_MAGIC, 3, communication_info_t)



#define SWIM_CSR_ADDR 					0x00007F80

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
	
	void __iomem*	tmr_base_vaddr;
	void __iomem*	tmr_0_ctrl;
	void __iomem*	tmr_0_intv;
	void __iomem*	tmr_0_current;

	void __iomem*  port_io_vaddr;
	void __iomem*  pd_cfg3_reg;	//bit[14:12] -> PD27_SELECT:  000:input  001:output
	void __iomem*  pd_data_reg; //input status / output val
	void __iomem*  pd_drv1_reg; //level0, 1, 2, 3
	void __iomem*  pd_pull1_reg;//00:pull-up/down disable, 01:pull-up, 10:pull-down, 11:reserved

	void __iomem*  pwm_base_vaddr;
	void __iomem*  pwm_ch_ctrl;
	void __iomem*  pwm_ch0_period;
	void __iomem*  pwm_ch1_period;


	unsigned int return_line;
	void* private_date;
}swim_priv_t;


#define reg_readb(addr)		(*((volatile unsigned char  *)(addr)))
#define reg_readw(addr)		(*((volatile unsigned short *)(addr)))
#define reg_readl(addr)		(*((volatile unsigned long  *)(addr)))
#define reg_writeb(v, addr)		(*((volatile unsigned char  *)(addr)) = (unsigned char)(v))
#define reg_writew(v, addr)		(*((volatile unsigned short *)(addr)) = (unsigned short)(v))
#define reg_writel(v, addr)		(*((volatile unsigned long  *)(addr)) = (unsigned long)(v))

#endif



#if 0
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
static inline void swim_pin_low(void){

	//func ,0x1, output
	reg_writel((reg_readl(pp->pd_cfg3_reg) & (~(0x7 << PD27_SELECT_POS))) | (0x1 << PD27_SELECT_POS), pp->pd_cfg3_reg);
	//date, 0x0
	reg_writel((reg_readl(pp->pd_data_reg) & (~(0x1 << PD27_DATA_POS))) | (0x0 << PD27_DATA_POS), pp->pd_data_reg);
	//drv level 0x0
	reg_writel((reg_readl(pp->pd_drv1_reg) & (~(0X3 << PD27_DRV_POS))) | (0x0 << PD27_DRV_POS), pp->pd_drv1_reg);
	//pull 0x2 pull up
	reg_writel((reg_readl(pp->pd_pull1_reg) & (~(0X3 << PD27_PULL_POS))) | (0x2 << PD27_PULL_POS), pp->pd_pull1_reg);

}


static swim_handle_t swim_start_entry(swim_priv_t* priv){
	int i = 0, ret = SWIM_FAIL, flag = 1;
	unsigned char ch = 0;
	
	swim_pin_output(priv, SWIM, HIGH);
	swim_pin_output(priv, RST, HIGH);
	a83t_mdelay(priv, 50);
	
	/*1. To make the SWIM active, the SWIM pin must be forced low during a period of 16us*/
	swim_pin_output(priv, RST, LOW);
	a83t_mdelay(priv, 10);
	swim_pin_output(priv, SWIM, LOW);
	a83t_udelay(priv, 1000); 	//should be 16us, but 1000us could be better

	/*2. four pulses at 1 kHz followed by four pulses at 2 kHz.*/
    for (i=0; i<4; i++){
		swim_pin_output(priv, SWIM, HIGH);	//spend 1.25us
        a83t_udelay(priv, 500);
		swim_pin_output(priv, SWIM, LOW);
        a83t_udelay(priv, 500);
    }
    for (i=0; i<4; i++){
        swim_pin_output(priv, SWIM, HIGH);
        a83t_udelay(priv, 250);
        swim_pin_output(priv, SWIM, LOW);
        a83t_udelay(priv, 250);
    }
    swim_pin_output(priv, SWIM, HIGH);
   // 3. Swim is already in Active State
	swim_pin_input(priv, SWIM);

	//4. Delay for stm8's async ack, about 20us in this For Circle Func totally cost

	#if 1
#define RST_CHK_TIMEOUT 1000
	 for (i=0; i<RST_CHK_TIMEOUT; i++){
        if(swim_pin_input(priv, SWIM) == LOW)flag = 0;
        if((flag==0) && (swim_pin_input(priv, SWIM)==HIGH)){
			ret = SWIM_OK;
			break;
        }
    }
	 #endif
    hensen_debug("Send seq header done.\n");

	if(ret){
		swim_pin_output(priv, RST, HIGH);
		printk(KERN_ERR "Error: Wait ACK from stm8 timeout! %s(%d)\n", __func__, __LINE__);
		goto entry_err0;
	}

	a83t_mdelay(priv, 5);
	ret = swim_soft_reset(priv);
    if (ret){
        swim_pin_output(priv, RST, HIGH);
		printk(KERN_ERR "Error: Swim_soft_reset failed! %s(%d)\n", __func__, __LINE__);
		goto entry_err1;
    }

	hensen_debug("Swim_soft_reset done.\n");

	a83t_mdelay(priv, 30);
    ch = 0xA0;
    ret = swim_bus_write(priv, SWIM_CSR_ADDR, &ch, 1); 
    if (ret){
        swim_pin_output(priv, RST, HIGH);
		printk(KERN_ERR "Error: swim_write failed!\n");
        return ret;
    }
    a83t_mdelay(priv, 10);
	hensen_debug("Swim write 0xA0 done.\n");
	
	
    swim_pin_output(priv, RST, HIGH);
    a83t_mdelay(priv, 10);	
	hensen_debug("Start the option byte loading sequence done! Swim is ready for you!\n");
	
	return SWIM_OK;
	
entry_err1:

entry_err0:
	return SWIM_TIMEOUT;
}



#endif
