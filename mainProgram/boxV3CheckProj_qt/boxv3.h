#ifndef BOXV3
#define BOXV3

#include <QMutex>
#include <QMutexLocker>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

extern QMutex mutexAT88;

extern void prepareNetEnv(void);
extern void prepareCommunicationEnv(void);
extern void killallOtherApps(char killWlanFlag);
extern void quit_sighandler(int signum);
extern void testPreparation(void);

/*debug: for udp & tcp testing in circle*/
#if 0
#define BOXV3_DEBUG_NET 007
#define BOXV3_TEST_BARCODE "146181202501"
#endif

/*debug: for display more msg at debug mode*/
#if 0
#define BOXV3_DEBUG_DISPLAY 777
#endif

#define BOXV3_SOFTWARE_FILEPATH "/run/sysversion"
#define BOXV3_SOFTWARE_VERSION "BoxV3-Linux3.4.39"
#define BOXV3CHECKAPP_VERSION "V3.1.1"

//bargun tmp file
#define BOX_V3_BAR_CODE_TMP_FILE    "/run/boxv3/barCodeGun/signalCode.txt"
#define BOX_V3_BAR_CODE_TMP_FILE_DIR    "/run/boxv3/barCodeGun"

//box checkout result for usrspace filepath
#define BOXV3_HW_CHECKOUT_FILEDIR  "/opt/private/log/boxv3/checkout/"
#define BOXV3_HW_CHECKOUT_FILENAME "checkResult.txt"
#define BOXV3_HW_CHECKOUT_PASSWD    "777"

//4g tmp file
#define BOXV3_CHECK_4G_TMPFILE_IP "/tmp/lte/ip"
#define BOXV3_CHECK_4G_TMPFILE_DEVICENODE "/tmp/lte/devicenode"
#define BOXV3_CHECK_4G_TMPFILE_MODULE "/tmp/lte/module"
#define BOXV3_CHECK_4G_TMPFILE_ACCESS "/tmp/lte/cpin"
#define BOXV3_CHECK_4G_TMPFILE_DATASERVICE "/tmp/lte/sysinfoex1"
#define BOXV3_CHECK_4G_TMPFILE_SIMST "/tmp/lte/simst_unreal"
#define BOXV3_CHECK_4G_TMPFILE_CME_ERROR "/tmp/lte/cme_error"

//wifi
#define BOXV3_CHECK_WLAN0_TEMFILE_DIR "/tmp/wlan0/"

//wlan0, eth1 for test
#define BOXV3_SELFCHECK_LOGNAME_PREFIX "selfChecklog"
#define WIFI_NAME_V3BOX "wlan0"
#define BOXV3_SELFCHECK_LOGDIR_PATH "/opt/private/log/boxv3/"
#define BOXV3_SELFCHECK_TMPLOG "tmplog.txt"
#define BOXV3_SELFCHECK_DIR_PREFIX_PRO  "productionCheck/"
#define BOXV3_SELFCHECK_DIR_PREFIX_AGING  "agingCheck/"
#define BOXV3_SELFCHECK_DIR_PREFIX_UPTIME  "uptime/"
#define BOXV3_SELFCHECK_DIR_PREFIX_AFTERAGING  "afterAgingCheck/"
#define BOXV3_SELFCHECK_LOGNAME_SUFFIX  ".txt"
#define BOXV3_LOG_MSG_LENGTH    128
#define BOXV3_UI8MAC_LENGTH 8

#define BOXV3_SPI_FLASH_DEVNODE "/dev/spidev0.0"
#define BOXV3_SPI_LORA_DEVNODE "/dev/spidev1.0"
#define BOXV3_FLASH_MF_ID   0xef15  //Do not change this rashly
#define BOXV3_FLASH_MF_ID_STR   "0xef15"
#define BOXV3_LORA_PHYADDR_FORTEST  0x6
#define BOXV3_LORA_VALUE_FORTEST  0x99

#define LINUX_NODE_PROC_MTD "/proc/mtd"
#define BOXV3_MTD_FLASH_PART1_NAME "NorFlash-p0 system"


