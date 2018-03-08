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

static  inline void time_ndelay(unsigned int ns){
	if(!pp)return;
	
	getnstimeofday(pp->ts);
	do{
		getnstimeofday(pp->ts+1);
	}while((pp->ts[1].tv_nsec-pp->ts[0].tv_nsec) < ns);
	
}

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

static inline unsigned int swim_get_input_val(void){
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
		
		cnt = 0;
		swim_set_input();		//check ack
		while(swim_get_input_val()){
			if(cnt++ > 160)break;
		}
		if(cnt>160){
			retry = 1;
			if(j++ > 10)return 1;
			printk(KERN_ALERT "St8 Bug: No ack.try again...\n");
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
		if(!(i%8))
			printk(KERN_ALERT "addr(%#x - %#x):%#x %#x %#x %#x %#x %#x %#x %#x.\n", addr, addr+i, buf[i], buf[i+1], buf[i+2], buf[i+3], buf[i+4], buf[i+5], buf[i+6], buf[i+7]);
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
	
	rst_set_output_low();
	mdelay(10);		//1. rst first
	swim_set_output_low();
	mdelay(1);		//2. >16us
	
	entry_sequence();	//3. enter seq
	
	swim_set_input();
	while(swim_get_input_val());
	while(!swim_get_input_val());	//4. Wait for 128*HSI, 16us
	swim_set_output_high();
	mdelay(5);	//5. >300ns
	
#if 0
	if(swim_srst()){
		printk(KERN_ERR "ERROR: Swim soft rst failed!\n");
		return -1;
	}
	mdelay(30);
#endif
	ret = swim_write_byte(0x7f80, 0xa0);
	if(ret){
		printk(KERN_ERR "Set as DM mode failed(ret:%d)!\n", ret);
		return -2;
	}
	mdelay(10);
	rst_set_output_high();
	mdelay(10);		//7. release rst > 1ms

	hensen_debug();
	return 0;
}


static int program_boot(char* buf, unsigned int page_cnt){
	unsigned int i = 0;
	
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
	
	return 0;
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
	if(!pd_get_iomem())return -ENOMEM;

	swim_get_reg_val_to_tmp();	//get val to tmp for a tmp store, to prevent a hardware mis opt with a read.

	active_st8_dm_mode();

	hensen_debug();
	return 0;
}
static int stm8_swim_release(struct inode* inodp, struct file* filp){
	swim_priv_t * priv = filp->private_data;
	int ret = -1;

	if(swim_write_byte(0x7f80, 0xa4)){
		printk(KERN_ERR "Ready for rst failed!\n");
		return -3;
	}
	ret = swim_srst();
	if(ret){
		printk(KERN_ERR "ERROR: Swim soft rst failed(ret:%d)!\n", ret);
		return -4;
	}


	pd_free_iomem();
	kfree(priv);
	priv = NULL;
	pp = NULL;
	
	hensen_debug();
	return 0;
}

static ssize_t a83t_swim_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	unsigned int page_cnt = 0;
    unsigned char *kbuf = NULL;
	unsigned int cnt = 0;
	
    if(count<=0)return -EINVAL;
	if(count>ST8S_PAGE_CNT*ST8S_PAGE_SIZE)count = ST8S_PAGE_CNT*ST8S_PAGE_SIZE;
	
    kbuf = kzalloc(count, GFP_KERNEL);
    if (!kbuf)
    {
        printk("vmalloc %d bytes fail!\n", count);
        return 0;
    }

    if (copy_from_user(kbuf, buf, count))
    {
        printk("copy_from_user fail!\n");
        return 0;
    }
	page_cnt = count/64 + (((count%64) > 0)?1:0);
	printk(KERN_ALERT "count:%d, page_cnt:%d\n", count, page_cnt);
	cnt = program_boot(kbuf, page_cnt);
	if(cnt)
		printk(KERN_ALERT "Error: program_boot failed!(ret:%d)\n", cnt);


    kfree(kbuf);

    return count;
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
