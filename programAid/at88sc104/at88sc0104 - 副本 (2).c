#include "at88sc0104.h"

#define CM_MAJOR	250
#define PINMUX0     __REG(0x01c40000)
#define PINMUX1     __REG(0x01c40004)
#define baseaddr     0 //0x01c4
//PINMUX0 = PINMUX0 & 0xFFFFF9FF; //set gpio[8] and gpio[9] is gpio mode 
// -------------------------------------------------------------------------------------------------
// Data
// -------------------------------------------------------------------------------------------------


#define AT88_STATUS_ERROR -1
#define AT88_STATUS_UNINIT 0
#define AT88_STATUS_INITED 1
#define AT88_STATUS_BURNED 2
#define AT88_STATUS_INITED_BURNED 3
int at88_status=AT88_STATUS_ERROR; // -1 设备错误或未检测到  0 设备未初始化 1 设备初始化，但是没有烧fuse 2 设备未初始化，但是fuse已烧 3 设备初始化并且烧了fuse
uchar ucCM_DevGpaRegisters[16][Gpa_Regs];
uchar ucCM_DevEncrypt[16];
uchar ucCM_DevAuthenticate[16];
uchar ucCM_DevUserZone[16];
uchar ucCM_DevAntiTearing[16];

int block_num=0;

void __iomem *regdir, *regdat;

//GC_TABLE: 

const uchar GC0[]={0x03,0x21,0x0B,0x46,0x05,0x22,0xCD,0xE9};//GC0

const uchar GC1[]={0x34,0x12,0x36,0x76,0x87,0x24,0x23,0x45};//GC1

const uchar GC2[]={0x4e,0xb9,0x29,0xa0,0x04,0xf0,0xc0,0xfb};//GC2 

const uchar GC3[]={0x8c,0x21,0x12,0xa0,0x08,0xf0,0x96,0xff};//GC3



const uchar *GC_TABLE[4]={GC0,GC1,GC2,GC3};

const uchar	GC_INDEX[4]={0,0,2,3};

//******************When main pass,modify PASSWORD************************************

//PASSWORD_TABLE:

const uchar PW_WRITE0[]={0xE3,0xF7,0x74};//WRITE PASSWORD 0

const uchar PW_READ0[]= {0xE3,0xF7,0x74};//READ  PASSWORD 0

const uchar PW_WRITE1[]={0x06,0x58,0x49};//WRITE PASSWORD 1

const uchar PW_READ1[]= {0x06,0x58,0x49};//READ  PASSWORD 1

const uchar PW_WRITE2[]={0x08,0xe0,0x41};//WRITE PASSWORD 2

const uchar PW_READ2[]= {0x08,0xe0,0x41};//READ  PASSWORD 2

const uchar PW_WRITE3[]={0xf2,0xd8,0x40};//WRITE PASSWORD 3

const uchar PW_READ3[]= {0xf2,0xd8,0x40};//READ  PASSWORD 3

const uchar PW_WRITE4[]={0x23,0x54,0x56};//WRITE PASSWORD 4 

const uchar PW_READ4[]= {0x23,0x54,0x56};//READ  PASSWORD 4

const uchar PW_WRITE5[]={0x75,0x62,0x51};//WRITE PASSWORD 5

const uchar PW_READ5[]= {0x75,0x62,0x51};//READ  PASSWORD 5

const uchar PW_WRITE6[]={0x68,0x78,0x41};//WRITE PASSWORD 6

const uchar PW_READ6[]= {0x68,0x78,0x41};//READ  PASSWORD 6

const uchar PW_WRITE7[]={0xdd,0x42,0x97};//WRITE PASSWORD 7

const uchar PW_READ7[]= {0x77,0x77,0x77};//READ  PASSWORD 7



const uchar *PW_WRITE_TABLE[8]={PW_WRITE0,PW_WRITE1,PW_WRITE2,PW_WRITE3,PW_WRITE4,PW_WRITE5,PW_WRITE6,PW_WRITE7};

const uchar *PW_READ_TABLE[8]={PW_READ0,PW_READ1,PW_READ2,PW_READ3,PW_READ4,PW_READ5,PW_READ6,PW_READ7};

const uchar PW_INDEX[4]={0,0,2,3};


// Data Structures that configure the low level CryptoMemory functions

// CryptoMemory Low Level Linkage
// 
cm_low_level CM_LOW_LEVEL = {
	cm_TRUE,         // Carddetect
	cm_PowerOff,     // PowerOff
	cm_PowerOn,      // PowerOn
	cm_SendCommand,  // SendCommand
	cm_ReceiveData,  // ReceiveRet
	cm_SendData,     // SendData
	cm_RandGen,      // RandomGen
	cm_WaitClock,    // WaitClock
	cm_SendCmdByte   // SendCmdByte
};

// CryptoMemory Low Level Configuration
//
// Note: the port address is included in a manner that does not require a chip
//       specific header file. Note, the address of the port is the LAST address
//       of the group of three addresses of the port (the port output register).
//
cm_port_cfg  CM_PORT_CFG = {
	0xb0, // ucChipSelect        (0xb0 is default address for CryptoMemory)
	baseaddr, // ucClockPort         (0x32 is PORTD)
	10,    // ucClockPin          (SCL on bit 0)
	baseaddr, // ucDataPort          (0x32 is PORTD)
	11,    // ucDataPin           (SDA on bit 2)    
	baseaddr, // ucCardSensePort     (0x32 is PORTD)
	10,    // ucCardSensePin      (card sense switch, if any, on bit 2) 
	TRUE, // ucCardSensePolarity (TRUE -> "1" on bit in register means card is inserted)
	baseaddr, // ucPowerPort         (0x32 is PORTD)
	11,    // ucPowerPin          (power control, if any, on bit 3)
	TRUE, // ucPowerPolarity     (TRUE -> "1" on bit in register supplies power)
	10000,  // ucDelayCount
	100    // ucStartTries
};

// -------------------------------------------------------------------------------------------------
// Functions
// -------------------------------------------------------------------------------------------------
void __iomem *data_input_viradress;
int gpio_set_direction(int num,int dir)
{
	int tmp;

	tmp = readl(regdir);
	tmp &= ~(0x7<<(num*4));
	if(dir ==GIO_DIR_OUTPUT )
	{
		tmp|=(0x1<<(num*4));
	}

	writel(tmp,regdir);
	return 0;
}
int __gpio_set(int num,int val)
{
	int tmp;

	tmp = readl(regdat);
	if(val)
	{
		tmp|=(0x1<<num);
	}
	else
	{
		tmp&=~(0x1<<num);
	}
	writel(tmp,regdat);

	return 0;
}
int __gpio_get(int num)
{
	int tmp;

	tmp = readl(regdat);

	return ((tmp>>num)&0x1);
}
int cm_TRUE(void)
{
	return TRUE;
}

