/*-
 * Copyright (c) 2018 Hensen <xiansheng929@163.com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    similar to the "NO WARRANTY" disclaimer below ("Disclaimer") and any
 *    redistribution must be conditioned upon including a substantially
 *    similar Disclaimer requirement for further binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF NONINFRINGEMENT, MERCHANTIBILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGES.
 *
 */
 /********************************************************
 *         A83T                 STM8
 * -------------------       ------------
 * |      (PD26) rst |>>--->>| rst      |
 * |     (PD27) gpio |<----->| swim     |
 * -------------------       ------------
 *******************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/time.h>
#include <mach/sys_config.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <linux/pinctrl/consumer.h>
#include <linux/delay.h>

#include <asm/io.h>
#include <linux/mm.h>
#include <linux/clk.h>

#include <asm/uaccess.h>
#include <asm/string.h>

#include "./stm8_info.h"

static swim_priv_t* pp = NULL;

static void __iomem* timer0_get_iomem(void){
	void __iomem* vaddr = ioremap(TIMER_BASE_ADDR, PAGE_ALIGN(TMR_0_INTV_OFF));
	if(!vaddr){
		printk(KERN_ERR "Error: ioremap for %s failed!\n", __func__);
		return NULL;
	}
	pp->tmr_base_vaddr = vaddr;
	pp->tmr_0_ctrl = vaddr + TMR_0_CTRL_OFF;
	pp->tmr_0_intv = vaddr + TMR_0_INTV_OFF;
	pp->tmr_0_current = vaddr + TMR_0_CURRENT_OFF;
	
	return vaddr;
}
static void timer0_free_iomem(void){

	if(pp->tmr_base_vaddr){
		iounmap(pp->tmr_base_vaddr);
		pp->tmr_base_vaddr = NULL;
	}
}

/* 
 * by Hensen 2018
 */
static void a83t_ndelay(unsigned int ns){

	unsigned int tcnt;

	// osc = 24MHz, min valid delay = 41.667ns, tcnt  = ns * (pclk / 100000) / 10000;
	if(ns<42)ns=42;
	tcnt  = ns*24/1000;

	reg_writel(tcnt, pp->tmr_0_intv);									//Set interval value
	reg_writel(0x84, pp->tmr_0_ctrl);									//Select single mode, 24MHz clock source, 1 pre-scale
	reg_writel(reg_readl(pp->tmr_0_ctrl)|(1<<1), pp->tmr_0_ctrl);		//Set Reload bit
	while((reg_readl(pp->tmr_0_ctrl)>>1) & 1);						//Waiting reload bit turns to 0
	reg_writel(reg_readl(pp->tmr_0_ctrl)|(1<<0), pp->tmr_0_ctrl); 		//Enable timer0

	while(reg_readl(pp->tmr_0_current));	//Waiting timer0 current value turns to zero
	reg_writel(0x0, pp->tmr_0_ctrl); 		//Disable timer0	
}

static void a83t_udelay(unsigned int us){
	while(us--)
		a83t_ndelay(1000);
}

static void a83t_mdelay(unsigned int ms){
	while(ms--)
		a83t_udelay(1000);
}

static void timer0_start_timing(void){
//	unsigned int cnt;

	// osc = 24MHz, min valid delay = 41.667ns, 

	reg_writel(0xefffffff, pp->tmr_0_intv);									//Set interval value
	reg_writel(0x84, pp->tmr_0_ctrl);									//Select single mode, 24MHz clock source, 1 pre-scale
	reg_writel(reg_readl(pp->tmr_0_ctrl)|(1<<1), pp->tmr_0_ctrl);		//Set Reload bit
	while((reg_readl(pp->tmr_0_ctrl)>>1) & 1);						//Waiting reload bit turns to 0
//	hensen_debug("timer0 start val: %d\n", reg_readl(pp->tmr_0_current));
	reg_writel(reg_readl(pp->tmr_0_ctrl)|(1<<0), pp->tmr_0_ctrl); 		//Enable timer0
}

static void timer0_get_time(void){
	register unsigned int cnt;

	reg_writel(reg_readl(pp->tmr_0_ctrl)& (~(1<<0)), pp->tmr_0_ctrl); 		//Disable timer0	
	cnt = reg_readl(pp->tmr_0_current);
	cnt = 0xefffffff - cnt;
	hensen_debug("Timing cnt: %d\n", cnt);
	
	cnt = (cnt * 1000) / 24;
	hensen_debug("Total time: %d(ns)\n", cnt);
}

static void __iomem* swim_get_iomem(void){
	void __iomem* vaddr = ioremap(PORT_IO_BASEADDR, PAGE_ALIGN(PD_PULL1_REG_OFFSET));
	if(!vaddr){
		printk(KERN_ERR "Error: ioremap for %s failed!\n", __func__);
		return NULL;
	}
	pp->port_io_vaddr = vaddr;
	pp->pd_cfg3_reg = vaddr + PD_CFG3_REG_OFFSET;
	pp->pd_data_reg = vaddr + PD_DATA_REG_OFFSET;
	pp->pd_drv1_reg = vaddr + PD_DRV1_REG_OFFSET;
	pp->pd_pull1_reg = vaddr + PD_PULL1_REG_OFFSET;

	return vaddr;
}
static void swim_free_iomem(void){
	if(pp->port_io_vaddr){
		iounmap(pp->port_io_vaddr);
		pp->port_io_vaddr = NULL;
	}
}


