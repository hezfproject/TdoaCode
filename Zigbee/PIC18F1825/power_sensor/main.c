#include <htc.h>
#include <pic.h> 										// include standard header file
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "PIC1825_Type.h"
#include "config.h"
#include "uartprotocol.h"
#include "..\..\..\common\MBusProto.h"



#include "crc.h"

// 熔丝配置参数

//__CONFIG(INTIO & WDTDIS & PWRTEN & MCLRDIS & UNPROTECT & BORXSLP & IESODIS & FCMDIS & WAKECNT);
//__CONFIG(INTIO & WDTDIS & PWRTDIS & MCLRDIS & UNPROTECT & BORDIS & IESODIS & FCMDIS & WAKECNT);
//__CONFIG(LP & WDTDIS & PWRTDIS & MCLRDIS & UNPROTECT & BORDIS & IESODIS & FCMDIS & WAKECNT);

#define _XTAL_FREQ 500000                           //Used by the XC8 delay_ms(x) macro
#define MBUS_VERSION (01)             // 0-63
#define RELEASE_VER  (01)             //版本号

__CONFIG ((FOSC_INTOSC  & WDTE_NSLEEP &  PWRTE_OFF & MCLRE_ON & CP_OFF & CPD_OFF & BOREN_ON & CLKOUTEN_OFF & IESO_OFF &  FCMEN_OFF));
__CONFIG ((WRT_OFF & PLLEN_OFF & STVREN_OFF & BORV_25 & LVP_OFF));
/****************************************************************/




#define EN_MOS	RA0=1		//打开PMOS
#define DE_MOS	RA0=0		//关闭PMOS

static bit	super_current;
static bit	super_voltage;
uint8 TX_flag = 0;

uint16 AD_Ivalue,AD_Vvalue;

enum
{
	RX_LISTEN_STATE = 0,
    RX_CMD_STATE = 1,
    RX_ADDRESS_L_STATE = 2,
	RX_ADDRESS_H_STATE = 3,
    RX_DATA_LEN_H_STATE = 4,
    RX_DATA_LEN_L_STATE = 5,
    RX_CRC_L_STATE = 6,
    RX_CRC_H_STATE = 7,
};


uint8 RxBusState = RX_LISTEN_STATE;
uint8 fram_control;
uint8 DeviceID;

static mbus_hdr_mstr_t *mbus_hdr_mstr_p,mbus_hdr_mstr;
static uartprotocol_hdr_slv_t_Temp *mbus_hdr_slv_p,mbus_hdr_slv;
volatile unsigned char uart_data;   							// use 'volatile' qualifer as this is changed in ISR

void ParseRx(unsigned char data);


void Cpu_Init()
{
	OSCCON = INTER16MHZCLK;									//16M
	//OSCTUNE= 0B00011111;								//最高频率
	CLKRCON = DUTYCYCLE50PERCENT;
	OPTION_REG = 0x0;
	
}

void Hal_Uart_Init(void)
{
	TRISC = 0x80;

    TXSTA = TXEN_ENABLE;		// select low speed Baud Rate (see baud rate calcs below)/select 8 data bits/enable transmit/asynchronization
    RCSTA = (SPEN_ENABLE	| CREN_ENABLE | RX9_DISABLE);	//serial port is enabled/select 8 data bits/receive enabled

    BRGH = HIGH_BAUD_RATE_ENABLE;	//high baud rate


    SPBRGL=8;  // here is calculated value of SPBRGH and SPBRGL 115200(16mhz/(16*115200)-1)
    SPBRGH=0;

    PIR1 &= (~0x20);            // make sure receive interrupt flag is clear
    PIE1 |= RECEIVE_INTERRUPT_ENABLE;
    INTCON |= (GIE_ENABLE | PEIE_ENABLE);    // Enable peripheral interrupt and global interrupt
}

void Hal_Io_Init(void)
{
	TRISC |= (RC0_AS_INPUT | RC1_AS_INPUT); 
	WPUA &= (RA0_PULL_UP_DISABLE&RA1_PULL_UP_DISABLE);		//RA0/RA1不上拉
}
void Adc_Itext_Init()										//初始化ADC和比较器
{
    ANSELA |= RA2_ANALOG_INPUT;  								//数字IO  AN2模拟
    TRISA |= RA2_AS_INPUT;   									//      AN2输入

	ADCON0 |= (ADC_SLECT_AN2 | ADC_ENABLE);							//通道2，未启动，使能ADC

	ADCON1 |= ADC_FOSC_8;										//left justified - FOSC/8 speed - Vref is Vdd

	//uart_str("adc_init\r\n");
}
void Wdg_Init(void)
{
	WDTCON = 0x14;//1s
}