// Select Chip
int cm_SelectChip(uchar ucChipNew)
{
	uchar ucChip, i;

	if (ucChipNew == 0xFF) {
		// Clear All State
		for (ucChip = 0; ucChip < 16; ++ucChip) {
			for (i = 0; i < 16; ++i) ucCM_DevGpaRegisters[ucChip][i] = 0;
			ucCM_DevEncrypt[ucChip] = 0;
			ucCM_DevAuthenticate[ucChip] = 0;
			ucCM_DevUserZone[ucChip] = 0;
			ucCM_DevAntiTearing[ucChip] = 0;
		}
	}
	else {
		ucChip = CM_PORT_CFG.ucChipSelect;
		if (ucChip != ucChipNew) {
			// Set Chip Select
			CM_PORT_CFG.ucChipSelect = ucChipNew;

			// Shift Addresses
			ucChip = (ucChip>>4)&0xF0;
			ucChipNew = (ucChipNew>>4)&0xF0;

			// Swap GPA Registers
			for (i = 0; i < 16; ++i) {
				ucCM_DevGpaRegisters[ucChip][i] = ucGpaRegisters[i];
				ucGpaRegisters[i] = ucCM_DevGpaRegisters[ucChipNew][i];
			}

			//Save State 
			ucCM_DevEncrypt[ucChip] = ucCM_Encrypt;
			ucCM_DevAuthenticate[ucChip] = ucCM_Authenticate;
			ucCM_DevUserZone[ucChip] = ucCM_UserZone;
			ucCM_DevAntiTearing[ucChip] = ucCM_AntiTearing;

			// Restore Saved State
			ucCM_Encrypt = ucCM_DevEncrypt[ucChipNew];
			ucCM_Authenticate = ucCM_DevAuthenticate[ucChipNew];
			ucCM_UserZone = ucCM_DevUserZone[ucChipNew];
			ucCM_AntiTearing = ucCM_DevAntiTearing[ucChipNew];
		}
	}

	return SUCCESS;
}



// Local function prototypes
static uchar cm_AuthenEncrypt(uchar ucCmd1, uchar ucAddrCi, puchar pucCi, puchar pucG_Sk, puchar pucRandom);

// Global Data
uchar ucCM_Ci[8], ucCM_G_Sk[8];
uchar ucCM_Q_Ch[16], ucCM_Ci2[8];

// Activate Security
//
// When called the function:
// ?reads the current cryptogram (Ci) of the key set, 
// ?computes the next cryptogram (Ci+1) based on the secret key pucKey (GCi) and the random number selected,
// ?sends the (Ci+1) and the random number to the CryptoMemory?device, 
// ?computes (Ci+2) and compares its computed value the new cryptogram of the key set.
// ?If (Ci+2) matches the new cryptogram of the key set, authentication was successful.
// In addition, if ucEncrypt is TRUE the function:
// ?computes the new session key (Ci+3) and a challenge, 
// ?sends the new session key and the challenge to the CryptoMemory?device, 
// ?If the new session key and the challenge are correctly related, encryption is activated.
//
int cm_ActiveSecurity(uchar ucKeySet, puchar pucKey, puchar pucRandom, uchar ucEncrypt)
{
	uchar i;
	uchar ucAddrCi;
	int ret;

	// Read Ci for selected key set
	ucAddrCi = CM_Ci + (ucKeySet<<4);              // Ci blocks on 16 byte boundries
	if ((ret = cm_ReadConfigZone(ucAddrCi, ucCM_Ci, 8)) != SUCCESS) return ret;
	// Try to activate authentication
	for (i = 0; i < 8; ++i) ucCM_G_Sk[i] = pucKey[i];
	if ((ret = cm_AuthenEncrypt(ucKeySet, ucAddrCi, ucCM_Ci, ucCM_G_Sk, pucRandom)) != SUCCESS) return ret;
	ucCM_Authenticate = TRUE;
	// If Encryption required, try to activate that too
	if (ucEncrypt) {
		if (pucRandom) pucRandom += 8;
		if ((ret = cm_AuthenEncrypt(ucKeySet+0x10, ucAddrCi, ucCM_Ci, ucCM_G_Sk, pucRandom)) != SUCCESS) return ret;
		ucCM_Encrypt = TRUE;
	}
	// Done
	return SUCCESS;
}

// Common code for both activating authentication and encryption
static uchar cm_AuthenEncrypt(uchar ucCmd1, uchar ucAddrCi, puchar pucCi, puchar pucG_Sk, puchar pucRandom)
{
	uchar i;
	uchar ucReturn;

	// Generate chalange data
	if (pucRandom) for (i = 0; i < 8; ++i) ucCM_Q_Ch[i] = pucRandom[i];
	else           CM_LOW_LEVEL.RandomGen(ucCM_Q_Ch);

	//for (i = 0; i < 8; ++i) ucCM_Q_Ch[i] = pucRandom[i];
	cm_AuthenEncryptCal(pucCi, pucG_Sk, ucCM_Q_Ch, &ucCM_Q_Ch[8]);

	// Send chalange
	ucCM_InsBuff[0] = 0xb8;
	ucCM_InsBuff[1] = ucCmd1;
	ucCM_InsBuff[2] = 0x00;
	ucCM_InsBuff[3] = 0x10;
	if ((ucReturn = cm_WriteCommand(ucCM_InsBuff, ucCM_Q_Ch, 16)) != 16) return ucReturn;

	// Give chips some clocks to do calculations
	CM_LOW_LEVEL.WaitClock(3);

	// Verify result
	if ((ucReturn = cm_ReadConfigZone(ucAddrCi, ucCM_Ci2, 8)) != SUCCESS) return ucReturn;
	for(i=0; i<8; i++) if (pucCi[i]!=ucCM_Ci2[i]) return FAILED;

	// Done
	return SUCCESS;
}

// 1/2 Clock Cycle transition to HIGH
//
void cm_Clockhigh(void)
{
	cm_Delay(1);
	CM_CLK_HI;
	cm_Delay(1);
}

// 1/2 Clock Cycle transition to LOW
//
void cm_Clocklow(void)
{
	cm_Delay(1);
	CM_CLK_LO;
	cm_Delay(1);
}

// Do one full clock cycle
//
// Changed 1/19/05 to eliminate one level of return stack requirements
//
void cm_ClockCycle(void)
{
	cm_Delay(1);
	CM_CLK_LO;
	cm_Delay(2);
	CM_CLK_HI;
	cm_Delay(1);
}

// Do a number of clock cycles
//
void cm_ClockCycles(uchar ucCount)
{
	uchar i;

	for (i = 0; i < ucCount; ++i) cm_ClockCycle();
}

// Send a start sequence
//
// Modified 7-21-04 to correctly set SDA to be an output
// 
void cm_Start(void)
{
	CM_DATA_OUT;                         // Data line must be an output to send a start sequence
	cm_Clocklow();
	CM_DATA_HI;
	cm_Delay(4);
	cm_Clockhigh();
	cm_Delay(4);
	CM_DATA_LO;
	cm_Delay(8);
	cm_Clocklow();
	cm_Delay(8);
}

// Send a stop sequence
//
// Modified 7-21-04 to correctly set SDA to be an output
// 
void cm_Stop(void)
{
	CM_DATA_OUT;                         // Data line must be an output to send a stop sequence
	cm_Clocklow();
	CM_DATA_LO;
	cm_Delay(4);
	cm_Clockhigh();
	cm_Delay(8);
	CM_DATA_HI;
	cm_Delay(4);
}

// Write a byte
//
// Returns 0 if write successed, 1 if write fails failure
//
// Modified 7-21-04 to correctly control SDA
// 
int cm_Write(uchar ucData)
{
	int i;
	CM_DATA_OUT;                         // Set data line to be an output
	for(i=0; i<8; i++) 
	{                 // Send 8 bits of data
		cm_Clocklow();
		if (ucData&0x80)
		{
			CM_DATA_HI;
		}
		else
		{
			CM_DATA_LO;
		}
		cm_Clockhigh();
		ucData = ucData<<1;
	}
	cm_Clocklow();
	// wait for the ack
	CM_DATA_IN;                      // Set data line to be an input
	cm_Delay(8);
	cm_Clockhigh();
	while(i>1) 
	{                    // loop waiting for ack (loop above left i == 8)
		cm_Delay(2);
		if (CM_DATA_RD) i--;        // if SDA is high level decrement retry counter
		else i = 0;
	}   

	cm_Clocklow();
	CM_DATA_OUT;                     // Set data line to be an output
	return i;
}