static void rst_set_as_output_high(void){
	
	reg_writel((reg_readl(pp->pd_cfg3_reg) & (~(0x7 << PD26_SELECT_POS))) | (0x1 << PD26_SELECT_POS), pp->pd_cfg3_reg);
	reg_writel((reg_readl(pp->pd_data_reg) & (~(0x1 << PD26_DATA_POS))) | (0x1 << PD26_DATA_POS), pp->pd_data_reg);
	reg_writel((reg_readl(pp->pd_drv1_reg) & (~(0X3 << PD26_DRV_POS))) | (0x3<< PD26_DRV_POS), pp->pd_drv1_reg);
	reg_writel((reg_readl(pp->pd_pull1_reg) & (~(0X3 << PD26_PULL_POS))) | (0x1 << PD26_PULL_POS), pp->pd_pull1_reg);	

	return;
}

static void rst_set_as_output_low(void){
	
	reg_writel((reg_readl(pp->pd_cfg3_reg) & (~(0x7 << PD26_SELECT_POS))) | (0x1 << PD26_SELECT_POS), pp->pd_cfg3_reg);
	reg_writel((reg_readl(pp->pd_data_reg) & (~(0x1 << PD26_DATA_POS))) | (0x0 << PD26_DATA_POS), pp->pd_data_reg);
	reg_writel((reg_readl(pp->pd_drv1_reg) & (~(0X3 << PD26_DRV_POS))) | (0x0<< PD26_DRV_POS), pp->pd_drv1_reg);
	reg_writel((reg_readl(pp->pd_pull1_reg) & (~(0X3 << PD26_PULL_POS))) | (0x2 << PD26_PULL_POS), pp->pd_pull1_reg);	

	return;
}

static void rst_set_as_input(void){
	
	reg_writel((reg_readl(pp->pd_cfg3_reg) & (~(0x7 << PD26_SELECT_POS))) | (0x0 << PD26_SELECT_POS), pp->pd_cfg3_reg);
	reg_writel((reg_readl(pp->pd_data_reg) & (~(0x1 << PD26_DATA_POS))) | (0x0 << PD26_DATA_POS), pp->pd_data_reg);
	reg_writel((reg_readl(pp->pd_drv1_reg) & (~(0X3 << PD26_DRV_POS))) | (0x0<< PD26_DRV_POS), pp->pd_drv1_reg);
	reg_writel((reg_readl(pp->pd_pull1_reg) & (~(0X3 << PD26_PULL_POS))) | (0x0 << PD26_PULL_POS), pp->pd_pull1_reg);	

	return;
}

static unsigned int rst_get_input_val(void){
	
	return ((reg_readl(pp->pd_data_reg)>>PD26_DATA_POS) & 0x1);
}

static void __iomem* pwm_get_iomem(void){

	void __iomem* vaddr = ioremap(PWM_BASE_ADDR, PAGE_ALIGN(PWM_CH1_PERIOD_OFFSET));
	if(!vaddr){
		printk(KERN_ERR "Error: ioremap for %s failed!\n", __func__);
		return NULL;
	}
	pp->pwm_base_vaddr = vaddr;
	pp->pwm_ch_ctrl = pp->pwm_base_vaddr + PWM_CH_CTRL_OFFSET;
	pp->pwm_ch0_period = pp->pwm_base_vaddr + PWM_CH0_PERIOD_OFFSET;
	pp->pwm_ch1_period = pp->pwm_base_vaddr + PWM_CH1_PERIOD_OFFSET;


	return vaddr;
}
static void pwm_free_iomem(void){
	if(pp->pwm_base_vaddr){
		iounmap(pp->pwm_base_vaddr);
		pp->pwm_base_vaddr = NULL;
	}
}

static inline void swim_set_as_pwm(void){
	
	reg_writel((reg_readl(pp->pd_cfg3_reg) & (~(0x7 << PD28_SELECT_POS))) | (0x2 << PD28_SELECT_POS), pp->pd_cfg3_reg);
	reg_writel((reg_readl(pp->pd_data_reg) & (~(0x1 << PD28_DATA_POS))) | (0x1 << PD28_DATA_POS), pp->pd_data_reg);
	reg_writel((reg_readl(pp->pd_drv1_reg) & (~(0X3 << PD28_DRV_POS))) | (0x3 << PD28_DRV_POS), pp->pd_drv1_reg);
	reg_writel((reg_readl(pp->pd_pull1_reg) & (~(0X3 << PD28_PULL_POS))) | (0x1 << PD28_PULL_POS), pp->pd_pull1_reg);

	return;
}
static inline void swim_set_as_input(void){
	
	reg_writel((reg_readl(pp->pd_cfg3_reg) & (~(0x7 << PD28_SELECT_POS))) | (0x0 << PD28_SELECT_POS), pp->pd_cfg3_reg);
	reg_writel((reg_readl(pp->pd_data_reg) & (~(0x1 << PD28_DATA_POS))) | (0x0 << PD28_DATA_POS), pp->pd_data_reg);
	reg_writel((reg_readl(pp->pd_drv1_reg) & (~(0X3 << PD28_DRV_POS))) | (0x0 << PD28_DRV_POS), pp->pd_drv1_reg);
	reg_writel((reg_readl(pp->pd_pull1_reg) & (~(0X3 << PD28_PULL_POS))) | (0x0 << PD28_PULL_POS), pp->pd_pull1_reg);

	return;
}

static unsigned int swim_get_input_val(void){
	return ((reg_readl(pp->pd_data_reg) >> PD28_DATA_POS) & 0x1);
}

static inline void swim_set_as_output_high(void){
	
	reg_writel((reg_readl(pp->pd_cfg3_reg) & (~(0x7 << PD28_SELECT_POS))) | (0x1 << PD28_SELECT_POS), pp->pd_cfg3_reg);
	reg_writel((reg_readl(pp->pd_data_reg) & (~(0x1 << PD28_DATA_POS))) | (0x1 << PD28_DATA_POS), pp->pd_data_reg);
	reg_writel((reg_readl(pp->pd_drv1_reg) & (~(0X3 << PD28_DRV_POS))) | (0x3 << PD28_DRV_POS), pp->pd_drv1_reg);
	reg_writel((reg_readl(pp->pd_pull1_reg) & (~(0X3 << PD28_PULL_POS))) | (0x1 << PD28_PULL_POS), pp->pd_pull1_reg);

	return;
}