//picture path
#define BOXV3_QT_PICTURE_PATH "/lib/qt/pictures"
#define BOXV3_QT_PICTURE_SCALED_SIZE    100
#define BOXV3_QT_PICTURE_GIF_LOADING "/lib/qt/pictures/loading.gif"
#define BOXV3_QT_PICTURE_LIGHT_WAIT "/lib/qt/pictures/lightWait.png"
#define BOXV3_QT_PICTURE_LIGHT_RED    "/lib/qt/pictures/lightRed.png"
#define BOXV3_QT_PICTURE_LIGHT_GREEN    "/lib/qt/pictures/lightGreen.png"
#define BOXV3_QT_PICTURE_LIGHT_YELLOW    "/lib/qt/pictures/lightYellow.png"
#define BOXV3_QT_PICTURE_LIGHT_SIZE    15
#define BOXV3_QT_PICTURE_CPU_TEMP    "/lib/qt/pictures/temperature.png"
#define BOXV3_QT_PICTURE_TEMP_WIDTH    100
#define BOXV3_QT_PICTURE_TEMP_HIGH    30

//lib path
#define FONT_DIR_WENQUANYI "/lib/qt/lib/fonts/wenquanyi.ttf"
#define BOXV3_QT_LIBRARY_PATH "/lib/qt/plugins/imageformats/"


//barCodeGun
#define BOXV3_BARCODE_LENGTH    12
enum barCodeGunScanResult_enum
{
    SCAN_WAIT,
    SCAN_BADCODE,
    SCAN_WRONGCODE,
    SCAN_INVALID,
    SCAN_BOXV3_CODE,
    SCAN_AFTER_KEY_PRESSED,
};



//check
//aging min time (hour)
#define BOXV3_AGING_MIN_TIME 48

enum checkflag_enum
{
    SELFCHECK_WAIT,
    SELFCHECK_NO_ADMISSION,
    SELFCHECK_JSON,
    SELFCHECK_PRODUCTION,
    SELFCHECK_AGING,
    SELFCHECK_AGED,
    SELFCHECK_UNKNOWN,
    SELFCHECK_END,
    SELFCHECK_FAILED,
    SELFCHECK_OK,
    SELFCHECK_ALL_STAGE_OK,
    SELFCHECK_ADMIN_PASS,
};
//stm8 version//  boxv3  v1.2.1
#define BOXV3_MAIN_VERSION 0x41
#define BOXV3_SUB_VERSION   0x21

#define BOXV3_FIRMWARE_PATH "/opt/mcufirmware"
#define BOXV3_STM8BOOTAPP_FILEPATH "/usr/sbin/stm8boot"

#define BOXV3_SHELL_FLASH_INIT_PART0 "/opt/flash_init.sh 0"
#define BOXV3_SHELL_FLASH_INIT_PART1 "/opt/flash_init.sh 1"

//LED WORK STYLE
#define ALLLED_FLASH    0xff

#if 0
//LED HW INFO
#define A83T_PIO_BASEADDR   0x01c20800
#define PC_CFG0_REG_OFFSET  0x48
#define PC_DATA_REG_OFFSET  0x58
#define LED0_CFG0_PC7_UNIT  28
#define LED0_DATA_REG_UNIT  0x7
#define LED2_CFG0_PC4_UNIT  16
#define LED2_DATA_REG_UNIT  0X4

#define PE_CFG2_REG_OFFSET  0x98
#define PE_DATA_REG_OFFSET  0xA0
#define LED3_CFG2_PE19_UNIT 12
#define LED3_DATA_REG_UNIT  0x13