// Send a ACK or NAK or to the device
void cm_AckNak(uchar ucAck)
{
	CM_DATA_OUT;                         // Data line must be an output to send an ACK
	cm_Clocklow();
	if (ucAck) CM_DATA_LO;               // Low on data line indicates an ACK
	else       CM_DATA_HI;               // High on data line indicates an NACK
	cm_Delay(2);
	cm_Clockhigh();
	cm_Delay(8);
	cm_Clocklow();
}

#ifdef PIGS_FLY

// ------------------------------------------------------------------------------------- 
// Original Version
// ------------------------------------------------------------------------------------- 

// Send a ACK to the device
void cm_Ack(void)
{
	CM_DATA_OUT;                         // Data line must be an output to send an ACK
	cm_Clocklow();
	CM_DATA_LO;                          // Low on data line indicates an ACK
	cm_Delay(2);
	cm_Clockhigh();
	cm_Delay(8);
	cm_Clocklow();
	//SET_SDA;
}

// Send a NACK to the device
void cm_N_Ack(void)
{
	CM_DATA_OUT;                         // Data line must be an output to send an NACK
	cm_Clocklow();
	CM_DATA_HI;                          // High on data line indicates an NACK
	cm_Delay(2);
	cm_Clockhigh();
	cm_Delay(8);
	cm_Clocklow();
	//SET_SDA;
}

// ------------------------------------------------------------------------------------- 
// Version that uses one less level of call stack
// ------------------------------------------------------------------------------------- 

// Send a ACK to the device
void cm_Ack(void)
{
	CM_DATA_OUT;                         // Data line must be an output to send an ACK
	cm_Delay(1);
	CM_CLK_LO;
	cm_Delay(1);
	CM_DATA_LO;                          // Low on data line indicates an ACK
	cm_Delay(3);
	CM_CLK_HI;
	cm_Delay(9);
	cm_Clocklow();
}

// Send a NACK to the device
void cm_N_Ack(void)
{
	CM_DATA_OUT;                         // Data line must be an output to send an NACK
	cm_Delay(1);
	CM_CLK_LO;
	cm_Delay(1);
	CM_DATA_HI;                          // High on data line indicates an NACK
	cm_Delay(2);
	CM_CLK_HI;
	cm_Delay(9);
	cm_Clocklow();
}
#endif

//     Read a byte from device, MSB
//
// Modified 7-21-04 to correctly control SDA
// 
uchar cm_Read(void)
{
	uchar i;
	uchar rByte = 0;

	CM_DATA_IN;                          // Set data line to be an input
	CM_DATA_HI;
	for(i=0x80; i; i=i>>1)
	{
		cm_ClockCycle();
		if (CM_DATA_RD) rByte |= i;
		cm_Clocklow();
	}
	CM_DATA_OUT;                         // Set data line to be an output
	//printk("%c",rByte);
	return rByte;
}

void cm_WaitClock(uchar loop)
{
	uchar i, j;

	CM_DATA_LO;
	for(j=0; j<loop; j++) {
		cm_Start();
		for(i = 0; i<15; i++) cm_ClockCycle();
		cm_Stop();
	}
}

// Send a command
//
int cm_SendCommand(puchar pucInsBuff)
{
	uchar i, ucCmd;

	i = CM_START_TRIES;
	ucCmd = (pucInsBuff[0]&0x0F)|CM_PORT_CFG.ucChipSelect;
	while (i) {
		cm_Start();
		if (cm_Write(ucCmd) == 0) break;
		if (--i == 0) return FAIL_CMDSTART;
	}
	for(i = 1; i< 4; i++) {
		if (cm_Write(pucInsBuff[i]) != 0) return FAIL_CMDSEND;
	}
	return SUCCESS;
}

int cm_ReceiveData(puchar pucRecBuf, int len)
{
	int i;

	for(i = 0; i < (len-1); i++) {
		pucRecBuf[i] = cm_Read();
		cm_AckNak(TRUE);
	}
	pucRecBuf[i] = cm_Read();
	cm_AckNak(FALSE);
	cm_Stop();
	return SUCCESS;
}

int cm_SendData(puchar pucSendBuf, int len)
{
	int i;
	for(i = 0; i< len; i++) {
		if (cm_Write(pucSendBuf[i])==1) return FAIL_WRDATA;
	}
	cm_Stop();

	//return SUCCESS;
	return len;
}

// Set User Zone
int cm_SetUserZone(uchar ucZoneNumber, uchar ucAntiTearing)
{
	int ret;

	ucCM_InsBuff[0] = 0xb4;
	if (ucAntiTearing) ucCM_InsBuff[1] = 0x0b;
	else 	           ucCM_InsBuff[1] = 0x03;
	ucCM_InsBuff[2] = ucZoneNumber;
	ucCM_InsBuff[3] = 0x00;

	// Only zone number is included in the polynomial
	cm_GPAGen(ucZoneNumber);

	if ((ret = CM_LOW_LEVEL.SendCommand(ucCM_InsBuff))!= SUCCESS) return ret;

	// save zone number and anti-tearing state
	ucCM_UserZone = ucZoneNumber;
	ucCM_AntiTearing = ucAntiTearing;

	// done	
	return  SUCCESS;//CM_LOW_LEVEL.ReceiveRet(NULL,0);
}

uchar ucCmdWrFuze[4] = {0xb4, 0x01, 0x00, 0x00};

// Burn Fuse
int cm_BurnFuse(uchar ucFuze)
{
	int ret;

	// Burn Fuze
	ucCmdWrFuze[2] = ucFuze;
	if((ret = CM_LOW_LEVEL.SendCommand(ucCmdWrFuze))!= SUCCESS) return ret;

	// done	
	return  SUCCESS;//CM_LOW_LEVEL.ReceiveRet(NULL,0);
}

int cm_CardDetect(void)
{
	CM_DETECT_IN;                                       // Make detect pin an input
	if (CM_DETECT_RD) return CM_DETECT_POL?TRUE:FALSE;  // Adjust pin HI for polarity
	return CM_DETECT_POL?FALSE:TRUE;                    // Adjust pin LO for polarity
}

// Functions that directly control the hardware that are not needed in all cases
// Send a command byte
//
int cm_SendCmdByte(uchar ucCommand)
{
	uchar i, ucCmd;

	i = CM_START_TRIES;

	ucCmd = (ucCommand&0x0F)|CM_PORT_CFG.ucChipSelect;
	while (i) {
		cm_Start();
		if (cm_Write(ucCmd) == 0) break;
		if (--i == 0) return FAIL_CMDSTART;
	}

	return SUCCESS;
}

// Data and Functions used by other low level functions
//
// Note: this module must be after all other low level functions in the library
//       to assure that any reference to functions in this library are satistified.
// Zone Data
uchar ucCM_UserZone;
uchar ucCM_AntiTearing;

// Chip state
uchar ucCM_Encrypt;
uchar ucCM_Authenticate;

// Global data
uchar ucCM_InsBuff[4];

// Delay
void cm_Delay(uchar ucDelay)
{
	//	uchar ucTimer;
	udelay(ucDelay*10);
	//usleep(ucDelay*10);
	/*
	   while(ucDelay) {
	   ucTimer = CM_TIMER;
	   while(ucTimer) ucTimer--;
	   ucDelay--;
	   }	
	 */
}

// Functions control the logical power on/off for the chip

