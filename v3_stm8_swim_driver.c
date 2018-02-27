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

#include "./stm8_info.h"


static void __iomem* timer0_get_iomem(swim_priv_t* priv){
	void __iomem* vaddr = ioremap(TIMER_BASE_ADDR, PAGE_ALIGN(TMR_0_INTV_OFF));
	if(!vaddr){
		printk(KERN_ERR "Error: ioremap for %s failed!\n", __func__);
		return NULL;
	}
	priv->tmr_base_vaddr = vaddr;
	priv->tmr_0_ctrl = vaddr + TMR_0_CTRL_OFF;
	priv->tmr_0_intv = vaddr + TMR_0_INTV_OFF;
	
	return vaddr;
}
static void timer0_free_iomem(swim_priv_t* priv){

	if(priv->tmr_base_vaddr){
		iounmap(priv->tmr_base_vaddr);
		priv->tmr_base_vaddr = NULL;
	}
}

/* 
 * by Hensen 2018
 */
static inline void a83t_ndelay(swim_priv_t * priv, unsigned int ns){

	unsigned long tcnt;

	// osc = 24MHz, min valid delay = 41.667ns, tcnt  = ns * (pclk / 100000) / 10000;
	if(ns<42)ns=42;
	tcnt  = ns*24/1000;

	writel(tcnt, priv->tmr_0_intv);									//Set interval value
	writel(0x84, priv->tmr_0_ctrl);									//Select single mode, 24MHz clock source, 1 pre-scale
	writel(readl(priv->tmr_0_ctrl)|(1<<1), priv->tmr_0_ctrl);		//Set Reload bit
	while((readl(priv->tmr_0_ctrl)>>1) & 1);						//Waiting reload bit turns to 0
	writel(readl(priv->tmr_0_ctrl)|(1<<0), priv->tmr_0_ctrl); 		//Enable timer0
}

static inline void a83t_udelay(swim_priv_t* priv, unsigned int us){
	while(us--)
		a83t_ndelay(priv, 1000);
}

static void a83t_mdelay(swim_priv_t* priv, unsigned int ms){
	while(ms--)
		a83t_udelay(priv, 1000);
}


static void __iomem* swim_get_iomem(swim_priv_t* priv){
	void __iomem* vaddr = ioremap(PORT_IO_BASEADDR, PAGE_ALIGN(PD_PULL1_REG_OFFSET));
	if(!vaddr){
		printk(KERN_ERR "Error: ioremap for %s failed!\n", __func__);
		return NULL;
	}
	priv->port_io_vaddr = vaddr;
	priv->pd_cfg3_reg = vaddr + PD_CFG3_REG_OFFSET;
	priv->pd_data_reg = vaddr + PD_DATA_REG_OFFSET;
	priv->pd_drv1_reg = vaddr + PD_DRV1_REG_OFFSET;
	priv->pd_pull1_reg = vaddr + PD_PULL1_REG_OFFSET;

	return vaddr;
}
static void swim_free_iomem(swim_priv_t* priv){
	if(priv->port_io_vaddr){
		iounmap(priv->port_io_vaddr);
		priv->port_io_vaddr = NULL;
	}
}

