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
#include <linux/time.h>
#include <linux/delay.h>

#include <asm/io.h>
#include <linux/mm.h>
#include <linux/clk.h>

#include <asm/uaccess.h>
#include <asm/string.h>

#include "./stm8_info.h"

static swim_priv_t* pp = NULL;


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
	
	reg_writel((reg_readl(pp->pd_data_reg) & (~(0x1 << PD26_DATA_POS))) | (0x1 << PD26_DATA_POS), pp->pd_data_reg);
	reg_writel((reg_readl(pp->pd_drv1_reg) & (~(0X3 << PD26_DRV_POS))) | (0x1<< PD26_DRV_POS), pp->pd_drv1_reg);
	reg_writel((reg_readl(pp->pd_pull1_reg) & (~(0X3 << PD26_PULL_POS))) | (0x1 << PD26_PULL_POS), pp->pd_pull1_reg);	
	reg_writel((reg_readl(pp->pd_cfg3_reg) & (~(0x7 << PD26_SELECT_POS))) | (0x1 << PD26_SELECT_POS), pp->pd_cfg3_reg);

	return;
}

static void rst_set_as_output_low(void){
	
	reg_writel((reg_readl(pp->pd_data_reg) & (~(0x1 << PD26_DATA_POS))) | (0x0 << PD26_DATA_POS), pp->pd_data_reg);
	reg_writel((reg_readl(pp->pd_drv1_reg) & (~(0X3 << PD26_DRV_POS))) | (0x0<< PD26_DRV_POS), pp->pd_drv1_reg);
	reg_writel((reg_readl(pp->pd_pull1_reg) & (~(0X3 << PD26_PULL_POS))) | (0x2 << PD26_PULL_POS), pp->pd_pull1_reg);	
	reg_writel((reg_readl(pp->pd_cfg3_reg) & (~(0x7 << PD26_SELECT_POS))) | (0x1 << PD26_SELECT_POS), pp->pd_cfg3_reg);

	return;
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
	return;
}

static inline void swim_set_as_input(void){
	reg_writel((reg_readl(pp->pd_cfg3_reg) & (~(0x7 << PD28_SELECT_POS))) | (0x0 << PD28_SELECT_POS), pp->pd_cfg3_reg);
	
	return;
}

static unsigned int swim_get_input_val(void){
	return ((reg_readl(pp->pd_data_reg) >> PD28_DATA_POS) & 0x1);
}


static inline void swim_set_as_output_high(void){
	reg_writel((reg_readl(pp->pd_data_reg) & (~(0x1 << PD28_DATA_POS))) | (0x1 << PD28_DATA_POS), pp->pd_data_reg);
	reg_writel((reg_readl(pp->pd_cfg3_reg) & (~(0x7 << PD28_SELECT_POS))) | (0x1 << PD28_SELECT_POS), pp->pd_cfg3_reg);

	return;
}

static inline void swim_set_as_output_low(void){
#if 0
reg_writel((reg_readl(pp->pd_pull1_reg) & (~(0X3 << PD28_PULL_POS))) | (0x2 << PD28_PULL_POS), pp->pd_pull1_reg);
reg_writel((reg_readl(pp->pd_drv1_reg) & (~(0X3 << PD28_DRV_POS))) | (0x1 << PD28_DRV_POS), pp->pd_drv1_reg);

#endif
	reg_writel((reg_readl(pp->pd_data_reg) & (~(0x1 << PD28_DATA_POS))) | (0x0 << PD28_DATA_POS), pp->pd_data_reg);
	reg_writel((reg_readl(pp->pd_cfg3_reg) & (~(0x7 << PD28_SELECT_POS))) | (0x1 << PD28_SELECT_POS), pp->pd_cfg3_reg);

	return;
}

#if 0
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
#endif 


