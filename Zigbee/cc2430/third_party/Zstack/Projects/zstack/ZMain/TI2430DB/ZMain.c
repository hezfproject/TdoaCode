/**************************************************************************************************
  Filename:       ZMain.c
  Revised:        $Date: 2011/04/15 23:29:17 $
  Revision:       $Revision: 1.16 $
  
  Description:    Startup and shutdown code for ZStack
  Notes:          This version targets the Chipcon CC2430DB/CC2430EB


  Copyright 2005-2007 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, 
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE, 
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com. 
**************************************************************************************************/

/*********************************************************************
 * INCLUDES
 */

#include "ZComDef.h"
#include "OSAL.h"
#include "OSAL_Memory.h"
#include "OSAL_Nv.h"
#include "OnBoard.h"
#include "ZMAC.h"
#include "MTEL.h"

#include "nwk_globals.h"
#include "ZDApp.h"
#include "ssp.h"
#include "ZGlobals.h"

#include "app_cfg.h"

#ifndef NONWK
  #include "AF.h"
#endif

/* Hal */
#include "hal_lcd.h"
#include "hal_key.h"
#include "hal_led.h"
#include "hal_adc.h"
#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
#include "hal_audio.h"
#endif
#include "hal_drivers.h"
#include "hal_assert.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// LED Flash counter, waiting for default 64-bit address
#define FLASH_COUNT 20000

// Maximun number of Vdd samples checked before go on
#define MAX_VDD_SAMPLES  3
#define ZMAIN_VDD_LIMIT  HAL_ADC_VDD_LIMIT_4

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

extern __near_func uint8 GetCodeByte(uint32);

extern bool HalAdcCheckVdd (uint8 limit);

/*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * ZMAIN API JUMP FUNCTIONS
 *
 * If the MINIMIZE_ROOT compile flag is defined, ZMAIN API functions
 * are implemented as "jump functions" located in the ROOT segment,
 * as expected by the NWK object libraries. This allows the actual
 * ZMAIN function bodies to locate outside ROOT memory, increasing
 * space for user defined constants, strings, etc in ROOT memory.
 *
 * If the MINIMIZE_ROOT compile flag in not defined, the ZMAIN API
 * functions are aliased to the similarly-named function bodies and
 * located in the ROOT segment with no "jump function" overhead.
 * This is the default behavior which produces smaller overall code
 * size and maximizes available code space in BANK1...BANK3.
 *
 */

#ifdef MINIMIZE_ROOT
  // ZMAIN functions are not forced into ROOT segment
  #define ZSEG
#else
  // ZMAIN functions are forced into ROOT segment
  #define ZSEG ROOT
#endif

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static ZSEG void zmain_dev_info( void );
#if 0
static ZSEG void zmain_ext_addr( void );
#endif
static ZSEG void zmain_ram_init( void );
#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE) || (defined GASMONITOR_PROJECT)
#else
static ZSEG void zmain_vdd_check( void );
#endif
static ZSEG void zmain_ext_addr_check( void );

#ifdef LCD_SUPPORTED
static ZSEG void zmain_lcd_init( void );
#endif

/*********************************************************************
 * @fn      main
 * @brief   First function called after startup.
 * @return  don't care
 *********************************************************************/