// Power On Chip  
//
// Returns 0 (SUCCESS) if no error
//
void cm_PowerOn(void)   
{
	// Reset chip data
	cm_ResetCrypto();
	ucCM_UserZone = ucCM_AntiTearing = 0;

	// Sequence for powering on secure memory according to ATMEL spec
	CM_DATA_OUT;                              // SDA and SCL start as outputs
	CM_CLK_OUT;
	CM_CLK_LO;                                // Clock should start LOW
	CM_DATA_HI;                               // Data high during reset
	cm_ClockCycles(CM_PWRON_CLKS);            // Give chip some clocks cycles to get started

	// Chip should now be in sync mode and ready to operate
}

// Shut down secure memory
//
void cm_PowerOff(void)
{
	cm_Delay(1);
	CM_CLK_LO;
	cm_Delay(6);
}
// Functions that directly control the hardware

// Power On Chip  
//
// Returns 0 (SUCCESS) if no error
//
void cm_FullPowerOn(void)   
{
	// Reset chip data
	cm_ResetCrypto();
	ucCM_UserZone = ucCM_AntiTearing = 0;

	// Power control
	CM_POWER_OUT;                           // Set power pin to be an output
	if (CM_POWER_POL) CM_POWER_LO; else CM_POWER_HI;   // Turn OFF power
	CM_DIR_INIT;                              // SDA, SCL both start as outputs
	CM_CLK_LO;                                // Clock should start LOW
	CM_DATA_HI;                               // Data high during reset
	if (CM_POWER_POL) CM_POWER_HI; else CM_POWER_LO;   // Turn ON power
	cm_Delay(100);                           // Give chip a chance stabilize after power is applied

	// Sequence for powering on secure memory according to ATMEL spec
	cm_ClockCycles(CM_PWRON_CLKS);           // Give chip some clocks cycles to get started

	// Chip should now be in sync mode and ready to operate
}

// Shut down secure memory
//
void cm_FullPowerOff(void)
{
	cm_Delay(1);
	CM_CLK_LO;
	cm_Delay(6);
	if (CM_POWER_POL) CM_POWER_LO; else CM_POWER_HI;   // Turn OFF power
}
// Read Configuration Zone
//

// CryptoMemory Library Include Files
// Read Configuration Zone
int cm_ReadConfigZone(uchar ucCryptoAddr, puchar pucBuffer, uchar ucCount)
{
	uchar  ucEncrypt;
	int ret;

	ucCM_InsBuff[0] = 0xb6;
	ucCM_InsBuff[1] = 0x00;
	ucCM_InsBuff[2] = ucCryptoAddr;
	ucCM_InsBuff[3] = ucCount;

	// Three bytes of the command must be included in the polynominals
	cm_GPAcmd2(ucCM_InsBuff);

	// Do the read
	if ((ret = cm_ReadCommand(ucCM_InsBuff, pucBuffer, ucCount)) != SUCCESS) return ret;

	// Only password zone is ever encrypted
	ucEncrypt = ((ucCryptoAddr>= CM_PSW) && ucCM_Encrypt);

	// Include the data in the polynominals and decrypt if required
	cm_GPAdecrypt(ucEncrypt, pucBuffer, ucCount); 

	// Done
	return SUCCESS;
}

// Read Checksum
//

// CryptoMemory Library Include Files

uchar	ucCmdRdChk[4] = {0xb6, 0x02, 0x00, 0x02};

// Read Checksum
int cm_ReadChecksum(puchar pucChkSum)
{
	uchar ucDCR[1];
	int ret;

	// 20 0x00s (10 0x00s, ignore first byte, 5 0x00s, ignore second byte, 5 0x00s  
	cm_GPAGenN(20);

	// Read the checksum                  
	if((ret = cm_ReadCommand(ucCmdRdChk, pucChkSum, 2))!= SUCCESS) return ret;

	// Check if unlimited reads allowed
	if ((ret = cm_ReadConfigZone(DCR_ADDR, ucDCR, 1)) != SUCCESS) return ret;
	if ((ucDCR[0]&DCR_UCR)) cm_ResetCrypto();

	return SUCCESS;
}

// Read Fuze Byte
//
uchar	ucCmdRdFuze[4] = {0xb6, 0x01, 0x00, 0x01};

// Read Fuse Byte
int cm_ReadFuse(puchar pucFuze)
{
	int ret;
	// 5 0x00, A2 (0x00), 5 0x00, N (0x01)	
	cm_GPAGenNF(11, 0x01);
	if((ret = cm_ReadCommand(ucCmdRdFuze,pucFuze,1)) != SUCCESS) return ret;
	cm_GPAGen(*pucFuze);         // fuze byte
	cm_GPAGenN(5);               // 5 0x00s
	return SUCCESS;

}

// Read User Zone
//
// The Read Large User Zone function is used to read data from CryptoMemory devices
// that have greater than 256 bytes in each user zone (AT88SC6416C, and larger)

// Read User Zone
int cm_ReadLargeZone(uint uiCryptoAddr, puchar pucBuffer, uchar ucCount)
{
	int ret;

	ucCM_InsBuff[0] = 0xb2;
	ucCM_InsBuff[1] = (uchar)(uiCryptoAddr>>8);
	ucCM_InsBuff[2] = (uchar)uiCryptoAddr;
	ucCM_InsBuff[3] = ucCount;

	// Three bytes of the command must be included in the polynominals
	cm_GPAcmd3(ucCM_InsBuff);

	// Read the data
	if ((ret = cm_ReadCommand(ucCM_InsBuff, pucBuffer, ucCount)) != SUCCESS) return ret;

	// Include the data in the polynominals and decrypt if required
	cm_GPAdecrypt(ucCM_Encrypt, pucBuffer, ucCount); 

	//return SUCCESS;
	return ucCount;
}

// Read Small User Zone
//
// The Read Small User Zone function is used to read data from CryptoMemory devices that
// have 256 bytes or less in each user zone (AT88SC3216C, and smaller)

// Read Small User Zone
int cm_ReadSmallZone(uchar ucCryptoAddr, puchar pucBuffer, uchar ucCount)
{
	int ret;

	ucCM_InsBuff[0] = 0xb2;
	ucCM_InsBuff[1] = 0;
	ucCM_InsBuff[2] = ucCryptoAddr;
	ucCM_InsBuff[3] = ucCount;
	// Two bytes of the command must be included in the polynominals
	cm_GPAcmd2(ucCM_InsBuff);
	// Read the data
	if ((ret = cm_ReadCommand(ucCM_InsBuff, pucBuffer, ucCount)) != SUCCESS) return ret;
	// Include the data in the polynominals and decrypt it required
	cm_GPAdecrypt(ucCM_Encrypt, pucBuffer, ucCount); 

	return ucCount;
}

// Write User Zone
//
// The Write Large User Zone function is used to write data to CryptoMemory devices that have
// greater than 256 bytes in each user zone (AT88SC6416C, and larger).

// Write User Zone
char cm_WriteLargeZone(uint uiCryptoAddr, puchar pucBuffer, uchar ucCount)
{
	uchar ucReturn;

	ucCM_InsBuff[0] = 0xb0;
	ucCM_InsBuff[1] = (uchar)(uiCryptoAddr>>8);
	ucCM_InsBuff[2] = (uchar)uiCryptoAddr;
	ucCM_InsBuff[3] = ucCount;

	// Three bytes of the command must be included in the polynominals
	cm_GPAcmd3(ucCM_InsBuff);

	// Include the data in the polynominals and encrypt it required
	cm_GPAencrypt(ucCM_Encrypt, pucBuffer, ucCount); 

	ucReturn = cm_WriteCommand(ucCM_InsBuff, pucBuffer, ucCount);

	// when anti-tearing, the host should send ACK should >= 20ms after write command
	if (ucCM_AntiTearing) CM_LOW_LEVEL.WaitClock(10);

	// Done
	return ucReturn;
}