void Adc_Vtext_Init()									//初始化ADC和比较器
{
    ANSELC |= RC2_ANALOG_INPUT;  								//数字IO  AN6模拟
    TRISC |= RC2_AS_INPUT;   											//      AN6输入

	ADCON0 |= (ADC_SLECT_AN6 | ADC_ENABLE);										//通道6，未启动，使能ADC
	ADCON1 |= ADC_FOSC_8;
    ADFM = ADC_ALIGN_RIGHT;
	//uart_str("adc_init\r\n");
}

void delay_us(uint32 time)
{
	uint32 x,y;
	for(y=0;y<time;y++)
	{
		for(x=0;x<16;x++);
	}
}

void Read_Io_State(void)
{

	mbus_hdr_slv.uartprotocol_tlv.power_information__packed.ATEXT = LATC0;

	mbus_hdr_slv.uartprotocol_tlv.power_information__packed.BTEXT = LATC1;

}



void Uart_Str(uint8 *P)
{
    while(*P != '\0')
    {
        TXREG=*P;
        P++;
        while(!TXIF);
    }
}

void Uart_Char(uint8  P)
{
	TXREG=P;
	while(!TXIF);
}

void Uart_Tx(uint8 *P,uint8 length) 
{
    //while(!TXSTAbits.TRMT);    // make sure buffer full bit is high before transmitting
    while(length)
    {
        TXREG=*P;
        P++;
		length--;
        while(!TXIF);
    }
}



uint16 Adc_Read(void)
{
	uint16 AD_value;
	
	GO = ADC_START;
	while(ADC_STOP);

	AD_value = ADRESH;
    AD_value &= 0x0003;
    AD_value = (AD_value<<8)+ADRESL;

	return	AD_value;
}





/****************************************************************************
 *
 * NAME:  Send_Pack
 *
 * DESCRIPTION:
 *
 *
 *
 * RETURNS:
 * Never returns.
 *
 ****************************************************************************/
void Send_Pack (uint8 fram_control)
{
    uint16 count;
    uint16 crc_send_clr =0x0000;
    uint16 crc_send_qry =0x0000;

    mbus_hdr_slv.sync[0] = 'Y';
    mbus_hdr_slv.sync[1] = 'I';
    mbus_hdr_slv.sync[2] = 'R';
    mbus_hdr_slv.sync[3] = 'I';
    mbus_hdr_slv.frame_control = fram_control;                                      //version 01 Data_flag=1
    mbus_hdr_slv.cmd = MBUS_CMD_RSP;
    mbus_hdr_slv.slv_id = DeviceID;

    mbus_hdr_slv.uartprotocol_tlv.type =0x23; //MBUS_TLV_SENSOR_READER_V2;
    mbus_hdr_slv.uartprotocol_tlv.len =sizeof(mbus_hdr_slv.uartprotocol_tlv) - 3;//不包括 type 和 len 的byte?

    if (mbus_hdr_mstr_p->cmd == MBUS_CMD_CLR)
    {
        mbus_hdr_slv.data_len = 0x0000;
        crc_send_clr = CRC16((uint8 *)mbus_hdr_slv_p, 10,0xFFFF);              //sync[0]到slv_id 共10个byte
		Uart_Tx((uint8 *)mbus_hdr_slv_p,10);
		Uart_Tx((uint8 *)&crc_send_clr,2);
    }
    if (mbus_hdr_mstr_p->cmd == MBUS_CMD_QRY)
    {
        mbus_hdr_slv.data_len = sizeof(mbus_hdr_slv.uartprotocol_tlv);
        crc_send_qry = CRC16((uint8 *)mbus_hdr_slv_p, sizeof(uartprotocol_hdr_slv_t_Temp),0xFFFF) ; //
        Uart_Tx((uint8 *)mbus_hdr_slv_p,sizeof(uartprotocol_hdr_slv_t_Temp));
		Uart_Tx((uint8 *)&crc_send_qry,2);
    }
}