#define PH_CFG1_REG_OFFSET  0x100
#define PH_DATA_REG_OFFSET  0x10c
#define LED1_CFG1_PH9_UNIT  4
#define LED1_DATA_REG_UNIT  0x9
#endif
//BOX HW
/* pin group */
#define SUNXI_PA_BASE	0
#define SUNXI_PB_BASE	32
#define SUNXI_PC_BASE	64
#define SUNXI_PD_BASE	96
#define SUNXI_PE_BASE	128
#define SUNXI_PF_BASE	160
#define SUNXI_PG_BASE	192
#define SUNXI_PH_BASE	224
#define SUNXI_PI_BASE	256
#define SUNXI_PJ_BASE	288
#define SUNXI_PK_BASE	320
#define SUNXI_PL_BASE	352
#define SUNXI_PM_BASE	384
#define SUNXI_PN_BASE	416
/* sunxi gpio name space */
#define GPIOA(n)	(SUNXI_PA_BASE + (n))
#define GPIOB(n)	(SUNXI_PB_BASE + (n))
#define GPIOC(n)	(SUNXI_PC_BASE + (n))
#define GPIOD(n)	(SUNXI_PD_BASE + (n))
#define GPIOE(n)	(SUNXI_PE_BASE + (n))
#define GPIOF(n)	(SUNXI_PF_BASE + (n))
#define GPIOG(n)	(SUNXI_PG_BASE + (n))
#define GPIOH(n)	(SUNXI_PH_BASE + (n))
#define GPIOI(n)	(SUNXI_PI_BASE + (n))
#define GPIOJ(n)	(SUNXI_PJ_BASE + (n))
#define GPIOK(n)	(SUNXI_PK_BASE + (n))
#define GPIOL(n)	(SUNXI_PL_BASE + (n))
#define GPIOM(n)	(SUNXI_PM_BASE + (n))
#define GPION(n)	(SUNXI_PN_BASE + (n))
//gpio

//box v3 led info
#define GPIO_INDEX_LED0 GPIOC(7)
#define GPIO_INDEX_LED1 GPIOH(9)
#define GPIO_INDEX_LED2 GPIOC(4)
#define GPIO_INDEX_LED3 GPIOE(19)
//BOX V3 SW3 KEY INFO
#define GPIO_VALUE_BOX_V3_SW3 "/sys/class/gpio/gpio120/value"
typedef struct _KEY_BOXV3_STAUTS
{
    int pressedCnt;
    barCodeGunScanResult_enum barStatus;
    checkflag_enum checkStatus;
    int displayStep;
}key_boxv3_status_t;

//BoxV3 checkout desc info
#define DEVICE_IS_CHECKED_FAILED  0x0
#define DEVICE_IS_BEEN_CHECKING 0x20
#define DEVICE_IS_CHECKED_OK	0x11

enum device_status_enum
{
    NOTCHECK,
    ONCHECK,
    WELLCHECK,
    WELLEVENT1,
    WELLEVENT2,
    WELLEVENT3,
    WELLEVENT4,
    WELLEVENT5,
    WELLEVENT6,
    WELLEVENT7,
    FAILEDCHECK,
    FAILEDEVENT1,
    FAILEDEVENT2,
    FAILEDEVENT3,
    FAILEDEVENT4,
    FAILEDEVENT5,
    FAILEDEVENT6,
    FAILEDEVENT7,
    OVERTIMECHECK,
    DONECHECK,
};

typedef struct RESULTS_INFO{
    char wifi;
    char ntpdate;
    char rtc;
    char sysTime;
    char uart232;
    char uart485;
    char uartFlag;
    char spi0Flash;
    char spi1Lora;
    char iicAt88sc104c;
    char network4G;
    char uart2Stm8;
    char networkLanPort1;
    char networkLanPort2;
    char networkLanPort3;
    char networkLanPort4;
    char networkWan;
    char agingResult;

    char whole; //final check;

    char udpBro;
    char tcpSer;
    RESULTS_INFO()
    {
        wifi = NOTCHECK;
        ntpdate = NOTCHECK;
        rtc = NOTCHECK;
        sysTime = NOTCHECK;
        uart232 = NOTCHECK;
        uart485 = NOTCHECK;
        uartFlag = NOTCHECK;
        spi0Flash = NOTCHECK;
        spi1Lora = NOTCHECK;
        iicAt88sc104c = NOTCHECK;
        network4G = NOTCHECK;
        uart2Stm8 = NOTCHECK;
        networkLanPort1 = NOTCHECK;
        networkLanPort2 = NOTCHECK;
        networkLanPort3 = NOTCHECK;
        networkLanPort4 = NOTCHECK;
        networkWan = NOTCHECK;
        agingResult = NOTCHECK;
        whole = NOTCHECK;

        udpBro = NOTCHECK;
        tcpSer = NOTCHECK;
    }
}results_info_t;

#define RESULTS_INFO_LENGTH sizeof(results_info_t)



extern int getIfconfigIp(QString &netName, QString &ipStr);

#endif // BOXV3