static inline void swim_set_as_output_low(void){
	
	reg_writel((reg_readl(pp->pd_cfg3_reg) & (~(0x7 << PD28_SELECT_POS))) | (0x1 << PD28_SELECT_POS), pp->pd_cfg3_reg);
	reg_writel((reg_readl(pp->pd_data_reg) & (~(0x1 << PD28_DATA_POS))) | (0x0 << PD28_DATA_POS), pp->pd_data_reg);
	reg_writel((reg_readl(pp->pd_drv1_reg) & (~(0X3 << PD28_DRV_POS))) | (0x0 << PD28_DRV_POS), pp->pd_drv1_reg);
	reg_writel((reg_readl(pp->pd_pull1_reg) & (~(0X3 << PD28_PULL_POS))) | (0x2 << PD28_PULL_POS), pp->pd_pull1_reg);

	return;
}


/*
 * enable: 1:enable pwm_ch0, 0: disable pwm_ch0
 * prescal: This bts should be setting before the PWM Channel 0 clock gate on.
			 * 0000:/120	0001:/180	0010:/240	0011:/360
			 * 0100:/480	0101:/		0110:/		0111:/
			 * 1000:/12k	1001:/24k	1010:/36k	1011:/48k
			 * 1100:/72k	1101:/		1110:/		1111:/1
 * entire_cys: 0:1cycle, 1:2cycles, ..., n: n+1cycles(Number of the entire cycles in the PWM clock)
 * act_cys: //0:1cycle, 1:2cycles, ..., n: n+1cycles(Number of the act cycles in the PWM clock)
*/