/*  by Hensen 2018.  //TO save time so no val check, please care about for that.
	name: port name, eg. PG6, PG7, PG8...
	func: multi sel val: 0 - input, 1 - output... 
	pull:  pull val: 0 - pull up/down disable, 1 - pull up... , 2-pull down
	drv: driver level val: 0 - level 0, 1 - level 1...
	data: data val: 0 - low, 1 - high, only vaild when mul_sel is input/output
*/
static inline int swim_pin_set(swim_priv_t* priv, unsigned char* pin_name, unsigned int func, unsigned int pull, unsigned int drv, unsigned int data){

	spin_lock_irqsave(&priv->spinlock, priv->irqflags);
	
	//8->PWM, 7 -> SWIM, 6 -> RST, strcmp spend 200ns, not recommend
	
	if('8' == pin_name[3]){
		reg_writel((reg_readl(priv->pd_cfg3_reg) & (~(0x7 << PD28_SELECT_POS))) | (func << PD28_SELECT_POS), priv->pd_cfg3_reg);
		reg_writel((reg_readl(priv->pd_data_reg) & (~(0x1 << PD28_DATA_POS))) | (data << PD28_DATA_POS), priv->pd_data_reg);
		reg_writel((reg_readl(priv->pd_drv1_reg) & (~(0X3 << PD28_DRV_POS))) | (drv << PD28_DRV_POS), priv->pd_drv1_reg);
		reg_writel((reg_readl(priv->pd_pull1_reg) & (~(0X3 << PD28_PULL_POS))) | (pull << PD28_PULL_POS), priv->pd_pull1_reg);

		spin_unlock_irqrestore(&priv->spinlock, priv->irqflags);
		return 0;
	}
	if('7' == pin_name[3]){
		reg_writel((reg_readl(priv->pd_cfg3_reg) & (~(0x7 << PD27_SELECT_POS))) | (func << PD27_SELECT_POS), priv->pd_cfg3_reg);
		reg_writel((reg_readl(priv->pd_data_reg) & (~(0x1 << PD27_DATA_POS))) | (data << PD27_DATA_POS), priv->pd_data_reg);
		reg_writel((reg_readl(priv->pd_drv1_reg) & (~(0X3 << PD27_DRV_POS))) | (drv << PD27_DRV_POS), priv->pd_drv1_reg);
		reg_writel((reg_readl(priv->pd_pull1_reg) & (~(0X3 << PD27_PULL_POS))) | (pull << PD27_PULL_POS), priv->pd_pull1_reg);

		spin_unlock_irqrestore(&priv->spinlock, priv->irqflags);
		return 0;
	}
	
	if('6' == pin_name[3]){
		reg_writel((reg_readl(priv->pd_cfg3_reg) & (~(0x7 << PD26_SELECT_POS))) | (func << PD26_SELECT_POS), priv->pd_cfg3_reg);
		reg_writel((reg_readl(priv->pd_data_reg) & (~(0x1 << PD26_DATA_POS))) | (data << PD26_DATA_POS), priv->pd_data_reg);
		reg_writel((reg_readl(priv->pd_drv1_reg) & (~(0X3 << PD26_DRV_POS))) | (drv << PD26_DRV_POS), priv->pd_drv1_reg);
		reg_writel((reg_readl(priv->pd_pull1_reg) & (~(0X3 << PD26_PULL_POS))) | (pull << PD26_PULL_POS), priv->pd_pull1_reg);	

		spin_unlock_irqrestore(&priv->spinlock, priv->irqflags);
		return 0;
	}

	
	
	spin_unlock_irqrestore(&priv->spinlock, priv->irqflags);
	return 0;
}