ZSEG int main( void )
{
  
#if 1
/* Turn off shake and backlight first */
#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)  
  HALSHAKEINITIAL(false);
  BACKLIGHT_INIT;
  BACKLIGHT_OFF;
#endif
  
  // Turn off interrupts
  osal_int_disable( INTS_ALL );

  // Initialize HAL
  HAL_BOARD_INIT();

  // Make sure supply voltage is high enough to run
  #if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE) || (defined GASMONITOR_PROJECT)
  #else
  zmain_vdd_check();
  #endif
  // Initialize stack memory
  zmain_ram_init();

  // Initialize board I/O
  
  InitBoard( OB_COLD );

  // Initialze HAL drivers
  HalDriverInit();
  
  // Initialize NV System
  osal_nv_init( NULL );

  //Check the extended address
  zmain_ext_addr_check();
  
  // Determine the extended address
  //zmain_ext_addr();

  // Initialize basic NV items
  zgInit();

  // Initialize the MAC
  ZMacInit();

#ifndef NONWK
  // Since the AF isn't a task, call it's initialization routine
  afInit();
#endif

  // Initialize the operating system
  osal_init_system();

  // Allow interrupts
  osal_int_enable( INTS_ALL );

  // Final board initialization
  InitBoard( OB_READY );

  // Display information about this device
  zmain_dev_info();

  /* Display the device info on the LCD */
#ifdef LCD_SUPPORTED
  zmain_lcd_init();
#endif

  osal_start_system(); // No Return from here
  
  //HalAudioOpen();
  //while(1) Hal_ProcessPoll();
#else
#include "lcd.h"
#include "hal_audio.h"
#include "keyboard.h"
#define STOP_RADIO()        ISRFOFF;
#define ISRFOFF     do{RFST = 0xE5;}while(0);

	HAL_BOARD_INIT();
	InitialAudio();
	InitialLCD();
	STOP_RADIO();
	InitialKEY();
	StartLCD();  
	HalAudioStart();
	while(1)
	{
		
	}
#endif
} // main()

/*********************************************************************
 * @fn      zmain_vdd_check
 * @brief   Check if the Vdd is OK to run the processor.
 * @return  Return if Vdd is ok; otherwise, flash LED, then reset
 *********************************************************************/
#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE) || (defined GASMONITOR_PROJECT)
#else
static ZSEG void zmain_vdd_check( void )
{
  uint8 vdd_passed_count = 0;
  bool toggle = 0;

  // Repeat getting the sample until number of failures or successes hits MAX
  // then based on the count value, determine if the device is ready or not
  while ( vdd_passed_count < MAX_VDD_SAMPLES )
  {
    if ( HalAdcCheckVdd (ZMAIN_VDD_LIMIT) )
    {
      vdd_passed_count++;    // Keep track # times Vdd passes in a row
      MicroWait (10000);     // Wait 10ms to try again
    }
    else
    {
      vdd_passed_count = 0;  // Reset passed counter
      MicroWait (50000);     // Wait 50ms
      MicroWait (50000);     // Wait another 50ms to try again
    }

    /* toggle LED1 and LED2 */
    if (vdd_passed_count == 0)
    {
      if ((toggle = !(toggle)))
        HAL_TOGGLE_LED1();
      else
        HAL_TOGGLE_LED2();
    }
  }

  /* turn off LED1 */
  HAL_TURN_OFF_LED1();
  HAL_TURN_OFF_LED2();
}
#endif
#if 0
/*********************************************************************
 * @fn      zmain_ext_addr
 * @brief   Makes extended address if none exists.
 * @return  none
 *********************************************************************/