// Write Small User Zone
//
// The Write Small User Zone function is used to write data to CryptoMemory devices that have
// 256 bytes or less in each user zone (AT88SC3216C, and smaller)

// Write Small User Zone
int cm_WriteSmallZone(uchar ucCryptoAddr, puchar pucBuffer, uchar ucCount)
{
	int ret;

	ucCM_InsBuff[0] = 0xb0;
	ucCM_InsBuff[1] = 0x00;
	ucCM_InsBuff[2] = ucCryptoAddr;
	ucCM_InsBuff[3] = ucCount;

	// Two bytes of the command must be included in the polynominals
	cm_GPAcmd2(ucCM_InsBuff);

	// Include the data in the polynominals and encrypt it required
	cm_GPAencrypt(ucCM_Encrypt, pucBuffer, ucCount); 

	// Write the data
	ret = cm_WriteCommand(ucCM_InsBuff, pucBuffer,ucCount);

	// when anti-tearing, the host should send ACK should >= 20ms after write command
	if (ucCM_AntiTearing) CM_LOW_LEVEL.WaitClock(10);

	return ret;
}


// Mid Level Utility Function: cm_ReadCommand()
//
// Note: this module must be after all low level functions in the library and
//       before all high level user function to assure that any reference to
//       this function in this library are satistified.

int cm_ReadCommand(puchar pucInsBuff, puchar pucRetVal, uchar ucLen)
{ 
	int ret;

	if ((ret = CM_LOW_LEVEL.SendCommand(pucInsBuff)) != SUCCESS) {return ret;}
	return CM_LOW_LEVEL.ReceiveRet(pucRetVal, ucLen);
}

// Reset Password
int cm_ResetPassword(void)
{
	return CM_LOW_LEVEL.SendCmdByte(0xba);
}

// Deactivate Security
int cm_DeactiveSecurity(void)
{
	int ret;

	if ((ret = CM_LOW_LEVEL.SendCmdByte(0xb8)) != SUCCESS) return ret;
	cm_ResetCrypto();

	return SUCCESS;
}

// Low quality random number generator
void cm_RandGen(puchar pucRanddat)
{
	uchar i;

	//	srand(2);                      // need to introduce a source of entrophy
	//	for(i = 0; i < 8; i++) pucRanddat[i] = (uchar)rand();
	for(i = 0; i < 8; i++) pucRanddat[i] = (uchar)(i%6);
}

// Verify Password
//

// CryptoMemory Library Include Files

uchar ucCmdPassword[4] = {0xba, 0x00, 0x00, 0x03};
uchar ucPSW[3];
// Verify Password
int cm_VerifyPasswordx(puchar pucPassword, uchar ucSet, uchar ucRW)
{
	int i, j;
	int  ret;
	uchar ucAddr;

	// Build command and PAC address
	ucAddr = CM_PSW + (ucSet<<3);
	ucCmdPassword[1] = ucSet;
	if (ucRW != CM_PWWRITE) {
		ucCmdPassword[1] |= 0x10;
		ucAddr += 4;
	}

	// Deal with encryption if in authenticate mode
	for (j = 0; j<3; j++)
	{
		// Encrypt the password
		//if(ucCM_Authenticate)
		{
			for(i = 0; i<5; i++) cm_GPAGen(pucPassword[j]);
			ucPSW[j] = Gpa_byte;
		}
		// Else just copy it
		//ucPSW[j] = pucPassword[j];
	}

	// Send the command
	ret = cm_WriteCommand(ucCmdPassword, ucPSW, 3);

	// Wait for chip to process password
	CM_LOW_LEVEL.WaitClock(3);

	// Read Password attempts counter to determine if the password was accepted
	if (ret == 3)
	{
		ret = cm_ReadConfigZone(ucAddr, ucPSW, 1);
		if (ucPSW[0]!= 0xFF) ret = FAILED;
	}
	if (ucCM_Authenticate && (ret!= SUCCESS)) 
	{
		cm_ResetCrypto();
	}

	// Done
	if(ret ==1 )
		return 0;
	else
		return ret;
}
// Verify Password
int cm_VerifyPassword(puchar pucPassword, uchar ucSet, uchar ucRW)
{
	int i, j;
	int  ret;
	uchar ucAddr;

	// Build command and PAC address
	ucAddr = CM_PSW + (ucSet<<3);
	ucCmdPassword[1] = ucSet;
	if (ucRW != CM_PWWRITE) {
		ucCmdPassword[1] |= 0x10;
		ucAddr += 4;
	}

	// Deal with encryption if in authenticate mode
	for (j = 0; j<3; j++)
	{
		// Encrypt the password
		if(ucCM_Authenticate)
		{
			for(i = 0; i<5; i++) cm_GPAGen(pucPassword[j]);
			ucPSW[j] = Gpa_byte;
		}
		// Else just copy it
		ucPSW[j] = pucPassword[j];
	}

	// Send the command
	ret = cm_WriteCommand(ucCmdPassword, ucPSW, 3);

	// Wait for chip to process password
	CM_LOW_LEVEL.WaitClock(3);

	// Read Password attempts counter to determine if the password was accepted
	if (ret == 3)
	{
		ret = cm_ReadConfigZone(ucAddr, ucPSW, 1);
		if (ucPSW[0]!= 0xFF) ret = FAILED;
	}
	if (ucCM_Authenticate && (ret!= SUCCESS)) 
	{
		cm_ResetCrypto();
	}

	// Done
	if(ret ==1 )
		return 0;
	else
		return ret;
}

// Write Configuration Zone
// Write Configuration Zone
int cm_WriteConfigZone(uchar ucCryptoAddr, puchar pucBuffer, uchar ucCount, uchar ucAntiTearing)
{
	uchar  ucEncrypt;
	int ret;

	ucCM_InsBuff[0] = 0xb4;
	if(ucAntiTearing) ucCM_InsBuff[1] = 0x08;
	else              ucCM_InsBuff[1] = 0x00;
	ucCM_InsBuff[2] = ucCryptoAddr;
	ucCM_InsBuff[3] = ucCount;

	// Three bytes of the command must be included in the polynominals
	cm_GPAcmd2(ucCM_InsBuff);

	// Only password zone is ever encrypted
	ucEncrypt = ((ucCryptoAddr>= CM_PSW) && ucCM_Encrypt);

	// Include the data in the polynominals and encrypt if required
	cm_GPAencrypt(ucEncrypt, pucBuffer, ucCount); 

	// Do the write
	ret = cm_WriteCommand(ucCM_InsBuff, pucBuffer,ucCount);

	// when anti-tearing, the host should send ACK should >= 20ms after write command
	if (ucAntiTearing) CM_LOW_LEVEL.WaitClock(10);

	return ret;
}

// Write Checksum
uchar	ucCmdWrChk[4] = {0xb4, 0x02, 0x00, 0x02};

// Send Checksum
int cm_SendChecksum(puchar pucChkSum)
{
	int ret;
	uchar ucChkSum[2];

	// Get Checksum if required
	if(pucChkSum == NULL) cm_CalChecksum(ucChkSum);
	else {
		ucChkSum[0] = *pucChkSum++; 
		ucChkSum[1] = *pucChkSum; 
	} 

	// Send the command
	ret = cm_WriteCommand(ucCmdWrChk, ucChkSum, 2);

	// Give the CyrptoMemory some processing time
	CM_LOW_LEVEL.WaitClock(5);

	// Done
	if(ret==2)
	{
		return SUCCESS;
	}
	else
	{
		return ret;
	}
}

// Mid Level Utility Function: cm_WriteCommand()
//
// Note: this module must be after all low level functions in the library and
//       before all high level user function to assure that any reference to
//       this function in this library are satistified.