void ParseRx(uint8 data)
{
    static uint16 u16rx_slv_id = 0; //first send low byte or high byte ??
    static uint16 u16rx_crc = 0;    //first send low byte or high byte ??
   	static uint16 u16highAddr = 0;
	static	uint16 u16lowAddr = 0;
	uint16 u16highcrc = 0;
    //static uint8 fram_control = 0;
    //uint32 DIO_status,*p;
    //uint8  DIO_flag;
    switch (RxBusState)
    {
    case RX_LISTEN_STATE:
        if (MBUS_GET_SLAVE_VERSION(data) == MBUS_VERSION)
        {
            fram_control = data;
            mbus_hdr_mstr_p->frame_control = data;
            RxBusState = RX_CMD_STATE;
        }
        break;
    case RX_CMD_STATE:
        if ((data == MBUS_CMD_CLR)||(data == MBUS_CMD_QRY))
        {
            mbus_hdr_mstr_p->cmd = data;
            RxBusState = RX_ADDRESS_L_STATE;
        }
        else
        {
            RxBusState = RX_LISTEN_STATE;
        }
        break;
    case RX_ADDRESS_L_STATE:
		//u16lowAddr = data;
         u16rx_slv_id &=0x0000;
         u16rx_slv_id += data;
 		 u16rx_slv_id &= 0xff;
         RxBusState = RX_ADDRESS_H_STATE;
        break;
    case RX_ADDRESS_H_STATE:
		u16highAddr = data;
		u16highAddr &= 0xff;
		u16rx_slv_id |= (u16highAddr << 8);
        if ((uint8)u16rx_slv_id == DeviceID)
        {
            mbus_hdr_mstr_p->slv_id = u16rx_slv_id;
            RxBusState = RX_CRC_L_STATE;
        }
        else
        {
            RxBusState = RX_LISTEN_STATE;
        }
        break;
    case RX_CRC_L_STATE :
        u16rx_crc &=0x0000;
        u16rx_crc += data;
        RxBusState = RX_CRC_H_STATE;
        break;
    case RX_CRC_H_STATE :
		u16highcrc = data;
		u16highcrc &= 0xff;
		u16rx_crc |= (u16highcrc << 8);

        if( CRC16((uint8 *)mbus_hdr_mstr_p,4,0xFFFF) == u16rx_crc)
        {
			Send_Pack(fram_control);
        }
        else
        {
            ;
        }
        RxBusState = RX_LISTEN_STATE;
        break;
    default:
        break;
    }
}


void ReadAddr_Form_Eeprom(void)
{
	DeviceID = eeprom_read(0x00);
}


//*************************************************************************************
// Interrupt Service Routine
// Check uart receive bit and if it is what caused the interrupt, turn LED on and
// read the data from the uart receive buffer.
// Note there is only one ISR to handle all interrupts so you need to determine
// what caused the interrupt before taking action.
//*************************************************************************************
void interrupt ISR() 
{


    if (PIR1bits.RCIF)          // see if interrupt caused by incoming data
    {
        uart_data = RCREG;     // read the incoming data
        ParseRx(uart_data);
        PIR1bits.RCIF = 0;      // clear interrupt flag
    }

}




void main()
{
	unsigned int val=0;
    Cpu_Init();

	Wdg_Init();

    Hal_Uart_Init();

    Hal_Io_Init();
	
	ReadAddr_Form_Eeprom();

   	mbus_hdr_slv_p = & mbus_hdr_slv;
	mbus_hdr_mstr_p =& mbus_hdr_mstr;

	while(1)
	{
		//Uart_Char("adc_init\r\n");
		CLRWDT(); 
		delay_us(10);
		Adc_Itext_Init();
        AD_Ivalue=Adc_Read();

	
		Read_Io_State();		

        delay_us(10);
		Adc_Vtext_Init();
        AD_Vvalue=Adc_Read();
		
		mbus_hdr_slv.uartprotocol_tlv.power_information__packed.VTEXT =	(AD_Vvalue*100/9-700);
		mbus_hdr_slv.uartprotocol_tlv.power_information__packed.ITEXT = (AD_Ivalue);
	
	

	}
}
