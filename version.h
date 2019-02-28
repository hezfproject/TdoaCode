#ifndef VERSION_H
#define VERSION_H

#define VERSION "LFK-V1.1-RC3"
#define RELEASE "2018.12.04"

/******************************************************/
//default : old_lcd  parameter
//#define NEW_LCD_20120104
//#define     NEW_LCD_20120907    
#define     NEW_LCD_20121220   
/******************************************************/
#if defined NEW_LCD_20120104
//NEW LCD
#define  LCD_PARAMETER1     0xff 
#define  LCD_PARAMETER2     0xdc
#elif  defined NEW_LCD_20120907
#define  LCD_PARAMETER1     0xfb
#define  LCD_PARAMETER2     0xe4
#elif  defined NEW_LCD_20121220
#define  LCD_PARAMETER1     0xf0
#define  LCD_PARAMETER2     0x6f
#else
//OLD LCD
#define  LCD_PARAMETER1     0xf2 
#define  LCD_PARAMETER2     0xfc
#endif 

/* 2.0 OAD version */
#define OAD_CARD_VERSION	(0x0FA4) // B means 2011, 8 means Aug, 5 means version 5 of 2011.08
#define OAD_LOCATOR_VERSION	(0x0E61) 
#define OAD_LOC_STATION_VERSION	(0x0E61) 
#define OAD_RSSI_STATION_VERSION	(0x0E61) 
#define OAD_CHECKIN_STATION_VERSION	(0x0E61) 
#define OAD_COM_STATION_VERSION	(0x0E61) 

/* UWB card OAD version*/

#define OAD_UWB_CARD_VERSION (0x11A9)   //11 means 2017,A means Oct, 1 means version 1 of 2017.10

#endif