int cm_WriteCommand(puchar pucInsBuff, puchar pucSendVal, uchar ucLen)
{ 
	int ret;

	if ((ret = CM_LOW_LEVEL.SendCommand(pucInsBuff)) != SUCCESS) return ret;
	return CM_LOW_LEVEL.SendData(pucSendVal, ucLen);
}

// Encryption Functions
//
// Note: the naming conventions in this module do not match those used in all other modules. This
//       is because the name used in this module are intended to be as close to those used in the
//       Atmel documentation to make verification of these functions simpler.

// -------------------------------------------------------------------------------------------------
// Data
// -------------------------------------------------------------------------------------------------

uchar ucGpaRegisters[Gpa_Regs];

// -------------------------------------------------------------------------------------------------
// Functions
// -------------------------------------------------------------------------------------------------

// Reset the cryptographic state
void cm_ResetCrypto(void)
{
	uchar i;

	for (i = 0; i < Gpa_Regs; ++i) ucGpaRegisters[i] = 0;
	ucCM_Encrypt = ucCM_Authenticate = FALSE;
}

// Generate next value
uchar cm_GPAGen(uchar Datain)
{
	uchar Din_gpa;
	uchar Ri, Si, Ti;
	uchar R_sum, S_sum, T_sum;

	// Input Character
	Din_gpa = Datain^Gpa_byte;
	Ri = Din_gpa&0x1f;   			                //Ri[4:0] = Din_gpa[4:0]
	Si = ((Din_gpa<<3)&0x78)|((Din_gpa>>5)&0x07);   //Si[6:0] = {Din_gpa[3:0], Din_gpa[7:5]}
Ti = (Din_gpa>>3)&0x1f;  		                //Ti[4:0] = Din_gpa[7:3];

//R polynomial
R_sum = cm_Mod(RD, cm_RotR(RG), CM_MOD_R);
RG = RF;
RF = RE;
RE = RD;
RD = RC^Ri;
RC = RB;
RB = RA;
RA = R_sum;

//S ploynomial
S_sum = cm_Mod(SF, cm_RotS(SG), CM_MOD_S);
SG = SF;
SF = SE^Si;
SE = SD;
SD = SC;
SC = SB;
SB = SA;
SA = S_sum;

//T polynomial
T_sum = cm_Mod(TE,TC,CM_MOD_T);
TE = TD;
TD = TC;
TC = TB^Ti;
TB = TA;
TA = T_sum;

// Output Stage
Gpa_byte =(Gpa_byte<<4)&0xF0;                                  // shift gpa_byte left by 4
Gpa_byte |= ((((RA^RE)&0x1F)&(~SA))|(((TA^TD)&0x1F)&SA))&0x0F; // concat 4 prev bits and 4 new bits
return Gpa_byte;
}

// Do authenticate/encrypt chalange encryption
void cm_AuthenEncryptCal(uchar *Ci, uchar *G_Sk, uchar *Q, uchar *Ch)
{	
	uchar i, j;

	// Reset all registers
	cm_ResetCrypto();

	// Setup the cyptographic registers
	for(j = 0; j < 4; j++) {
		for(i = 0; i<3; i++) cm_GPAGen(Ci[2*j]);	
		for(i = 0; i<3; i++) cm_GPAGen(Ci[2*j+1]);
		cm_GPAGen(Q[j]);
	}

	for(j = 0; j<4; j++ ) {
		for(i = 0; i<3; i++) cm_GPAGen(G_Sk[2*j]);
		for(i = 0; i<3; i++) cm_GPAGen(G_Sk[2*j+1]);
		cm_GPAGen(Q[j+4]);
	}

	// begin to generate Ch
	cm_GPAGenN(6);                    // 6 0x00s
	Ch[0] = Gpa_byte;

	for (j = 1; j<8; j++) {
		cm_GPAGenN(7);                // 7 0x00s
		Ch[j] = Gpa_byte;
	}

	// then calculate new Ci and Sk, to compare with the new Ci and Sk read from eeprom
	Ci[0] = 0xff;		              // reset AAC 
	for(j = 1; j<8; j++) {
		cm_GPAGenN(2);                // 2 0x00s
		Ci[j] = Gpa_byte;
	}

	for(j = 0; j<8; j++) {
		cm_GPAGenN(2);                // 2 0x00s
		G_Sk[j] = Gpa_byte;
	}

	cm_GPAGenN(3);                    // 3 0x00s
}

// Calaculate Checksum
void cm_CalChecksum(uchar *Ck_sum)
{
	cm_GPAGenN(15);                    // 15 0x00s
	Ck_sum[0] = Gpa_byte;
	cm_GPAGenN(5);                     // 5 0x00s
	Ck_sum[1] = Gpa_byte;	
}

// The following functions are "macros" for commonly done actions

// Clock some zeros into the state machine
void cm_GPAGenN(uchar Count)
{
	while(Count--) cm_GPAGen(0x00);
}

// Clock some zeros into the state machine, then clock in a byte of data
void cm_GPAGenNF(uchar Count, uchar DataIn)
{
	cm_GPAGenN(Count);                             // First ones are allways zeros
	cm_GPAGen(DataIn);                             // Final one is sometimes different
}

// Include 2 bytes of a command into a polynominal
void cm_GPAcmd2(puchar pucInsBuff)
{
	cm_GPAGenNF(5, pucInsBuff[2]);
	cm_GPAGenNF(5, pucInsBuff[3]);
}

// Include 3 bytes of a command into a polynominal
void cm_GPAcmd3(puchar pucInsBuff)
{
	cm_GPAGenNF(5, pucInsBuff[1]);
	cm_GPAcmd2(pucInsBuff);
}

// Include the data in the polynominals and decrypt it required
void cm_GPAdecrypt(uchar ucEncrypt, puchar pucBuffer, uchar ucCount)
{
	uchar i;

	for (i = 0; i < ucCount; ++i) {
		if (ucEncrypt) pucBuffer[i] = pucBuffer[i]^Gpa_byte;
		cm_GPAGen(pucBuffer[i]);
		cm_GPAGenN(5);        // 5 clocks with 0x00 data
	}
}

// Include the data in the polynominals and encrypt it required
void cm_GPAencrypt(uchar ucEncrypt, puchar pucBuffer, uchar ucCount)
{
	uchar i, ucData; 

	for (i = 0; i<ucCount; i++) {
		cm_GPAGenN(5);                          // 5 0x00s
		ucData = pucBuffer[i];
		if (ucEncrypt) pucBuffer[i] = pucBuffer[i]^Gpa_byte;
		cm_GPAGen(ucData);
	}
}


void print_buf(char* buf, int len){
	int i = 0;
	for(i=0; i<len; i++){
		printk(KERN_ALERT "%#x ", buf[i]);
	}
	printk(KERN_ALERT "\n");
}
static int at88_open(struct inode *inode,struct file *filp)
{

	//power on the chip
	cm_PowerOn();
	return 0;
}
static int at88_release(struct inode *inode,struct file *filp)
{
	//power off the chip
	cm_PowerOff();
	return 0;
}

