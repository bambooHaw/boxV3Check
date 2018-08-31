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
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include <asm/uaccess.h>
#include <asm/string.h>
#include <asm/io.h>

#include "./swim_v3.h"

static swim_priv_t* pp = NULL;

//#define DONT_USE_PWM_AS_TIMEER 1

#ifdef DONT_USE_PWM_AS_TIMEER //test pwm begin
static  inline void time_ndelay(unsigned int ns){
	if(!pp)return;
	
	getnstimeofday(pp->ts);
	do{
		getnstimeofday(pp->ts+1);
	}while((pp->ts[1].tv_nsec-pp->ts[0].tv_nsec) < ns);
	
}
#else
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


static void pwm_pulse_low(unsigned int pulse_width){
    // pulse like this： ---|_low__|, 1/24 us as each pulse time.
    //  0 =< pulse_width <= 65534
    int i = 0;
#define PWM_TIMER_TIMEOUT 1000
    reg_writel((((pulse_width-1)&PWM_CH0_ENTIRE_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_CYS_POS) | ((pulse_width&PWM_CH0_ENTIRE_ACT_CYS_BITFIELDS_MASK)<<PWM_CH0_ENTIRE_ACT_CYS_POS), pp->pwm_ch0_period);
	reg_writel((PULSE_MODE<<PWM_CHANNEL0_MODE_POS) | (PULSE_STATE_LOW<<PWM_CH0_ACT_STA_POS) | ((PWM_CH0_PRESCAL_VAL&PWM_CH0_PRESCAL_BITFIELDS_MASK)<<PWM_CH0_PRESCAL_POS) | (0x1<<PWM_CH0_PUL_START_POS) | (0x1<<SCLK_CH0_GATING_POS) | (0x1<<PWM_CH0_EN_POS), pp->pwm_ch_ctrl);
	while(1){	//	wait for PWM_CH0_PUL_START_POS bit cleared automatically(After the pulse is finished.).
		if(i++>PWM_TIMER_TIMEOUT){
			printk(KERN_ERR "Error:pwm func dismiss!\n");
			break;
		}
		if(!((reg_readl(pp->pwm_ch_ctrl)>>PWM_CH0_PUL_START_POS)&0x1))break;
	}
}

static void time_ndelay(unsigned int ns){
	if(ns<42)ns=42;	// because of OSC is 24MHZ, and 1000/24 < 42.
	pwm_pulse_low(ns*24/1000);
}

#endif  //test pwm end

static void __iomem* pd_get_iomem(void){
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
static void pd_free_iomem(void){
	if(pp->port_io_vaddr){
		iounmap(pp->port_io_vaddr);
		pp->port_io_vaddr = NULL;
	}
}

static void swim_get_reg_val_to_tmp(void){
	pp->pd_cfg3_reg_tmp = reg_readl(pp->pd_cfg3_reg);
	pp->pd_pull1_reg_tmp = reg_readl(pp->pd_pull1_reg);
	pp->pd_drv1_reg_tmp = reg_readl(pp->pd_drv1_reg);
	pp->pd_data_reg_tmp = reg_readl(pp->pd_data_reg);
}

static inline void rst_set_output_high(void){
	
	reg_writel((pp->pd_cfg3_reg_tmp & (~(0x7 << PD26_SELECT_POS))) | (0x1 << PD26_SELECT_POS), pp->pd_cfg3_reg);
	reg_writel((pp->pd_data_reg_tmp & (~(0x1 << PD26_DATA_POS))) | (0x1 << PD26_DATA_POS), pp->pd_data_reg);
}

static inline void rst_set_output_low(void){
	
	reg_writel((pp->pd_cfg3_reg_tmp & (~(0x7 << PD26_SELECT_POS))) | (0x1 << PD26_SELECT_POS), pp->pd_cfg3_reg);
	reg_writel((pp->pd_data_reg_tmp & (~(0x1 << PD26_DATA_POS))) | (0x0 << PD26_DATA_POS), pp->pd_data_reg);
}


static inline void swim_set_output_high(void){
	reg_writel((pp->pd_cfg3_reg_tmp & (~(0x7 << PD27_SELECT_POS))) | (0x1 << PD27_SELECT_POS), pp->pd_cfg3_reg);
	reg_writel((pp->pd_data_reg_tmp & (~(0x1 << PD27_DATA_POS))) | (0x1 << PD27_DATA_POS), pp->pd_data_reg);
}

static inline void swim_set_output_low(void){
	reg_writel((pp->pd_cfg3_reg_tmp & (~(0x7 << PD27_SELECT_POS))) | (0x1 << PD27_SELECT_POS), pp->pd_cfg3_reg);
	reg_writel((pp->pd_data_reg_tmp & (~(0x1 << PD27_DATA_POS))) | (0x0 << PD27_DATA_POS), pp->pd_data_reg);
}

static inline void swim_set_output(void){
	reg_writel((pp->pd_cfg3_reg_tmp & (~(0x7 << PD27_SELECT_POS))) | (0x1 << PD27_SELECT_POS), pp->pd_cfg3_reg);
}

static inline void swim_high(void){
	reg_writel((pp->pd_data_reg_tmp & (~(0x1 << PD27_DATA_POS))) | (0x1 << PD27_DATA_POS), pp->pd_data_reg);
}

static inline void swim_low(void){
	reg_writel((pp->pd_data_reg_tmp & (~(0x1 << PD27_DATA_POS))) | (0x0 << PD27_DATA_POS), pp->pd_data_reg);
}

static inline void swim_set_input(void){
	reg_writel((pp->pd_cfg3_reg_tmp & (~(0x7 << PD27_SELECT_POS))) | (0x0 << PD27_SELECT_POS), pp->pd_cfg3_reg);
}

static unsigned int swim_get_input_val(void){
	return ((reg_readl(pp->pd_data_reg) >> PD27_DATA_POS) & 0x1);
}

static inline void swim_send_bit(unsigned char bit){
	if(bit){	
		swim_low();
		time_ndelay(250);
		swim_high();
		time_ndelay(2500);
	}else{
		swim_low();
		time_ndelay(2500);
		swim_high();
		time_ndelay(250);
	}
}

static int swim_send_unit(unsigned char data, unsigned char len){
	signed char i=0, j=0, retry=1, cnt=0;
	unsigned char p=0, m=0;
#define SWIM_SEND_CHECK_TIMEOUT 300

	for(retry=1; retry>0; retry--){
		swim_set_output_high();
		swim_send_bit(0);
		p = 0;
		for (i=len-1; i>=0; --i){
			m = (data >> i) & 1;
			swim_send_bit(m);
			p += m;
		}
		swim_send_bit(p&1);		// parity bit
		
		swim_set_input();		//check ack
		for(cnt=0; cnt<SWIM_SEND_CHECK_TIMEOUT; cnt++){
			if(!swim_get_input_val())
				break;
		}
		if(cnt>SWIM_SEND_CHECK_TIMEOUT){
			j++;
			printk(KERN_ALERT "St8 Bug: No ack.try again...\n");
			if(j > 3){
				swim_set_output_high();
				return -1;
			}else
				continue;
		}
		time_ndelay(1000);	//125*8ns
		if(swim_get_input_val())break;
		time_ndelay(1750);
	}
	
	swim_set_output_high();
	return (retry<=0);
}

static int swim_write_byte(unsigned int addr, char data){

	if(swim_send_unit(SWIM_CMD_WOTF, SWIM_CMD_LEN))return -1;
	if(swim_send_unit(1, 8))return -2;		//N
	if(swim_send_unit(0, 8))return -3;		//@E
	if(swim_send_unit((addr>>8)&0xff, 8))return -4;		//@H
	if(swim_send_unit(addr&0xff, 8))return -5;		//@L
	if(swim_send_unit(data, 8))return -6;
	
	return 0;
}
static int swim_write_buf(unsigned int addr, char* buf, unsigned int n){
	unsigned int i = 0;
	
	if(swim_send_unit(SWIM_CMD_WOTF, SWIM_CMD_LEN))return -1;
	if(swim_send_unit(n, 8))return -2;		//N
	if(swim_send_unit(0, 8))return -3;		//@E
	if(swim_send_unit((addr>>8)&0xff, 8))return -4;		//@H
	if(swim_send_unit(addr&0xff, 8))return -5;		//@L
	for(i=0; i<n; i++){
		#if 0
		if(!(i%8))
			printk(KERN_ALERT "addr(%#x-%#x):|%#x %#x %#x %#x %#x %#x %#x %#x|\n", addr+i, addr+i+7, buf[i], buf[i+1], buf[i+2], buf[i+3], buf[i+4], buf[i+5], buf[i+6], buf[i+7]);
		#endif
		if(swim_send_unit(buf[i], 8))break;
	}
	
	if(i<n)return -6;
	
	return 0;
}

static void entry_sequence(void){
	int i = 0;
	swim_set_output_high();
	for(i=0; i<4; i++){
		swim_high();
		udelay(500);
		swim_low();
		udelay(500);
	}
	for(i=0; i<4; i++){
		swim_high();
		udelay(250);
		swim_low();
		udelay(250);
	}
	swim_set_output_high();
}

static int swim_srst(void){
	return swim_send_unit(SWIM_CMD_SRST, SWIM_CMD_LEN);
}

static int active_st8_dm_mode(void){
	int ret = -1;
#define SEQUENCE_ACK_TIMEOUT 3000
#define SEQUENCE_ACK_WRONG	1000

	rst_set_output_low();
	mdelay(10);		//1. rst first
	swim_set_output_low();
	mdelay(1);		//2. >16us
	
	entry_sequence();	//3. enter seq

	ret = 0;
	swim_set_input();
	for(ret=0; ret<SEQUENCE_ACK_TIMEOUT; ret++){
		if(!swim_get_input_val())break;
	}
	if(ret>=SEQUENCE_ACK_TIMEOUT)
		return -1;
	for(ret=0; ret<SEQUENCE_ACK_WRONG; ret++){	//4. Wait for 128*HSI, 16us
		if(swim_get_input_val())break;
	}
	if(ret>=SEQUENCE_ACK_WRONG)
		return -2;
	
	swim_set_output_high();
	mdelay(5);	//5. >300ns
	
#if 0	//in this point srst is inavailable.
	if(swim_srst()){
		printk(KERN_ERR "ERROR: Swim soft rst failed!\n");
		return -1;
	}
	mdelay(30);
#endif
	ret = swim_write_byte(0x7f80, 0xa0);
	if(ret){
		printk(KERN_ERR "Set as DM mode failed(ret:%d)!\n", ret);
		return -3;
	}
	mdelay(10);
	rst_set_output_high();
	mdelay(10);		//7. release rst > 1ms

	return 0;
}


static int program_boot(char* buf, unsigned int page_cnt){
	unsigned int i = 0;
	
	spin_lock_irqsave(&pp->spinlock, pp->irqflags);
	if(swim_write_byte(0x7f99, 0x08))return -1;
	if(swim_write_byte(0x5062, 0x56))return -2;
	mdelay(1);
	if(swim_write_byte(0x5062, 0xae))return -3;//unlock flash
	mdelay(10);
	for(i=0; i<page_cnt; i++){
		if(swim_write_byte(0x505b, 0x01))return -4;
		if(swim_write_byte(0x505c, 0xfe))return -5;
		if(swim_write_buf(0x8000+i*64, buf+i*64, 64))return -6;
		mdelay(10);
	}
	spin_unlock_irqrestore(&pp->spinlock, pp->irqflags);
	
	return 0;
}
static int stm8_swim_open(struct inode* inodp, struct file* filp){
	int err = 0;
	
	//1. Alloc private date, and add private_date into file
	swim_priv_t* priv = kzalloc(sizeof(swim_priv_t), GFP_KERNEL);
	if(!priv){
		printk(KERN_ERR "Kzalloc for swim_priv failed!\n");
		err = -ENOMEM;
		goto swim_open_err0;
	}
	filp->private_data = priv;
	pp = priv;

	//2. get swim_pin iomap
	if(!pd_get_iomem()){
		printk(KERN_ERR "pd_get_iomem failed!\n");
		err = -ENOMEM;
		goto swim_open_err1;
	}
	
#ifndef DONT_USE_PWM_AS_TIMEER
	if(!pwm_get_iomem()){
		printk(KERN_ERR "pwm_get_iomem failed!\n");
		err = -ENOMEM;
		goto swim_open_err2;
	}
#endif
	swim_get_reg_val_to_tmp();	//get val to tmp for a tmp store, to prevent a hardware mis opt with a read.

	
	if(active_st8_dm_mode()){
		printk(KERN_ERR "active_st8_dm_mode failed!\n");
		err = -EAGAIN;
		goto swim_open_err3;
	}


	return 0;

swim_open_err3:
#ifndef DONT_USE_PWM_AS_TIMEER
	pwm_free_iomem();
#endif
swim_open_err2:
	pd_free_iomem();
swim_open_err1:
	kfree(priv);
	priv = NULL;
	pp = NULL;
swim_open_err0:
	return err;
}
static int stm8_swim_release(struct inode* inodp, struct file* filp){
	swim_priv_t * priv = filp->private_data;
	int ret = 0;
	ret = swim_write_byte(0x7f80, 0xa4);
	if(ret){
		printk(KERN_ERR "Ready for rst failed!\n");
		goto release_write_err;
	}
	ret = swim_srst();
	if(ret){
		printk(KERN_ERR "ERROR: Swim soft rst failed(ret:%d)!\n", ret);
		goto release_srst_err;
	}

#ifndef DONT_USE_PWM_AS_TIMEER
	pwm_free_iomem();
#endif
	pd_free_iomem();
	kfree(priv);
	priv = NULL;
	pp = NULL;

	return ret;

release_srst_err:
release_write_err:
#ifndef DONT_USE_PWM_AS_TIMEER
	pwm_free_iomem();
#endif
	pd_free_iomem();
	kfree(priv);
	priv = NULL;
	pp = NULL;

	return ret;
}

static ssize_t a83t_swim_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	unsigned int page_cnt = 0;
    unsigned char *kbuf = NULL;
	unsigned int ret = 0;
	
    if(count<=0)return -EINVAL;
	if(count>ST8S_PAGE_CNT*ST8S_PAGE_SIZE)count = ST8S_PAGE_CNT*ST8S_PAGE_SIZE;
	
    kbuf = kzalloc(count, GFP_KERNEL);
    if (!kbuf){
    	ret = -ENOMEM;
        printk(KERN_ERR "kzalloc %d bytes fail!\n", count);
		goto wrtie_kzalloc_err;
    }

    if (copy_from_user(kbuf, buf, count)){
		ret = -EACCES;
        printk("copy_from_user fail!\n");\
        goto write_copy_err;
    }
	page_cnt = count/64 + (((count%64) > 0)?1:0);
	//hensen_debug("count:%d, page_cnt:%d\n", count, page_cnt);
	
	ret = program_boot(kbuf, page_cnt);
	if(ret){
		printk(KERN_ALERT "Error: program_boot failed!(ret:%d)\n", ret);
		goto write_program_boot_err;
	}

    kfree(kbuf);
    return count;

write_program_boot_err:
write_copy_err:
	kfree(kbuf);
wrtie_kzalloc_err:
	return ret;
}


static struct file_operations stm8_swim_fops = {
	.open = stm8_swim_open,
	.release = stm8_swim_release,
    .write          = a83t_swim_write,
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
MODULE_ALIAS("Nothing");