static ZSEG void zmain_ext_addr( void )
{
  uint8 i;
  uint8 led;
  uint8 tmp;
  uint8 *xad;
  uint16 AtoD;

  // Initialize extended address in NV
  osal_nv_item_init( ZCD_NV_EXTADDR, Z_EXTADDR_LEN, NULL );
  osal_nv_read( ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, &aExtendedAddress );

  // Check for uninitialized value (erased EEPROM = 0xFF)
  xad = (uint8*)&aExtendedAddress;
  for ( i = 0; i < Z_EXTADDR_LEN; i++ )
    if ( *xad++ != 0xFF ) return;

#ifdef ZDO_COORDINATOR
  tmp = 0x10;
#else
  tmp = 0x20;
#endif
  // Initialize with a simple pattern
  xad = (uint8*)&aExtendedAddress;
  for ( i = 0; i < Z_EXTADDR_LEN; i++ )
    *xad++ = tmp++;
  while(1)
  {
     MicroWait( 62500 );
     HalLedSet( HAL_LED_1, led^=HAL_LED_MODE_ON );  // Toggle the LED
     MicroWait( 62500 );
  }
#if 0
  // Flash LED1 until user hits SW5
  led = HAL_LED_MODE_OFF;
  #if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
  #else
  while ( HAL_KEY_SW_5 != HalKeyRead() )
  #endif
  {
    MicroWait( 62500 );
    HalLedSet( HAL_LED_1, led^=HAL_LED_MODE_ON );  // Toggle the LED
    MicroWait( 62500 );
  }
  HalLedSet( HAL_LED_1, HAL_LED_MODE_OFF );

  // Plug AtoD data into lower bytes
  AtoD = HalAdcRead (HAL_ADC_CHANNEL_7, HAL_ADC_RESOLUTION_10);
  xad = (uint8*)&aExtendedAddress;
  *xad++ = LO_UINT16( AtoD );
  *xad = HI_UINT16( AtoD );

#if !defined( ZTOOL_PORT ) || defined( ZPORT ) || defined( NV_RESTORE )
  // If no support for Z-Tool serial I/O,
  // Write temporary 64-bit address to NV
  osal_nv_write( ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, &aExtendedAddress );
#endif
#endif
}
#endif
/*********************************************************************
 * @fn      zmain_ext_addr_check
 * @brief   Checks extended address if valid or not.
 * @return  none
 *********************************************************************/
static ZSEG void zmain_ext_addr_check( void )
{
	// Initialize extended address in NV
	osal_nv_item_init( ZCD_NV_EXTADDR, Z_EXTADDR_LEN, NULL );
	osal_nv_read( ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, aExtendedAddress);

#if (defined MINE_PROJECTS)   // Sustation and Phone
#if (defined ZDO_COORDINATOR)	

/*     HAL_ASSERT(aExtendedAddress[EXT_MACADDR_TYPE] == EXT_MACADDR_TYPE_SUBSTATION);
	//Get the low two bytes to build the PANID.
	zgConfigPANID = BUILD_UINT16(aExtendedAddress[EXT_MACADDR_DEVID_LBYTE],
									aExtendedAddress[EXT_MACADDR_DEVID_HBYTE]);
*/
#elif (((defined HAL_AUDIO) && (HAL_AUDIO == TRUE)) || ((defined MINE_CONTROLLER)))
	HAL_ASSERT(aExtendedAddress[EXT_MACADDR_TYPE] == EXT_MACADDR_TYPE_PHONE);
	HAL_ASSERT(aExtendedAddress[EXT_MACADDR_CHANNEL] <= 0x1A && aExtendedAddress[EXT_MACADDR_CHANNEL] >= 0x0B);
#ifdef MENU_RF_DEBUG
       osal_nv_read( ZCD_NV_SET_PANID, 0, sizeof(uint16), &zgConfigPANID);
#endif
     
#endif
#elif(defined GASMONITOR_PROJECT) //Gas monitor
#if (defined ZDO_COORDINATOR)	
	HAL_ASSERT(aExtendedAddress[EXT_MACADDR_TYPE] == EXT_MACADDR_TYPE_GASMONITOR);
	//Get the low two bytes to build the PANID.
	zgConfigPANID = BUILD_UINT16(aExtendedAddress[EXT_MACADDR_DEVID_LBYTE],
									aExtendedAddress[EXT_MACADDR_DEVID_HBYTE]);
#endif
	HAL_ASSERT(aExtendedAddress[EXT_MACADDR_CHANNEL] <= 0x1A && aExtendedAddress[EXT_MACADDR_CHANNEL] >= 0x0B);
#elif (defined AMMETER_DATACENTER) ||(defined AMMETER_COLLECTOR)||(defined AMMETER_REPEATER) || (defined CARDTRANSFER_COLLECTOR) ||(defined CARDTRANSFER_DATACENTER)
	zgConfigPANID = BUILD_UINT16(aExtendedAddress[EXT_MACADDR_DEVID_LBYTE],
								aExtendedAddress[EXT_MACADDR_DEVID_HBYTE]);
#else
	  uint8 i;
  	  uint8 led;
  	  uint8 *xad;
	  // Check for uninitialized value (erased EEPROM = 0xFF)
	  xad = (uint8*)&aExtendedAddress;
	  for ( i = 0; i < Z_EXTADDR_LEN; i++ )
	    if ( *xad++ != 0xFF ) return;
		led = HAL_LED_MODE_OFF;
	  while(1)
	  {
	     MicroWait( 62500 );
            led^=HAL_LED_MODE_ON;
	     HalLedSet( HAL_LED_1, led);  // Toggle the LED
            HalLedSet( HAL_LED_2, led);
	     MicroWait( 62500 );
	  }
#endif
}