static ssize_t at88_read (struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	//return 0;
	int ret=-1;
	int i;
	//uchar ucG[8];
	//uchar psw[4];
	int tmp_count;
	int ret_cout=0;

	if(block_num<0 || block_num>3) return -EINVAL;
	
	tmp_count=count;
	//for(i=0;i<4;i++)
	i=block_num;
	{
		ret = cm_ActiveSecurity((uchar)(GC_INDEX[i]), (puchar)(GC_TABLE[GC_INDEX[i]]), NULL, TRUE);
		if (ret != SUCCESS) 
		{
			printk("activate authentication failed!\n");
			return ret;
		}
		ret = cm_VerifyPasswordx((puchar)(PW_WRITE_TABLE[PW_INDEX[i]]), (uchar)(PW_INDEX[i]), CM_PWWRITE);
		if (ret != SUCCESS) 
		{
			printk("cm_VerifyPassword failed! \n");
			return ret;
		}
		ret = cm_SetUserZone(i, FALSE);
		if(ret != SUCCESS) 
		{
			printk("cm_SetUserZone failed!\n");
				return -1;
		}
		if(tmp_count<=16)
		{
			ret = cm_ReadSmallZone(0, buf+ret_cout, count);
			if(ret < 0)
			{
				printk("1cm_ReadSmallZone failed! ret=%d\n",ret);
				return ret;
			}
			return ret_cout+ret;
		}
		else if(tmp_count<=32)
		{
			ret = cm_ReadSmallZone(0, buf+ret_cout, 16);
			if(ret < 0)
			{
				printk("2cm_ReadSmallZone failed! ret=%d\n",ret);
				return ret;
			}

			tmp_count-=16;
			ret_cout+=16;
			ret = cm_ReadSmallZone(16, buf+ret_cout, tmp_count);
			if(ret < 0)
			{
				printk("1cm_ReadSmallZone failed! ret=%d\n",ret);
				return ret;
			}
			return ret_cout+ret;
		}
		else
		{
			ret = cm_ReadSmallZone(0, buf+ret_cout, 16);
			if(ret < 0)
			{
				printk("2cm_ReadSmallZone failed! ret=%d\n",ret);
				return ret;
			}
			tmp_count-=16;
			ret_cout+=16;
			ret = cm_ReadSmallZone(16, buf+ret_cout, 16);
			if(ret < 0)
			{
				printk("2cm_ReadSmallZone failed! ret=%d\n",ret);
				return ret;
			}
			tmp_count-=16;
			ret_cout+=16;
		}
	}

	return ret_cout;
}

static ssize_t at88_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	ssize_t  ret;
	int tmp_count=count;
	int ret_cout=0;
	int i;
	
	if(block_num<0 || block_num>3) return -EINVAL;
	
	ret = cm_SetUserZone(0, FALSE);
	if(ret != SUCCESS) 
	{
		printk("cm_SetUserZone failed!\n");
		return ret;
	}
	//for(i=0;i<4;i++)
	i=block_num;
	{
		ret = cm_ActiveSecurity((uchar)(GC_INDEX[i]), (puchar)(GC_TABLE[GC_INDEX[i]]), NULL, TRUE);
		if (ret != SUCCESS) 
		{
			printk("activate authentication failed!\n");
			return ret;
		}
		ret = cm_VerifyPasswordx((puchar)(PW_WRITE_TABLE[PW_INDEX[i]]), (uchar)(PW_INDEX[i]), CM_PWWRITE);
		if (ret != SUCCESS) 
		{
			printk("cm_VerifyPassword failed! \n");
			return ret;
		}
		ret = cm_SetUserZone(i, FALSE);
		if(ret != SUCCESS) 
		{
			printk("cm_SetUserZone failed!\n");
				return -1;
		}
		if(tmp_count<=16)
		{
			ret = cm_WriteSmallZone(0, (puchar)(buf+ret_cout), (uchar)tmp_count);
			if(ret < 0)
			{
				printk("1cm_ReadSmallZone failed! ret=%d\n",ret);
				return ret;
			}
			ret = cm_SendChecksum(NULL);
			if (ret != SUCCESS) 
				return ret;
			return ret_cout+ret;
		}
		else if(tmp_count<=32)
		{
			ret = cm_WriteSmallZone(0, (puchar)(buf+ret_cout), 16);
			if(ret < 0)
			{
				printk("2cm_ReadSmallZone failed! ret=%d\n",ret);
				return ret;
			}
			ret = cm_SendChecksum(NULL);
			if (ret != SUCCESS) 
				return ret;
			tmp_count-=16;
			ret_cout+=16;
			ret = cm_WriteSmallZone(16, (puchar)(buf+ret_cout), (uchar)tmp_count);
			if(ret < 0)
			{
				printk("1cm_ReadSmallZone failed! ret=%d\n",ret);
				return ret;
			}
			ret = cm_SendChecksum(NULL);
			if (ret != SUCCESS) 
				return ret;
			return ret_cout+ret;
		}
		else
		{
			ret = cm_WriteSmallZone(0, (puchar)(buf+ret_cout), 16);
			if(ret < 0)
			{
				printk("2cm_ReadSmallZone failed! ret=%d\n",ret);
				return ret;
			}
			ret = cm_SendChecksum(NULL);
			if (ret != SUCCESS) 
				return ret;
			tmp_count-=16;
			ret_cout+=16;
			ret = cm_WriteSmallZone(16, (puchar)(buf+ret_cout), 16);
			if(ret < 0)
			{
				printk("2cm_ReadSmallZone failed! ret=%d\n",ret);
				return ret;
			}
			ret = cm_SendChecksum(NULL);
			if (ret != SUCCESS) 
				return ret;
			tmp_count-=16;
			ret_cout+=16;
		}
	}
	return ret_cout;
}