static inline void pwm_pulse_low_250ns(void){
    // way at low speed
    reg_writel(((ACT_CYS_CNT1&PWM_CH0_ENTIRE_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_CYS_POS) | ((ACT_CYS_CNT1&PWM_CH0_ENTIRE_ACT_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_ACT_CYS_POS), pp->pwm_ch0_period);
   
	reg_writel((PULSE_MODE<<PWM_CHANNEL0_MODE_POS) | (PULSE_STATE_LOW<<PWM_CH0_ACT_STA_POS) | ((PWM_CH0_PRESCAL_VAL&PWM_CH0_PRESCAL_BITFIELDS_MASK)<<PWM_CH0_PRESCAL_POS) | (0x1<<PWM_CH0_PUL_START_POS) | (0x1<<SCLK_CH0_GATING_POS) | (0x1<<PWM_CH0_EN_POS), pp->pwm_ch_ctrl);
	while((reg_readl(pp->pwm_ch_ctrl)>>PWM_CH0_PUL_START_POS)&0x1);	//	wait for PWM_CH0_PUL_START_POS bit cleared automatically(After the pulse is finished.).
}

static inline void pwm_pulse_low_2500ns(void){
    // way at low speed
	reg_writel(((ACT_CYS_CNT0&PWM_CH0_ENTIRE_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_CYS_POS) | ((ACT_CYS_CNT0&PWM_CH0_ENTIRE_ACT_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_ACT_CYS_POS), pp->pwm_ch0_period);

	reg_writel((PULSE_MODE<<PWM_CHANNEL0_MODE_POS) | (PULSE_STATE_LOW<<PWM_CH0_ACT_STA_POS) | ((PWM_CH0_PRESCAL_VAL&PWM_CH0_PRESCAL_BITFIELDS_MASK)<<PWM_CH0_PRESCAL_POS) | (0x1<<PWM_CH0_PUL_START_POS) | (0x1<<SCLK_CH0_GATING_POS) | (0x1<<PWM_CH0_EN_POS), pp->pwm_ch_ctrl);
	while((reg_readl(pp->pwm_ch_ctrl)>>PWM_CH0_PUL_START_POS)&0x1);	//	wait for PWM_CH0_PUL_START_POS bit cleared automatically(After the pulse is finished.).

}

static inline void stay_high_delay_125ns(void){
    // way at low speed
    swim_set_as_pwm();
	reg_writel(((ACT_CYS_CNT3&PWM_CH0_ENTIRE_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_CYS_POS) | ((ACT_CYS_CNT_ZERO&PWM_CH0_ENTIRE_ACT_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_ACT_CYS_POS), pp->pwm_ch0_period);
	reg_writel((PULSE_MODE<<PWM_CHANNEL0_MODE_POS) | (PULSE_STATE_LOW<<PWM_CH0_ACT_STA_POS) | ((PWM_CH0_PRESCAL_VAL&PWM_CH0_PRESCAL_BITFIELDS_MASK)<<PWM_CH0_PRESCAL_POS) | (0x1<<PWM_CH0_PUL_START_POS) | (0x1<<SCLK_CH0_GATING_POS) | (0x1<<PWM_CH0_EN_POS), pp->pwm_ch_ctrl);
	while((reg_readl(pp->pwm_ch_ctrl)>>PWM_CH0_PUL_START_POS)&0x1);	//	wait for PWM_CH0_PUL_START_POS bit cleared automatically(After the pulse is finished.).
	swim_set_as_input();
}

static inline void pwm_stay_high_250ns(void){
    // way at low speed
    
    reg_writel(((ACT_CYS_CNT1&PWM_CH0_ENTIRE_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_CYS_POS) | ((ACT_CYS_CNT_ZERO&PWM_CH0_ENTIRE_ACT_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_ACT_CYS_POS), pp->pwm_ch0_period);
   
	reg_writel((PULSE_MODE<<PWM_CHANNEL0_MODE_POS) | (PULSE_STATE_LOW<<PWM_CH0_ACT_STA_POS) | ((PWM_CH0_PRESCAL_VAL&PWM_CH0_PRESCAL_BITFIELDS_MASK)<<PWM_CH0_PRESCAL_POS) | (0x1<<PWM_CH0_PUL_START_POS) | (0x1<<SCLK_CH0_GATING_POS) | (0x1<<PWM_CH0_EN_POS), pp->pwm_ch_ctrl);
	while((reg_readl(pp->pwm_ch_ctrl)>>PWM_CH0_PUL_START_POS)&0x1);	//	wait for PWM_CH0_PUL_START_POS bit cleared automatically(After the pulse is finished.).
}

static inline void pwm_stay_high_2500ns(void){
    // way at low speed
	reg_writel(((ACT_CYS_CNT0&PWM_CH0_ENTIRE_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_CYS_POS) | ((ACT_CYS_CNT_ZERO&PWM_CH0_ENTIRE_ACT_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_ACT_CYS_POS), pp->pwm_ch0_period);

	reg_writel((PULSE_MODE<<PWM_CHANNEL0_MODE_POS) | (PULSE_STATE_LOW<<PWM_CH0_ACT_STA_POS) | ((PWM_CH0_PRESCAL_VAL&PWM_CH0_PRESCAL_BITFIELDS_MASK)<<PWM_CH0_PRESCAL_POS) | (0x1<<PWM_CH0_PUL_START_POS) | (0x1<<SCLK_CH0_GATING_POS) | (0x1<<PWM_CH0_EN_POS), pp->pwm_ch_ctrl);
	while((reg_readl(pp->pwm_ch_ctrl)>>PWM_CH0_PUL_START_POS)&0x1);	//	wait for PWM_CH0_PUL_START_POS bit cleared automatically(After the pulse is finished.).

}

static inline void pwm_stay_high_2750ns(void){
    // way at low speed
	reg_writel(((ENTIRE_CYS_CNT&PWM_CH0_ENTIRE_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_CYS_POS) | ((ACT_CYS_CNT_ZERO&PWM_CH0_ENTIRE_ACT_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_ACT_CYS_POS), pp->pwm_ch0_period);
	reg_writel((PULSE_MODE<<PWM_CHANNEL0_MODE_POS) | (PULSE_STATE_LOW<<PWM_CH0_ACT_STA_POS) | ((PWM_CH0_PRESCAL_VAL&PWM_CH0_PRESCAL_BITFIELDS_MASK)<<PWM_CH0_PRESCAL_POS) | (0x1<<PWM_CH0_PUL_START_POS) | (0x1<<SCLK_CH0_GATING_POS) | (0x1<<PWM_CH0_EN_POS), pp->pwm_ch_ctrl);
	while((reg_readl(pp->pwm_ch_ctrl)>>PWM_CH0_PUL_START_POS)&0x1);	//	wait for PWM_CH0_PUL_START_POS bit cleared automatically(After the pulse is finished.).

}



static inline void send_swim_bit(unsigned int bit){
	
	struct timespec ts[2];
	
	spin_lock_irq(&pp->spinlock);
	// way at low speed
	if(0==bit){//0
		getnstimeofday(ts);
		pwm_pulse_low_2500ns();	//2500ns
		do{
			getnstimeofday(ts+1);
		}while((ts[1].tv_nsec-ts[0].tv_nsec) < 2750);
		
		//pwm_stay_high_250ns();	//250ns
		//udelay(1);
	}else{//1
		getnstimeofday(ts);
		pwm_pulse_low_250ns();	//250ns	
		do{
			getnstimeofday(ts+1);
		}while((ts[1].tv_nsec-ts[0].tv_nsec) < 2750);
		
		//pwm_stay_high_2500ns();	//2500ns
		//udelay(3);
	}
	spin_unlock_irq(&pp->spinlock);
	return;
}

static char rvc_swim_bit(void){
    unsigned int i;
    unsigned char cnt = 0, flag = 1;
    
#define ACK_CHK_TIMEOUT 22
    for (i=0; i<ACK_CHK_TIMEOUT; i++){
		stay_high_delay_125ns();
        if (LOW == swim_get_input_val()){
            flag = 0;
            cnt++;
        }
        if((flag == 0) && (HIGH == swim_get_input_val()))return (cnt <= 8) ? 1 : 0;
    }
	
    return -1;
}

static void swim_send_ack(unsigned char ack){
    pwm_stay_high_2750ns();
    send_swim_bit(ack);
}


static swim_handle_t swim_send_unit(unsigned char data, unsigned char len, unsigned int retry){
		signed char i;
		register unsigned char p, m;
		char ack = 0;
	
  	  	pwm_stay_high_2750ns();
		
		do {
			send_swim_bit(0);
			p = 0;
			for (i=len-1; i>=0; i = i-1){
				m = (data >> i) & 1;
				send_swim_bit(m);
				p += m;
			}
			send_swim_bit(p&1);		// parity bit
			//pwm_pulse_clear();		//Forcing flushing out the pwm previous pulse from the pulse shift register
			
			swim_set_as_input();
			ack = rvc_swim_bit();
			swim_set_as_pwm();
			if (ack == -1){
				hensen_debug("Error: ACK failed!\n");
				return SWIM_TIMEOUT;
			}
		} while (!ack && retry--);
		
		return ack ? SWIM_OK : SWIM_FAIL;
	}



static swim_handle_t swim_soft_reset(void)
{
    return swim_send_unit(SWIM_CMD_SRST, SWIM_CMD_LEN, SWIM_MAX_RESEND_CNT);
}


static swim_handle_t swim_rcv_unit(unsigned char *data, unsigned char len)
{
    char i;
    unsigned char s = 0, p = 0, cp = 0;
    char ack = 0;
	
	swim_set_as_input();

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
            *data <<= 1;
            *data |= ack;
            cp += ack;
        }
    }

    if (s == 1 && p == (cp & 1))
    {
        ack = 1;
    }
	
	//swim_set_as_pwm();

    //swim_send_ack(ack);
	
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
	
    while (size)
    {
        cur_len = (size > 255) ? 255 : size;

        ret = swim_send_unit(SWIM_CMD_ROTF, SWIM_CMD_LEN, SWIM_MAX_RESEND_CNT);
        if (ret)
        {
            pp->return_line = __LINE__;
   			return ret;
        }
        ret = swim_send_unit(cur_len, 8, 0);
        if (ret)
        {
            pp->return_line = __LINE__;
   			return ret;
        }
        ret = swim_send_unit((cur_addr >> 16) & 0xFF, 8, 0);
        if (ret)
        {
            pp->return_line = __LINE__;
   			return ret;
        }
        ret = swim_send_unit((cur_addr >> 8) & 0xFF, 8, 0);
        if (ret)
        {
            pp->return_line = __LINE__;
   			return ret;
        }
        ret = swim_send_unit((cur_addr >> 0) & 0xFF, 8, 0);
        if (ret)
        {
            pp->return_line = __LINE__;
   			return ret;
        }
        for (i = 0; i < cur_len; i++)
        {
            ret = swim_rcv_unit(buf++, 8);
            if (ret)
            {
                pp->return_line = __LINE__;
				hensen_debug("Error receive\n");
                return SWIM_FAIL;
            }

            if (first == 0)
                first = *(buf - 1);
        }

        cur_addr += cur_len;
        size -= cur_len;
    }
    //hensen_debug("swim_bus_read end addr=0x%X, data=0x%X, ret=%d\n", addr, first, ret);
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
				hensen_debug("Error: send unit data failed.\n");
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
	rst_set_as_output_high();
	mdelay(10);
	
	/*1. To make the SWIM active, the SWIM pin must be forced low during a period of 16us*/
	rst_set_as_output_low();
	mdelay(10);
	swim_set_as_output_low();
	udelay(1000); 	//should be 16us, but 1000us could be better


	/*2. four pulses at 1 kHz followed by four pulses at 2 kHz.*/
    for (i=0; i<4; i++){
		swim_set_as_output_high();
        udelay(500);
		swim_set_as_output_low();
        udelay(500);
    }
    for (i=0; i<4; i++){
		swim_set_as_output_high();
        udelay(250);
		swim_set_as_output_low();
        udelay(250);
    }
	//swim_set_as_high();//stm8 should be already pull the swim line to high level for his next 16us ack
   // 3. Swim is already in Active State
	swim_set_as_input();
	

	//4. Delay for stm8's async ack, about 20us in this For Circle Func totally cost
	//about 16us low level for swim device ack to MCU
	 for(i=0; i<RST_CHK_TIMEOUT; i++){
	 	udelay(1);
        if(!swim_get_input_val()){
			ret = SWIM_OK;
			break;
        }
    }
	 if(ret){
		hensen_debug("Error: swim ack timeout!\n");
	 }else{	 
		ret = SWIM_FAIL;
		 for(i=0; i<RST_CHK_TIMEOUT; i++){
	 		udelay(1);
	        if(swim_get_input_val()){
				ret = SWIM_OK;
				break;
	        }
	    }
	}
		
    //hensen_debug("ret:%d. Send seq header done.\n", ret);

	if(ret){
		rst_set_as_output_high();
		printk(KERN_ERR "Error: Wait ACK from stm8 timeout! %s(%d)\n", __func__, __LINE__);
		goto entry_err0;
	}

	//5. Before start a SWIM communication, the SWIM line must be release at 1 to guarantee that it's ready for communication(at least 300ns)
	mdelay(5);
	swim_set_as_pwm();


#if 1
	/*SWIM_CSR/RST bit is not yet set. Soft option is meaningless(Active state is stil maintain the Active state).*/
	ret = swim_soft_reset();
    if (ret){
        rst_set_as_output_high();
		printk(KERN_ERR "Error: Swim_soft_reset failed! %s(%d)\n", __func__, __LINE__);
		goto entry_err1;
    }
	
	mdelay(5);
	//hensen_debug("Swim soft reset done.\n");
#endif


#if 1
	//test007
	ch = 0xa0;
	swim_bus_write(SWIM_CSR_ADDR, &ch, 1);
	mdelay(1);
	ch = 0;
	swim_bus_read(SWIM_CSR_ADDR, &ch, 1);
	printk(KERN_ALERT "csr:%#x\n", ch);
	mdelay(500);
	
	return 0;
	//test007 end
#endif

    ch = 0xA0;
    ret = swim_bus_write(SWIM_CSR_ADDR, &ch, 1); 
    if (ret){
        rst_set_as_output_high();
		printk(KERN_ERR "Error: swim_write failed!\n");
        return ret;
    }
	//hensen_debug("Swim write SWIM_CSR(0x7f80) 0xA0 done.\n");
	
	rst_set_as_output_high();
	swim_set_as_output_high();

	mdelay(10);
	//hensen_debug("Swim is ready for you!\n");
	
	return SWIM_OK;
	
entry_err1:

entry_err0:
	return SWIM_TIMEOUT;
}


static  inline void my_ndelay(unsigned int ns){
	struct timespec ts[2];
	
	getnstimeofday(ts);
	do{
		getnstimeofday(ts+1);
	}while((ts[1].tv_nsec-ts[0].tv_nsec) < ns);
	
	return;
}

static int stm8_swim_open(struct inode* inodp, struct file* filp){
	//1. Alloc private date, and add private_date into file
	swim_priv_t* priv = kzalloc(sizeof(swim_priv_t), GFP_KERNEL);
	if(!priv){
		printk(KERN_ERR "Kzalloc for swim_priv failed!\n");
		return -ENOMEM;
	}
	filp->private_data = priv;
	pp = priv;
	spin_lock_init(&pp->spinlock);	

	
	//2. get swim_pin iomap
	if(!swim_get_iomem())return -ENOMEM;
	if(!pwm_get_iomem())return -ENOMEM;


	while(1){
		swim_set_as_output_low();
		my_ndelay(2500);
		swim_set_as_output_high();
		my_ndelay(250);
		
		swim_set_as_output_low();
		my_ndelay(250);
		swim_set_as_output_high();
		my_ndelay(2500);
	}
	
	swim_start_entry_new();

	
	hensen_debug();
	return 0;
}
static int stm8_swim_release(struct inode* inodp, struct file* filp){
	swim_priv_t * priv = filp->private_data;

	swim_free_iomem();
	pwm_free_iomem();

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

static struct file_operations stm8_swim_fops = {
	.open = stm8_swim_open,
	.release = stm8_swim_release,
    .write          = a83t_swim_write,
    .read           = a83t_swim_read,
    .llseek         = a83t_swim_seek,
    .unlocked_ioctl = a83t_swim_ioctl,
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