static inline int swim_pin_input(swim_priv_t* priv, unsigned char* pin_name){
	swim_pin_set(priv, pin_name, 0, 0, 0, 0);
	
	switch(pin_name[3]){
		case '8':
			return ((readl(priv->pd_data_reg) >> PD28_DATA_POS) & 0x1);
			break;
		case '7':
			return ((readl(priv->pd_data_reg) >> PD27_DATA_POS) & 0x1);
			break;
		case '6':
			return ((readl(priv->pd_data_reg) >> PD26_DATA_POS) & 0x1);
			break;
	}
	
	return 0;
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
	priv->pwm_base_vaddr = vaddr;
	priv->pwm_ch_ctrl = priv->pwm_base_vaddr + PWM_CH_CTRL_OFFSET;
	priv->pwm_ch0_period = priv->pwm_base_vaddr + PWM_CH0_PERIOD_OFFSET;
	priv->pwm_ch1_period = priv->pwm_base_vaddr + PWM_CH1_PERIOD_OFFSET;


	return vaddr;
}
static void pwm_free_iomem(swim_priv_t* priv){
	if(priv->pwm_base_vaddr){
		iounmap(priv->pwm_base_vaddr);
		priv->pwm_base_vaddr = NULL;
	}
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
static inline int pwm_reg_set(swim_priv_t* priv, unsigned char* pin_name, unsigned int enable, unsigned int prescal, unsigned int entire_cys, unsigned int act_cys){
	if('8' == pin_name[3]){
		if(!enable){
			reg_writel(0x0<<PWM_CH0_EN_POS, priv->pwm_ch_ctrl);
			return 0;
		}else{
			
			reg_writel(0x0<<PWM_CH0_EN_POS, priv->pwm_ch_ctrl);
			reg_writel(((entire_cys&PWM_CH0_ENTIRE_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_CYS_POS) | ((act_cys&PWM_CH0_ENTIRE_ACT_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_ACT_CYS_POS), priv->pwm_ch0_period);
			reg_writel((0x0<<PWM0_BYPASS_POS) | (0x1<<PWM_CH0_ACT_STA_POS) | ((prescal&PWM_CH0_PRESCAL_BITFIELDS_MASK)<<PWM_CH0_PRESCAL_POS), priv->pwm_ch_ctrl);
			reg_writel(reg_readl(priv->pwm_ch_ctrl) | (0x1<<SCLK_CH0_GATING_POS) | (0x1<<PWM_CH0_EN_POS), priv->pwm_ch_ctrl);
			
		}
	}

	return 0;
}

static inline int pwm_reg_get(swim_priv_t* priv, unsigned char* pin_name, unsigned int enable, unsigned int prescal, unsigned int entire_cys, unsigned int act_cys){

	if('8' == pin_name[3]){
		
		hensen_debug("pwm_ch_ctrl: %#x\n", reg_readl(priv->pwm_ch_ctrl));
		
		hensen_debug("pwm_ch0_period: %#x\n", reg_readl(priv->pwm_ch0_period));
	
		hensen_debug("pwm_ch1_period: %#x\n", reg_readl(priv->pwm_ch1_period));
		return 0;
	}

	return 0;
}

/* because the prescale val is 0xf, so PWM CLK equals to 24MHz/1 as 24MHz
 * 
*/
static inline void pwm_waveform_set(swim_priv_t* priv, unsigned int prescal, unsigned int entire_cys, unsigned int act_cys){

	pwm_reg_set(priv, PWM, 1, prescal, entire_cys, act_cys);
}
static void pwm_send_bit(swim_priv_t* priv, unsigned char bit){
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

/* After the pulse is finished, the bit(PWM_CH0_PUL_START_POS) will be cleared automatically.
 * pulse_state: 0:low_level, 1:high_level
 * pulse_width:
 * 
*/
static inline int pwm_pulse_set(swim_priv_t* priv, unsigned char* pin_name, unsigned int enable, unsigned int prescal, unsigned int pulse_state, unsigned int pulse_width){

	if('8' == pin_name[3]){
		if(!enable){
			reg_writel(0x0<<PWM_CH0_EN_POS, priv->pwm_ch_ctrl);
			return 0;
		}else{
			
			reg_writel(0x0<<PWM_CH0_EN_POS, priv->pwm_ch_ctrl);
			reg_writel((pulse_width&PWM_CH0_ENTIRE_ACT_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_ACT_CYS_POS, priv->pwm_ch0_period);
			reg_writel((0x0<<PWM0_BYPASS_POS) | (0x1<<PWM_CHANNEL0_MODE_POS) | (pulse_state<<PWM_CH0_ACT_STA_POS) | ((prescal&PWM_CH0_PRESCAL_BITFIELDS_MASK)<<PWM_CH0_PRESCAL_POS), priv->pwm_ch_ctrl);
			reg_writel(reg_readl(priv->pwm_ch_ctrl) | (0x1<<PWM_CH0_PUL_START_POS) | (0x1<<SCLK_CH0_GATING_POS) | (0x1<<PWM_CH0_EN_POS), priv->pwm_ch_ctrl);
			
		}
	}

	return 0;
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



static swim_handle_t swim_send_unit(swim_priv_t* priv, unsigned char data, unsigned char len, unsigned int retry){
		signed char i;
		unsigned char p, m;
		char ack = 0;
	
		a83t_ndelay(priv,2750);
	
		do {
			swim_send_bit(priv, 0);
			p = 0;
			for (i=len-1; i>=0; i--){
				m =(data >> i) & 1;
				swim_send_bit(priv, m);
				p += m;
			}
			// parity bit
			swim_send_bit(priv, (p & 1));
			ack = swim_rvc_ack(priv);
			if (ack == -1){
				return SWIM_TIMEOUT;
			}
		} while (!ack && retry--);
	
		return ack ? SWIM_OK : SWIM_FAIL;
	}



static swim_handle_t swim_soft_reset(swim_priv_t* priv){

    return swim_send_unit(priv, SWIM_CMD_SRST, SWIM_CMD_LEN, SWIM_MAX_RESEND_CNT);
}


static swim_handle_t swim_rcv_unit(swim_priv_t* priv, unsigned char *data, unsigned char len)
{
    char i;
    unsigned char s = 0, m = 0, p = 0, cp = 0;
    char ack = 0;

    for (i=0; i<len+2; i++)
    {
        ack = swim_rvc_bit(priv);
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

    swim_send_ack(priv, ack);

    return ack ? SWIM_OK : SWIM_FAIL;
}

static swim_handle_t swim_bus_read(swim_priv_t* priv, unsigned int addr, unsigned char *buf, unsigned int size)
{
    unsigned char cur_len, i;
    unsigned int cur_addr = addr;
    swim_handle_t ret = SWIM_OK;
    unsigned char first = 0;

    if (!buf)
    {
        return SWIM_FAIL;
    }
    printk("swim_read addr=0x%X, size=0x%X\n", addr, size);
    while (size)
    {
        cur_len = (size > 255) ? 255 : size;

        ret = swim_send_unit(priv, SWIM_CMD_ROTF, SWIM_CMD_LEN, SWIM_MAX_RESEND_CNT);
        if (ret)
        {
            priv->return_line = __LINE__;
            break;
        }
        ret = swim_send_unit(priv, cur_len, 8, 0);
        if (ret)
        {
            priv->return_line = __LINE__;
            break;
        }
        ret = swim_send_unit(priv, (cur_addr >> 16) & 0xFF, 8, 0);
        if (ret)
        {
            priv->return_line = __LINE__;
            break;
        }
        ret = swim_send_unit(priv, (cur_addr >> 8) & 0xFF, 8, 0);
        if (ret)
        {
            priv->return_line = __LINE__;
            break;
        }
        ret = swim_send_unit(priv, (cur_addr >> 0) & 0xFF, 8, 0);
        if (ret)
        {
            priv->return_line = __LINE__;
            break;
        }
        for (i = 0; i < cur_len; i++)
        {
            ret = swim_rcv_unit(priv, buf++, 8);
            if (ret)
            {
                priv->return_line = __LINE__;
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
    hensen_debug("swim_read addr=0x%X, data=0x%X, ret=%d\n", addr, first, ret);
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
static swim_handle_t swim_bus_write(swim_priv_t* priv, unsigned int addr, unsigned char *buf, unsigned int size){
		unsigned char cur_len, i;
		unsigned int cur_addr = addr;
		swim_handle_t ret = SWIM_OK;
	
		if (!buf)
		{
			return SWIM_FAIL;
		}
	
		while (size)
		{
			cur_len = (size > 255) ? 255 : size;
	
			ret = swim_send_unit(priv, SWIM_CMD_WOTF, SWIM_CMD_LEN, SWIM_MAX_RESEND_CNT);
			if (ret)
			{
				priv->return_line = __LINE__;
				break;
			}
			ret = swim_send_unit(priv, cur_len, 8, 0);
			if (ret)
			{
				priv->return_line = __LINE__;
				break;
			}
			ret = swim_send_unit(priv, (cur_addr >> 16) & 0xFF, 8, 0);
			if (ret)
			{
				priv->return_line = __LINE__;
				break;
			}
			ret = swim_send_unit(priv, (cur_addr >> 8) & 0xFF, 8, 0);
			if (ret)
			{
				priv->return_line = __LINE__;
				break;
			}
			ret = swim_send_unit(priv, (cur_addr >> 0) & 0xFF, 8, 0);
			if (ret)
			{
				priv->return_line = __LINE__;
				break;
			}
			for (i = 0; i < cur_len; i++)
			{
				ret = swim_send_unit(priv, *buf++, 8, SWIM_MAX_RESEND_CNT);
				if (ret)
				{
					priv->return_line = __LINE__;
					hensen_debug("Error: send\n");
					break;
				}
			}
			if (ret)
			{
				break;
			}
	
			cur_addr += cur_len;
			size -= cur_len;
		}
	
		return ret;
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
#define RST_CHK_TIMEOUT 100
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


static int stm8_swim_open(struct inode* inodp, struct file* filp){
	//1. Alloc private date, and add private_date into file
	swim_priv_t* priv = kzalloc(sizeof(swim_priv_t), GFP_KERNEL);
	if(!priv){
		printk(KERN_ERR "Kzalloc for swim_priv failed!\n");
		return -ENOMEM;
	}
	spin_lock_init(&priv->spinlock);
	filp->private_data = priv;
	
	//2. get timer0 iomap
	if(!timer0_get_iomem(priv))return -ENOMEM;
	
	//3. get swim_pin iomap
	if(!swim_get_iomem(priv))return -ENOMEM;
	if(!pwm_get_iomem(priv))return -ENOMEM;
	swim_pin_set(priv, PWM, 0x2, 0, 0, 0);


	//swim_start_entry(priv);

	

	
	hensen_debug();
	return 0;
}
static int stm8_swim_release(struct inode* inodp, struct file* filp){
	swim_priv_t * priv = filp->private_data;

//	swim_free_iomem(priv);
	pwm_free_iomem(priv);

	timer0_free_iomem(priv);
	kfree(priv);
	priv = NULL;
	
	hensen_debug();
	return 0;
}

static ssize_t a83t_swim_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    swim_handle_t ret;
    unsigned int addr = *ppos;
    unsigned char *kbuf = NULL;	
	swim_priv_t * priv = filp->private_data;
    
    kbuf = kmalloc(count, GFP_KERNEL);
    if (!kbuf)
    {
        printk("kmalloc %d bytes fail!\n", count);
        return 0;
    }

    hensen_debug("swim read addr = 0x%x, size = 0x%x\n", addr, count);

    local_irq_disable();
    ret = swim_bus_read(priv, addr, kbuf, count);
    if (ret)
    {
        printk("resd fail! ret = %d, priv->return_line = %d\n", ret, priv->return_line);
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
	swim_priv_t * priv = filp->private_data;
    
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
    ret = swim_bus_write(priv, addr, kbuf, count);
    if (ret)
    {
        printk("write fail! ret = %d, priv->return_line = %d\n", ret, priv->return_line);
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
	swim_priv_t * priv = filp->private_data;

    switch (cmd)
    {
        case SWIM_IOCTL_RESET:
            local_irq_disable();
            ret = swim_soft_reset(priv);
            if (ret)
            {
                printk("reset fail! ret = %d, return_line = %d\n", ret, priv->return_line);
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
	swim_priv_t * priv = filp->private_data;
	communication_info_t* info = (communication_info_t*)arg;
	communication_info_t* t;
	
	if(A83T_IOCTL_MAGIC != _IOC_TYPE(cmd)){
		printk(KERN_ERR "unlocked_ioctl error!\n");
		return -EINVAL;
	}
	
    switch(_IOC_NR(cmd)){
        case 0:
			hensen_debug("prescal:%#x\n", info->prescal);
			pwm_waveform_set(priv, info->prescal, info->entire_cys, info->act_cys);
			pwm_reg_get(priv, PWM, 1, 0, 0, 0);
            break;
		case 1:
			pwm_pulse_set(priv, PWM, 1, info->prescal, info->pulse_state, info->pulse_width);
			pwm_reg_get(priv, PWM, 1, 0, 0, 0);
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