long at88_ioctl(struct file *fd, unsigned int cmd, unsigned long arg)
{

#if 0
	int ret = 0;
	uchar ucReturn = 0;
	char buf[64] = {};
	printk(KERN_ALERT "cmd:%d, buf:", cmd);
	print_buf(buf, sizeof(buf));
	switch(cmd)
	{
		case AT88_SET_PASSWD:
			break;
		case AT88_GET_MFandLOT_INFO:
			//GET menufacture's and lot's code
			//menufacture and lot codes 16 + 64 bytes in total,  at88sc104 -> |0x3b 0xb2| + |0x11 0x0 0x10 0x80 0x0 0x1 0x10 0x10|
			memset(buf, 0, sizeof(buf));
			ucReturn = cm_ReadConfigZone(0x00, buf, 10);
			printk(KERN_ALERT "0x00readl(10)ucReturn:%#x, ch:", ucReturn);
			print_buf(buf, 10);
			if(copy_to_user((char*)arg, buf, 10) > 0){
				printk(KERN_ERR "copy to user get something wrong!\n");
				ret = -EAGAIN;
			}
			break;
		case AT88_BURN_FUSE:
			break;
		case AT88_SET_BLK:
			if((arg>=0)&&(arg<4))
			{
				block_num=arg;
				return 0;
			}
			else
			{
				return -1;
			}
		default:
			return 0;
	};
	return ret;
#endif

#if 1
	int ret = 0;
	uchar ucReturn = 0;
	char tmpBuf[USR_ZONE_MAX_LENGTH] = {};
	at88sc_ioctl_arg_t kmsg;

	memset(&kmsg, 0, sizeof(at88sc_ioctl_arg_t));
	if(copy_from_user(&kmsg, (at88sc_ioctl_arg_t*)arg, sizeof(at88sc_ioctl_arg_t)))
	{
		printk(KERN_ERR "copy from user get something wrong!\n");
		ret = -EAGAIN;

	}
	//test
	printk(KERN_ALERT "kmsg.buf:");
	print_buf(kmsg.buf, 32);
	
	if(strncmp("General", kmsg.name, strlen("General")))
	{
		printk("Error: at88_ioctl EINVAL.");
		return -EINVAL;
	}

	if(AT88_GET_USER_ZONE == cmd)
	{
		if(strncmp("General gets userZone", kmsg.name, strlen("General gets userZone")))
		{
			printk("Error: at88_ioctl EINVAL.");
			return -EINVAL;
		}
	}

switch_continue_loop:
	switch(cmd)
	{
	case AT88_SET_PASSWD:
		break;
	case AT88_BURN_FUSE:
		break;
	case AT88_SET_BLK:
	{
		if((kmsg.usrZoneNum>=0)&&(kmsg.usrZoneNum<4))
		{
			block_num = kmsg.usrZoneNum;
			return 0;
		}
		else
		{
			return -1;
		}
		break;
	}
	
	case AT88_GET_MFandLOT_INFO:{
		
		if(strcmp("General gets mfAndLot", kmsg.name)) break;
		
		//GET menufacture's and lot's code
		//menufacture and lot codes 16 + 64 bytes in total,  at88sc104 -> |0x3b 0xb2| + |0x11 0x0 0x10 0x80 0x0 0x1 0x10 0x10|
		memset(kmsg.buf, 0, sizeof(kmsg.buf));
		ucReturn = cm_ReadConfigZone(0x00, kmsg.buf, 10);
		printk(KERN_ALERT "0x00readl(10)ucReturn:%#x, ch:", ucReturn);
		print_buf(kmsg.buf, 10);
		if(copy_to_user((at88sc_ioctl_arg_t*)arg, &kmsg, sizeof(at88sc_ioctl_arg_t)) > 0){
			printk(KERN_ERR "copy to user get something wrong!\n");
			ret = -EAGAIN;
		}
		break;
	}
	case AT88_GET_SERIAL_NUM:
	{
		if(strcmp("General gets serialNum", kmsg.name)) break;
		else
		{
			cmd = AT88_GET_USER_ZONE;
			printk(KERN_ALERT "General gets serialNum\n");
			goto switch_continue_loop;
		}
	}
	case AT88_GET_P2P_ID:
	{
		if(strcmp("General gets p2pId", kmsg.name)) break;
		else
		{
			cmd = AT88_GET_USER_ZONE;
			goto switch_continue_loop;
		}
	}
	case AT88_GET_LICENSE:
	{
		if(strcmp("General gets license", kmsg.name)) break;
		else
		{
			cmd = AT88_GET_USER_ZONE;
			goto switch_continue_loop;
		}
	}
	case AT88_GET_MAC:
	{
		if(strcmp("General gets mac", kmsg.name)) break;
		else
		{
			cmd = AT88_GET_USER_ZONE;
			goto switch_continue_loop;
		}
	}
	case AT88_GET_USER_ZONE:
	{
 		if((kmsg.usrZoneNum >= 0) && (kmsg.usrZoneNum < 4)) block_num = kmsg.usrZoneNum;
		
		if(kmsg.length < 0) kmsg.length = 0;
		if(kmsg.length > USR_ZONE_MAX_LENGTH) kmsg.length = USR_ZONE_MAX_LENGTH;
		if(kmsg.length + kmsg.index > USR_ZONE_MAX_LENGTH)
		{
			kmsg.index = 0;
			kmsg.length = USR_ZONE_MAX_LENGTH;
		}
		memset(tmpBuf, 0, sizeof(tmpBuf));
		memset(kmsg.buf, 0, sizeof(kmsg.buf));
		at88_read(NULL, tmpBuf, USR_ZONE_MAX_LENGTH, NULL);
		memcpy(kmsg.buf, tmpBuf + kmsg.index, kmsg.length);
		if(copy_to_user((at88sc_ioctl_arg_t*)arg, &kmsg, sizeof(at88sc_ioctl_arg_t)) > 0)
		{
			printk(KERN_ERR "copy to user get something wrong!\n");
			ret = -EAGAIN;
		}
		break;
	}
	case AT88_SET_SERIAL_NUM:
	{
		if(strcmp("General sets serialNum", kmsg.name)) break;
		else
		{
			cmd = AT88_SET_USER_ZONE;
			printk(KERN_ALERT "General sets serialNum\n");
			goto switch_continue_loop;
		}
	}
	case AT88_SET_P2P_ID:
	{
		if(strcmp("General sets p2pId", kmsg.name)) break;
		else
		{
			cmd = AT88_SET_USER_ZONE;
			goto switch_continue_loop;
		}
	}
	case AT88_SET_LICENSE:
	{
		if(strcmp("General sets license", kmsg.name)) break;
		else
		{
			cmd = AT88_SET_USER_ZONE;
			goto switch_continue_loop;
		}
	}
	case AT88_SET_MAC:
	{
		if(strcmp("General sets mac", kmsg.name)) break;
		else
		{
			cmd = AT88_SET_USER_ZONE;
			goto switch_continue_loop;
		}
	}
	case AT88_SET_USER_ZONE:
	{
 		if((kmsg.usrZoneNum >= 0) && (kmsg.usrZoneNum < 4)) block_num = kmsg.usrZoneNum;
		if(kmsg.length < 0) kmsg.length = 0;
		if(kmsg.length > 32) kmsg.length = 32;
		if(kmsg.length + kmsg.index > USR_ZONE_MAX_LENGTH)
		{
			kmsg.index = 0;
			kmsg.length = USR_ZONE_MAX_LENGTH;
		}
		memset(tmpBuf, 0, sizeof(tmpBuf));
		at88_read(NULL, tmpBuf, USR_ZONE_MAX_LENGTH, NULL);
		memcpy(tmpBuf + kmsg.index, kmsg.buf, kmsg.length);
		at88_write(NULL, tmpBuf, USR_ZONE_MAX_LENGTH, NULL);
#if 1
		memset(tmpBuf, 0, sizeof(tmpBuf));
		at88_read(NULL, tmpBuf, USR_ZONE_MAX_LENGTH, NULL);
		memset(kmsg.buf, 0, sizeof(kmsg.buf));
		memcpy(kmsg.buf, tmpBuf + kmsg.index, kmsg.length);
		if(copy_to_user((at88sc_ioctl_arg_t*)arg, &kmsg, sizeof(at88sc_ioctl_arg_t)) > 0)
		{
			printk(KERN_ERR "copy to user get something wrong!\n");
			ret = -EAGAIN;
		}
#endif
		break;
	}
	default:
		break;
	};
	return ret;

#endif
}

static struct file_operations at88_fops = {
	.open			= at88_open,
	.release		= at88_release,	
	.read			= at88_read,
	.write			= at88_write,
	.unlocked_ioctl	= at88_ioctl,
};

static struct miscdevice at88_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = AT88SC104_DEVICE_NODE_NAME,
	.fops = &at88_fops,
};

static int __init davinci_at88_init(void)
{
	int ret=-1;	

	/*PL4->scl  ,  PL5->sda*/
	regdat = ioremap(0x01f02c10, 4);//pl_data_reg
	if(!regdat){
		printk(KERN_ERR "Error:ioremap for regdat failed!\n");
		ret = -ENOMEM;
		goto err_regdat_ioremap;
	}
	regdir = ioremap(0x01f02c00, 4);//pl_cfg0_reg
	if(!regdir){
		printk(KERN_ERR "Error:ioremap for regdir failed!\n");
		ret = -ENOMEM;
		goto err_regdir_ioremap;
	}

	ret = misc_register(&at88_misc);
	if(ret != 0){
		printk(KERN_ERR "Error:%s(%d) failed.", __func__, __LINE__);
		goto err_misc_register;
	}



	return 0;

err_misc_register:
	iounmap(regdir);
err_regdir_ioremap:
	iounmap(regdat);
err_regdat_ioremap:
	return ret;

}

static void __exit davinci_at88_exit(void)
{

	misc_deregister(&at88_misc);
	iounmap(regdir);
	iounmap(regdat);
	printk("davinci at88 release success.\n");


}
module_init(davinci_at88_init);
module_exit(davinci_at88_exit);
MODULE_AUTHOR("Henry <haoxiansen@zhitongits.com.cn>");
MODULE_DESCRIPTION("AT88SC104c driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("Nothing");