/*********************************************************************
 * @fn      zmain_dev_info
 * @brief   Gets or makes extended address.
 * @return  none
 *********************************************************************/
static ZSEG void zmain_dev_info ( void )
{
#ifdef LCD_SUPPORTED
  uint8 i;
  uint8 ch;
  uint8 *xad;
  unsigned char lcd_buf[18];

  // Display the extended address
  xad = (uint8*)&aExtendedAddress + Z_EXTADDR_LEN - 1;
  for ( i = 0; i < Z_EXTADDR_LEN*2; xad-- ) {
    ch = (*xad >> 4) & 0x0F;
    lcd_buf[i++] = ch + (( ch < 10 ) ? '0' : '7');
    ch = *xad & 0x0F;
    lcd_buf[i++] = ch + (( ch < 10 ) ? '0' : '7');
  }
  lcd_buf[Z_EXTADDR_LEN*2] = '\0';
  HalLcdWriteString( "IEEE Address:", HAL_LCD_LINE_1 );
  HalLcdWriteString( (char*)lcd_buf, HAL_LCD_LINE_2 );
#endif // LCD
}

/*********************************************************************
 * @fn      zmain_ram_init
 * @brief   Initialize ram for stack "high-water-mark" observations.
 * @return  none
 *********************************************************************/
static ZSEG void zmain_ram_init( void )
{
  uint8 *end;
  uint8 *ptr;

  // Initialize the call (parameter) stack
  end = (uint8*)CSTK_BEG;  // Lower end
  ptr = (uint8*)(*( __idata uint16*)(CSTK_PTR));  // Upper end
  while ( --ptr > end )
    *ptr = STACK_INIT_VALUE;

  // Initialize the return (address) stack
  ptr = (uint8*)RSTK_END - 1;  // Upper end
  while ( --ptr > (uint8*)SP )
    *(__idata uint8*)ptr = STACK_INIT_VALUE;
}

#ifdef LCD_SUPPORTED
/*********************************************************************
 * @fn      zmain_lcd_init
 * @brief   Initialize LCD at start up.
 * @return  none
 *********************************************************************/
static ZSEG void zmain_lcd_init ( void )
{
#ifdef LCD_SD
 // if ( LcdLine1 == NULL )
  {
    HalLcdWriteString( "Figure8 Wireless", HAL_LCD_LINE_1 );

#if defined( MT_MAC_FUNC )
#if defined( ZDO_COORDINATOR )
      HalLcdWriteString( "MAC-MT Coord", HAL_LCD_LINE_2 );
#else
      HalLcdWriteString( "MAC-MT Device", HAL_LCD_LINE_2 );
#endif // ZDO
#elif defined( MT_NWK_FUNC )
#if defined( ZDO_COORDINATOR )
      HalLcdWriteString( "NWK Coordinator", HAL_LCD_LINE_2 );
#else
      HalLcdWriteString( "NWK Device", HAL_LCD_LINE_2 );
#endif // ZDO
#endif // MT_FUNC
  }
#endif // LCD_SD
}
#endif

/*********************************************************************
*********************************************************************/