static inline void pwm_reg_init(void){
	reg_writel(0x0, pp->pwm_ch_ctrl);	//disable pwm, clear pwm_ch_ctrl
	reg_writel(0x0, pp->pwm_ch0_period);	//clear pwm_ch0_period

}
static inline int pwm_reg_set(unsigned char* pin_name, unsigned int enable, unsigned int prescal, unsigned int entire_cys, unsigned int act_cys){

	if('8' == pin_name[3]){
		if(!enable){
			reg_writel(0x0<<PWM_CH0_EN_POS, pp->pwm_ch_ctrl);
			return 0;
		}else{
			
			reg_writel(0x0<<PWM_CH0_EN_POS, pp->pwm_ch_ctrl);
			reg_writel(((entire_cys&PWM_CH0_ENTIRE_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_CYS_POS) | ((act_cys&PWM_CH0_ENTIRE_ACT_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_ACT_CYS_POS), pp->pwm_ch0_period);
			reg_writel((0x0<<PWM0_BYPASS_POS) | (0x1<<PWM_CH0_ACT_STA_POS) | ((prescal&PWM_CH0_PRESCAL_BITFIELDS_MASK)<<PWM_CH0_PRESCAL_POS), pp->pwm_ch_ctrl);
			reg_writel(reg_readl(pp->pwm_ch_ctrl) | (0x1<<SCLK_CH0_GATING_POS) | (0x1<<PWM_CH0_EN_POS), pp->pwm_ch_ctrl);
			
		}
	}

	return 0;
}


/*
 * enable: 1:enable pwm_ch0, 0: disable pwm_ch0
 * prescal: This bts should be setting before the PWM Channel 0 clock gate on.
			 * 0000:/120	0001:/180	0010:/240	0011:/360
			 * 0100:/480	0101:/		0110:/		0111:/
			 * 1000:/12k	1001:/24k	1010:/36k	1011:/48k
			 * 1100:/72k	1101:/		1110:/		1111:/1
 * entire_cys: 0:1cycle, 1:2cycles, ..., n: n+1cycles(Number of the entire cycles in the PWM clock)
 * act_cys: //0:1cycle, 1:2cycles, ..., n: n+1cycles(Number of the act cycles in the PWM clock)
*/
static inline int pwm_reg_ctrl(unsigned int pwm_ch_ctrl){
	reg_writel(pwm_ch_ctrl, pp->pwm_ch_ctrl);
	return 0;
}


static inline int pwm_reg_period(unsigned int entire_cys, unsigned int act_cys){

	reg_writel(((entire_cys&PWM_CH0_ENTIRE_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_CYS_POS) | ((act_cys&PWM_CH0_ENTIRE_ACT_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_ACT_CYS_POS), pp->pwm_ch0_period);
	return 0;
}



static inline int pwm_reg_get(void){

	hensen_debug("pwm_ch_ctrl: %d\n", reg_readl(pp->pwm_ch_ctrl));
	hensen_debug("pwm_ch0_period: %d\n", reg_readl(pp->pwm_ch0_period));
	return 0;
}

/* because the prescale val is 0xf, so PWM CLK equals to 24MHz/1 as 24MHz
 * 
*/
static inline void pwm_waveform_set(unsigned int prescal, unsigned int entire_cys, unsigned int act_cys){

	pwm_reg_set(PWM, 1, prescal, entire_cys, act_cys);
}

/* After the pulse is finished, the bit(PWM_CH0_PUL_START_POS) will be cleared automatically.
 * pulse_state: 0:low_level, 1:high_level
 * pulse_width:
 * 
*/
static inline int pwm_pulse_set(unsigned int enable, unsigned int pulse_state, unsigned int  entire_cys, unsigned int pulse_width){
	unsigned int val = 0;
	
	if(!enable){
		reg_writel(0x0<<PWM_CH0_EN_POS, pp->pwm_ch_ctrl);
		return 0;
	}else{
		val = (PULSE_MODE<<PWM_CHANNEL0_MODE_POS) | (pulse_state<<PWM_CH0_ACT_STA_POS) | ((PWM_CH0_PRESCAL_VAL&PWM_CH0_PRESCAL_BITFIELDS_MASK)<<PWM_CH0_PRESCAL_POS);
		reg_writel(val, pp->pwm_ch_ctrl);
		reg_writel(((entire_cys&PWM_CH0_ENTIRE_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_CYS_POS) | ((pulse_width&PWM_CH0_ENTIRE_ACT_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_ACT_CYS_POS), pp->pwm_ch0_period);
		reg_writel(val | (0x1<<PWM_CH0_PUL_START_POS) | (0x1<<SCLK_CH0_GATING_POS) | (0x1<<PWM_CH0_EN_POS), pp->pwm_ch_ctrl);
	}

	return 0;
}

static inline int bit_pulse_set(register unsigned int val){

    //bit1: LOW  + HIGH  = [2*(1/8M)] +  [20*(1/8M)] = 250ns + 2500ns
    //bit0: HIGH  + LOW  = [20*(1/8M)] +  [2*(1/8M)] = 2500ns + 250ns

	if(1==val){
		val = (PULSE_MODE<<PWM_CHANNEL0_MODE_POS) | (PULSE_STATE_LOW<<PWM_CH0_ACT_STA_POS) | ((PWM_CH0_PRESCAL_VAL&PWM_CH0_PRESCAL_BITFIELDS_MASK)<<PWM_CH0_PRESCAL_POS);
		reg_writel(val, pp->pwm_ch_ctrl);
		reg_writel(((ENTIRE_CYS_CNT&PWM_CH0_ENTIRE_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_CYS_POS) | ((ACT_CYS_CNT1&PWM_CH0_ENTIRE_ACT_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_ACT_CYS_POS), pp->pwm_ch0_period);
		reg_writel(val | (0x1<<PWM_CH0_PUL_START_POS) | (0x1<<SCLK_CH0_GATING_POS) | (0x1<<PWM_CH0_EN_POS), pp->pwm_ch_ctrl);
	}else{
		val = (PULSE_MODE<<PWM_CHANNEL0_MODE_POS) | (PULSE_STATE_LOW<<PWM_CH0_ACT_STA_POS) | ((PWM_CH0_PRESCAL_VAL&PWM_CH0_PRESCAL_BITFIELDS_MASK)<<PWM_CH0_PRESCAL_POS);
		reg_writel(val, pp->pwm_ch_ctrl);
		reg_writel(((ENTIRE_CYS_CNT&PWM_CH0_ENTIRE_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_CYS_POS) | ((ACT_CYS_CNT0&PWM_CH0_ENTIRE_ACT_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_ACT_CYS_POS), pp->pwm_ch0_period);
		reg_writel(val | (0x1<<PWM_CH0_PUL_START_POS) | (0x1<<SCLK_CH0_GATING_POS) | (0x1<<PWM_CH0_EN_POS), pp->pwm_ch_ctrl);
	}
	return 0;
}

static inline void pwm_pulse_low_250ns(void){
    // way at low speed
    
	reg_writel(0x0, pp->pwm_ch_ctrl);	//disable pwm, clear pwm_ch_ctrl
    reg_writel(((ACT_CYS_CNT1&PWM_CH0_ENTIRE_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_CYS_POS) | ((ACT_CYS_CNT1&PWM_CH0_ENTIRE_ACT_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_ACT_CYS_POS), pp->pwm_ch0_period);
   
	reg_writel((PULSE_MODE<<PWM_CHANNEL0_MODE_POS) | (PULSE_STATE_LOW<<PWM_CH0_ACT_STA_POS) | ((PWM_CH0_PRESCAL_VAL&PWM_CH0_PRESCAL_BITFIELDS_MASK)<<PWM_CH0_PRESCAL_POS) | (0x1<<PWM_CH0_PUL_START_POS) | (0x1<<SCLK_CH0_GATING_POS) | (0x1<<PWM_CH0_EN_POS), pp->pwm_ch_ctrl);
	while((reg_readl(pp->pwm_ch_ctrl)>>PWM_CH0_PUL_START_POS)&0x1);	//	wait for PWM_CH0_PUL_START_POS bit cleared automatically(After the pulse is finished.).
}

static inline void pwm_pulse_low_2500ns(void){
    // way at low speed
	reg_writel(0x0, pp->pwm_ch_ctrl);	//disable pwm, clear pwm_ch_ctrl
	reg_writel(((ACT_CYS_CNT0&PWM_CH0_ENTIRE_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_CYS_POS) | ((ACT_CYS_CNT0&PWM_CH0_ENTIRE_ACT_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_ACT_CYS_POS), pp->pwm_ch0_period);

	reg_writel((PULSE_MODE<<PWM_CHANNEL0_MODE_POS) | (PULSE_STATE_LOW<<PWM_CH0_ACT_STA_POS) | ((PWM_CH0_PRESCAL_VAL&PWM_CH0_PRESCAL_BITFIELDS_MASK)<<PWM_CH0_PRESCAL_POS) | (0x1<<PWM_CH0_PUL_START_POS) | (0x1<<SCLK_CH0_GATING_POS) | (0x1<<PWM_CH0_EN_POS), pp->pwm_ch_ctrl);
	while((reg_readl(pp->pwm_ch_ctrl)>>PWM_CH0_PUL_START_POS)&0x1);	//	wait for PWM_CH0_PUL_START_POS bit cleared automatically(After the pulse is finished.).
}

static inline void pwm_pulse_clear(void){
    // way at low speed
	reg_writel(0x0, pp->pwm_ch_ctrl);	//disable pwm, clear pwm_ch_ctrl
	reg_writel(0x0, pp->pwm_ch0_period);

	reg_writel((PULSE_MODE<<PWM_CHANNEL0_MODE_POS) | (PULSE_STATE_LOW<<PWM_CH0_ACT_STA_POS) | ((PWM_CH0_PRESCAL_VAL&PWM_CH0_PRESCAL_BITFIELDS_MASK)<<PWM_CH0_PRESCAL_POS) | (0x1<<PWM_CH0_PUL_START_POS) | (0x1<<SCLK_CH0_GATING_POS) | (0x1<<PWM_CH0_EN_POS), pp->pwm_ch_ctrl);
	while((reg_readl(pp->pwm_ch_ctrl)>>PWM_CH0_PUL_START_POS)&0x1);	//	wait for PWM_CH0_PUL_START_POS bit cleared automatically(After the pulse is finished.).
}

static inline void pwm_send_disable(void){
	reg_writel((reg_readl(pp->pwm_ch_ctrl) & (~(0x1<<PWM_CH0_EN_POS))), pp->pwm_ch_ctrl);
}


static inline void send_swim_bit(unsigned int bit){
	// way at low speed
	if(0==bit){//0
		pwm_pulse_low_2500ns();	//2500ns
		a83t_ndelay(250);	//250ns
	}else{//1
		pwm_pulse_low_250ns();	//250ns	
		a83t_ndelay(2500);	//2500ns
	}
	
	return;
}

static char rvc_swim_bit(void){
    unsigned int i;
    unsigned char cnt = 0, flag = 1;
    
    // way at low speed
#define ACK_CHK_TIMEOUT 100
    for (i=0; i<ACK_CHK_TIMEOUT; i++){
        //a83t_ndelay(125);
        if (LOW == swim_get_input_val()){
            flag = 0;
            cnt++;
        }
        if((flag == 0) && (HIGH == swim_get_input_val()))return (cnt <= 8) ? 1 : 0;
    }
	
    return -1;
}

static void swim_send_ack(unsigned char ack){
    a83t_ndelay(2750);
    send_swim_bit(ack);
}

static char rvc_swim_ack(void){
    return rvc_swim_bit();
}



static swim_handle_t swim_send_unit(unsigned char data, unsigned char len, unsigned int retry){
		signed char i;
		unsigned char p, m;
		char ack = 0;
	
		a83t_ndelay(2750);
		
		//spin_lock_irq(&pp->spinlock);
		do {
			send_swim_bit(0);
			p = 0;
			for (i=0; i<len; i++){
				m = (data >> i) & 1;
				send_swim_bit(m);
				p += m;
			}
			// parity bit
			send_swim_bit(p & 1);
			reg_writel((reg_readl(pp->pd_cfg3_reg) & (~(0x7 << PD28_SELECT_POS))) | (0x0 << PD28_SELECT_POS), pp->pd_cfg3_reg);	//set swim as input
			ack = rvc_swim_bit();
			reg_writel((reg_readl(pp->pd_cfg3_reg) & (~(0x7 << PD28_SELECT_POS))) | (0x2 << PD28_SELECT_POS), pp->pd_cfg3_reg);	//set swim as pwm func
			if (ack == -1){
				//spin_unlock_irq(&pp->spinlock);
				hensen_debug("Error: ACK failed!\n");
				return SWIM_TIMEOUT;
			}
		} while (!ack && retry--);
		//spin_unlock_irq(&pp->spinlock);
		
		return ack ? SWIM_OK : SWIM_FAIL;
	}



static swim_handle_t swim_soft_reset(void){
    return swim_send_unit(SWIM_CMD_SRST, SWIM_CMD_LEN, SWIM_MAX_RESEND_CNT);
}


static swim_handle_t swim_rcv_unit(unsigned char *data, unsigned char len)
{
    char i;
    unsigned char s = 0, m = 0, p = 0, cp = 0;
    char ack = 0;

    for (i=0; i<len+2; i++)
    {
        ack = rvc_swim_bit();
        if (ack == -1)
        {
            return SWIM_TIMEOUT;
        }
        if (i == 0)
        {
            s = ack;
        }
        else if (i == len + 1)
        {
            p = ack;
        }
        else
        {
            m <<= 1;
            m |= ack;
            cp += ack;
        }
    }

    if (s == 1 && p == (cp & 1))
    {
        ack = 1;
    }

    swim_send_ack(ack);

    return ack ? SWIM_OK : SWIM_FAIL;
}

static swim_handle_t swim_bus_read(unsigned int addr, unsigned char *buf, unsigned int size)
{
    unsigned char cur_len, i;
    unsigned int cur_addr = addr;
    swim_handle_t ret = SWIM_OK;
    unsigned char first = 0;

    if (!buf)
    {
        return SWIM_FAIL;
    }
    hensen_debug("swim_bus_read start addr=0x%X, buf[0]=%#x, size=0x%X\n", addr, buf[0], size);
    while (size)
    {
        cur_len = (size > 255) ? 255 : size;

        ret = swim_send_unit(SWIM_CMD_ROTF, SWIM_CMD_LEN, SWIM_MAX_RESEND_CNT);
        if (ret)
        {
            pp->return_line = __LINE__;
            break;
        }
        ret = swim_send_unit(cur_len, 8, 0);
        if (ret)
        {
            pp->return_line = __LINE__;
            break;
        }
        ret = swim_send_unit((cur_addr >> 16) & 0xFF, 8, 0);
        if (ret)
        {
            pp->return_line = __LINE__;
            break;
        }
        ret = swim_send_unit((cur_addr >> 8) & 0xFF, 8, 0);
        if (ret)
        {
            pp->return_line = __LINE__;
            break;
        }
        ret = swim_send_unit((cur_addr >> 0) & 0xFF, 8, 0);
        if (ret)
        {
            pp->return_line = __LINE__;
            break;
        }
        for (i = 0; i < cur_len; i++)
        {
            ret = swim_rcv_unit(buf++, 8);
            if (ret)
            {
                pp->return_line = __LINE__;
				hensen_debug("Error receive\n");
                break;
            }

            if (first == 0)
                first = *(buf - 1);
        }
        if (ret)
        {
            break;
        }

        cur_addr += cur_len;
        size -= cur_len;
    }
    hensen_debug("swim_bus_read end addr=0x%X, data=0x%X, ret=%d\n", addr, first, ret);
    return ret;
}


/* The introduce of  swim_bus_write
 * WOTF: write on-the-fly
 * 	1 command followed by the number of bytes to be written followed by the address on three bytes.
 
 	|WOTF| + |N| + |@E| + |@H| + |@L| + |D[@]| + |D[@+N]|
 	
	Parameters:
	N:		  	The 8 bits are the number of bytes to write (from 1 to 255)
	@E/H/L: 	This is the 24-bit address to be accessed.
	D[...]:  	These are the data bytes to write in the memory space
				If a byte D [i] has not been written when the following byte D [i+1] arrives, D [i+1] will be
				followed by a NACK. In this case the host must send D [i+1] again until it is acknowledged.
				For the last byte, if it is not yet written when a new command occurs, the new command will
				receive a NACK and will not be taken into account.

	Notes:		If the SWIM_DM bit is cleared, the WOTF can only be done on the SWIM internal registers.
*/
static swim_handle_t swim_bus_write(unsigned int addr, unsigned char *buf, unsigned int size){
	unsigned char cur_len, i;
	unsigned int cur_addr = addr;
	swim_handle_t ret = SWIM_OK;

	if (!buf){
		return SWIM_FAIL;
	}

		hensen_debug("(start) addr=0x%X, buf[0]=%#x, size=0x%X\n", addr, buf[0], size);
	while (size){
		cur_len = (size > 255) ? 255 : size;

		ret = swim_send_unit(SWIM_CMD_WOTF, SWIM_CMD_LEN, SWIM_MAX_RESEND_CNT);
		if (ret)
		{
			pp->return_line = __LINE__;
			break;
		}
		ret = swim_send_unit(cur_len, 8, 0);
		if (ret){
			pp->return_line = __LINE__;
			break;
		}
		ret = swim_send_unit((cur_addr >> 16) & 0xFF, 8, 0);
		if (ret){
			pp->return_line = __LINE__;
			break;
		}
		ret = swim_send_unit((cur_addr >> 8) & 0xFF, 8, 0);
		if (ret){
			pp->return_line = __LINE__;
			break;
		}
		ret = swim_send_unit((cur_addr >> 0) & 0xFF, 8, 0);
		if (ret){
			pp->return_line = __LINE__;
			break;
		}
		for (i = 0; i < cur_len; i++){
			ret = swim_send_unit(*buf++, 8, SWIM_MAX_RESEND_CNT);
			if (ret){
				pp->return_line = __LINE__;
				hensen_debug("Error: send\n");
				break;
			}
		}
		if (ret){
			break;
		}

		cur_addr += cur_len;
		size -= cur_len;
	}

	return ret;
}

static void pd27_set_as_input(void){
	reg_writel((reg_readl(pp->pd_cfg3_reg) & (~(0x7 << PD27_SELECT_POS))) | (0x0 << PD27_SELECT_POS), pp->pd_cfg3_reg);
	reg_writel((reg_readl(pp->pd_data_reg) & (~(0x1 << PD27_DATA_POS))) | (0x0 << PD27_DATA_POS), pp->pd_data_reg);
	reg_writel((reg_readl(pp->pd_drv1_reg) & (~(0X3 << PD27_DRV_POS))) | (0x0 << PD27_DRV_POS), pp->pd_drv1_reg);
	reg_writel((reg_readl(pp->pd_pull1_reg) & (~(0X3 << PD27_PULL_POS))) | (0x0 << PD27_PULL_POS), pp->pd_pull1_reg);
	return;
}

static swim_handle_t swim_start_entry_new(void){
	int i = 0, ret = SWIM_FAIL;
	unsigned char ch = 0;

	pd27_set_as_input();
	swim_set_as_output_high();
	//rst_set_as_output_high();
	a83t_mdelay(50);
	
	/*1. To make the SWIM active, the SWIM pin must be forced low during a period of 16us*/
	//rst_set_as_output_low();
	a83t_mdelay(10);
	swim_set_as_output_low();
	a83t_udelay(1000); 	//should be 16us, but 1000us could be better

	
	/*2. four pulses at 1 kHz followed by four pulses at 2 kHz.*/
    for (i=0; i<4; i++){
		swim_set_as_output_high();//cost 200us
        a83t_udelay(300);
		swim_set_as_output_low();
        a83t_udelay(300);
    }
    for (i=0; i<4; i++){
		swim_set_as_output_high(); //cost 100us
        a83t_udelay(150);
		swim_set_as_output_low();
        a83t_udelay(150);
    }
	swim_set_as_output_high();
   // 3. Swim is already in Active State
	swim_set_as_input();

	

	//4. Delay for stm8's async ack, about 20us in this For Circle Func totally cost
	//about 16us low level for swim device ack to MCU
//	timer0_start_timing();
	 for(i=0; i<RST_CHK_TIMEOUT; i++){
        if(!swim_get_input_val()){
			ret = SWIM_OK;
			//timer0_get_time();
			break;
        }
    }
	 if(ret){
		hensen_debug("swim ack timeout!\n");
	 }else{	 
		ret = SWIM_FAIL;
		//timer0_start_timing();
		 for(i=0; i<RST_CHK_TIMEOUT; i++){
	        if(swim_get_input_val()){
				ret = SWIM_OK;
		//		timer0_get_time();
				break;
	        }
	    }
	}
		
    //hensen_debug("ret:%d. Send seq header done.\n", ret);

	if(ret){
	//	rst_set_as_output_high();
		printk(KERN_ERR "Error: Wait ACK from stm8 timeout! %s(%d)\n", __func__, __LINE__);
		goto entry_err0;
	}

	
	swim_set_as_pwm();
	a83t_mdelay(1);


#if 1
	ret = swim_soft_reset();
//		pwm_send_disable();
	//swim_set_as_input();
    if (ret){
        rst_set_as_output_high();
		rst_set_as_input();
		printk(KERN_ERR "Error: Swim_soft_reset failed! %s(%d)\n", __func__, __LINE__);
		goto entry_err1;
    }

//	hensen_debug("Swim_soft_reset done.\n");
	a83t_mdelay(5);
#endif

    ch = 0xA0;
    ret = swim_bus_write(SWIM_CSR_ADDR, &ch, 1); 
    if (ret){
    //    rst_set_as_output_high();
		printk(KERN_ERR "Error: swim_write failed!\n");
        return ret;
    }
    a83t_mdelay(10);
	hensen_debug("Swim write 0xA0 done.\n");
	
//	rst_set_as_output_high();
	swim_set_as_output_high();
    a83t_mdelay(10);	
	hensen_debug("Start the option byte loading sequence done! Swim is ready for you!\n");
	
	return SWIM_OK;
	
entry_err1:

entry_err0:
	return SWIM_TIMEOUT;
}



static int stm8_swim_open(struct inode* inodp, struct file* filp){
	int i = 0;
	
	//1. Alloc private date, and add private_date into file
	swim_priv_t* priv = kzalloc(sizeof(swim_priv_t), GFP_KERNEL);
	if(!priv){
		printk(KERN_ERR "Kzalloc for swim_priv failed!\n");
		return -ENOMEM;
	}
	filp->private_data = priv;
	pp = priv;
	spin_lock_init(&pp->spinlock);	

	
	//2. get timer0 iomap
	if(!timer0_get_iomem())return -ENOMEM;
	
	//3. get swim_pin iomap
	if(!swim_get_iomem())return -ENOMEM;
	if(!pwm_get_iomem())return -ENOMEM;

	

#if 1 //test for timer0 and pwm pulse

	swim_set_as_pwm();
	pwm_reg_init();

	while(1){
		a83t_mdelay(10);
		send_swim_bit(0);
		send_swim_bit(0);
		send_swim_bit(1);
		send_swim_bit(0);
		send_swim_bit(1);
		pwm_pulse_clear();
		//swim_send_unit(SWIM_CMD_WOTF, SWIM_CMD_LEN, 0);
	}
	
	return 0;

	
#endif

	
	swim_start_entry_new();

	
#if 0
	//test007
	hensen_debug("Set as low spend:");
	timer0_start_timing();
	swim_set_as_output_low();
	timer0_get_time();

	hensen_debug("Set as high spend:");

	timer0_start_timing();
	swim_set_as_output_high();
	timer0_get_time();

	hensen_debug("Get time spend:");

	swim_set_as_input();
	timer0_start_timing();
	swim_get_input_val();
	timer0_get_time();

	swim_set_as_output_low();
	a83t_mdelay(10);
	swim_set_as_pwm();
	while(1){
		send_swim_bit(1);
	}
	//test007 end
#endif


	hensen_debug();
	return 0;
}
static int stm8_swim_release(struct inode* inodp, struct file* filp){
	swim_priv_t * priv = filp->private_data;

	swim_free_iomem();
	pwm_free_iomem();

	timer0_free_iomem();
	kfree(priv);
	priv = NULL;
	pp = NULL;
	
	hensen_debug();
	return 0;
}

static ssize_t a83t_swim_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    swim_handle_t ret;
    unsigned int addr = *ppos;
    unsigned char *kbuf = NULL;	
    
    kbuf = kmalloc(count, GFP_KERNEL);
    if (!kbuf)
    {
        printk("kmalloc %d bytes fail!\n", count);
        return 0;
    }

    hensen_debug("swim read addr = 0x%x, size = 0x%x\n", addr, count);

    local_irq_disable();
    ret = swim_bus_read(addr, kbuf, count);
    if (ret)
    {
        printk("resd fail! ret = %d, priv->return_line = %d\n", ret, pp->return_line);
    }
    local_irq_enable();

    if (copy_to_user(buf, kbuf, count))
    {
        printk("copy_to_user fail!\n");
        return 0;
    }

    kfree(kbuf);

    return count;
}

static ssize_t a83t_swim_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    swim_handle_t ret;
    unsigned int addr = *ppos;
    unsigned char *kbuf = NULL;
    
    kbuf = kmalloc(count, GFP_KERNEL);
    if (!kbuf)
    {
        printk("kmalloc %d bytes fail!\n", count);
        return 0;
    }

    if (copy_from_user(kbuf, buf, count))
    {
        printk("copy_from_user fail!\n");
        return 0;
    }

    printk("swim write addr = 0x%x, size = 0x%x\n", addr, count);

    local_irq_disable();
    ret = swim_bus_write(addr, kbuf, count);
    if (ret)
    {
        printk("write fail! ret = %d, priv->return_line = %d\n", ret, pp->return_line);
    }
    local_irq_enable();

    kfree(kbuf);

    return count;
}

static loff_t a83t_swim_seek(struct file *filp, loff_t offset, int orig)
{
    loff_t ret;

    switch (orig) 
    {
        case SEEK_CUR:
            offset += filp->f_pos;
        case SEEK_SET:
            filp->f_pos = offset;
            ret = filp->f_pos;
            break;
        default:
            ret = -EINVAL;
    }

    return ret;
}

static long a83t_swim_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    swim_handle_t ret = 0;

    switch (cmd)
    {
        case SWIM_IOCTL_RESET:
            local_irq_disable();
            ret = swim_soft_reset();
            if (ret)
            {
                printk("reset fail! ret = %d, return_line = %d\n", ret, pp->return_line);
            }
            local_irq_enable();
            break;
        default:
            break;
    }

    return ret;
}



static long hensen_pwm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
    swim_handle_t ret = 0;
	communication_info_t* info = (communication_info_t*)arg;
	
	if(A83T_IOCTL_MAGIC != _IOC_TYPE(cmd)){
		printk(KERN_ERR "unlocked_ioctl error!\n");
		return -EINVAL;
	}
	
    switch(_IOC_NR(cmd)){
        case 0:
			hensen_debug("prescal:%#x\n", info->prescal);
			pwm_waveform_set(info->prescal, info->entire_cys, info->act_cys);
			pwm_reg_get();
            break;
		case 1:
			pwm_pulse_set(1, info->pulse_state, info->entire_cys, info->pulse_width);
			pwm_reg_get();
			break;
		case 2:
			pwm_reg_ctrl(info->pwm_ch_ctrl);
			pwm_reg_get();
			break;
		case 3:
			pwm_reg_period(info->entire_cys, info->act_cys);
			pwm_reg_get();
			break;
		case 4:
			info->count = 1;
			memset(info->buf, 0, sizeof(unsigned char)*info->count);
			if(swim_bus_read(info->addr, info->buf, info->count)){
				hensen_debug("ERROR: read!\n");
			}
			break;
		case 5:
			info->count = 1;
			if(swim_bus_write(info->addr, info->buf, info->count)){
				hensen_debug("ERROR: write!\n");
			}
			break;
        default:
        	hensen_debug("Error: unlocked_ioctl!\n");
            break;
    }

    return ret;
}


static struct file_operations stm8_swim_fops = {
	.open = stm8_swim_open,
	.release = stm8_swim_release,
    .write          = a83t_swim_write,
    .read           = a83t_swim_read,
    .llseek         = a83t_swim_seek,
//    .unlocked_ioctl = a83t_swim_ioctl,
    .unlocked_ioctl = hensen_pwm_ioctl,
};
	
static struct miscdevice stm8_swim_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = STM8_SWIM_DEVICE_NAME,
	.fops = &stm8_swim_fops,
	
};
static int __init stm8_swim_module_init(void){

	
	misc_register(&stm8_swim_misc);

	hensen_debug();
	return 0;
}
static void __exit stm8_swim_module_exit(void){

	misc_deregister(&stm8_swim_misc);
	hensen_debug();
}

module_init(stm8_swim_module_init);
module_exit(stm8_swim_module_exit);
MODULE_AUTHOR("Hensen <xiansheng929@163.com>");
MODULE_DESCRIPTION("STM8 swim driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("pinctrl subsystem");


#if 0

static void pd27_set_as_output_high(void){
	
	reg_writel((reg_readl(pp->pd_cfg3_reg) & (~(0x7 << PD27_SELECT_POS))) | (0x1 << PD27_SELECT_POS), pp->pd_cfg3_reg);
	reg_writel((reg_readl(pp->pd_data_reg) & (~(0x1 << PD27_DATA_POS))) | (0x1 << PD27_DATA_POS), pp->pd_data_reg);
	reg_writel((reg_readl(pp->pd_drv1_reg) & (~(0X3 << PD27_DRV_POS))) | (0x3 << PD27_DRV_POS), pp->pd_drv1_reg);
	reg_writel((reg_readl(pp->pd_pull1_reg) & (~(0X3 << PD27_PULL_POS))) | (0x1 << PD27_PULL_POS), pp->pd_pull1_reg);
	return;
}
static void pd27_set_as_output_low(void){
	
	reg_writel((reg_readl(pp->pd_cfg3_reg) & (~(0x7 << PD27_SELECT_POS))) | (0x1 << PD27_SELECT_POS), pp->pd_cfg3_reg);
	reg_writel((reg_readl(pp->pd_data_reg) & (~(0x1 << PD27_DATA_POS))) | (0x0 << PD27_DATA_POS), pp->pd_data_reg);
	reg_writel((reg_readl(pp->pd_drv1_reg) & (~(0X3 << PD27_DRV_POS))) | (0x0 << PD27_DRV_POS), pp->pd_drv1_reg);
	reg_writel((reg_readl(pp->pd_pull1_reg) & (~(0X3 << PD27_PULL_POS))) | (0x2 << PD27_PULL_POS), pp->pd_pull1_reg);
	return;
}

static void pd27_set_as_input(void){
	reg_writel((reg_readl(pp->pd_cfg3_reg) & (~(0x7 << PD27_SELECT_POS))) | (0x0 << PD27_SELECT_POS), pp->pd_cfg3_reg);
	reg_writel((reg_readl(pp->pd_data_reg) & (~(0x1 << PD27_DATA_POS))) | (0x0 << PD27_DATA_POS), pp->pd_data_reg);
	reg_writel((reg_readl(pp->pd_drv1_reg) & (~(0X3 << PD27_DRV_POS))) | (0x0 << PD27_DRV_POS), pp->pd_drv1_reg);
	reg_writel((reg_readl(pp->pd_pull1_reg) & (~(0X3 << PD27_PULL_POS))) | (0x0 << PD27_PULL_POS), pp->pd_pull1_reg);
	return;
}

static unsigned int pd27_get_input_val(void){
	
	return ((reg_readl(pp->pd_data_reg)>>PD27_DATA_POS) & 0x1);
}

#endif
