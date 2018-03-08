#ifndef __SWIM_V3__
#define __SWIM_V3__


#include <linux/spinlock.h>
#include <asm/io.h>
#include <linux/time.h>

#define RST "PD26"
#define SWIM "PD27"

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
	
	void __iomem*  port_io_vaddr;
	void __iomem*  pd_cfg3_reg;	//bit[14:12] -> PD27_SELECT:  000:input  001:output
	void __iomem*  pd_data_reg; //input status / output val
	void __iomem*  pd_drv1_reg; //level0, 1, 2, 3
	void __iomem*  pd_pull1_reg;//00:pull-up/down disable, 01:pull-up, 10:pull-down, 11:reserved

	unsigned int	pd_cfg3_reg_tmp;
	unsigned int	pd_data_reg_tmp;
	unsigned int	pd_drv1_reg_tmp;
	unsigned int	pd_pull1_reg_tmp;


	unsigned int return_line;
	void* private_date;
}swim_priv_t;


#define reg_readl(addr)		(*((volatile unsigned long  *)(addr)))
#define reg_writel(v, addr)		(*((volatile unsigned long  *)(addr)) = (unsigned long)(v))


#define hensen_debug(fmt, args...) do{printk(KERN_ALERT "%s(%d)."fmt, __func__, __LINE__, ##args);\
										printk(KERN_ALERT "\n");\
									}while(0)


#endif
