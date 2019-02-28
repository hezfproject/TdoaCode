/******************************************************************************\
* afe_cfg.h          
* DESCRIPTION:  
* config afe register through interal spi
*
*
\******************************************************************************/
#ifndef _afe_cfg_H_
#define _afe_cfg_H_

typedef unsigned long	Uint32;
typedef unsigned short	Uint16;
typedef unsigned char	Uint8;

#define AFE_CMD_MODUL_EN        	0x0000
#define AFE_CMD_MODUL_DIS       	0x0001
#define AFE_CMD_SLEEP           	0x0002
#define AFE_CMD_AGC_ON          	0x0003
#define AFE_CMD_AGC_OFF         	0x0004
#define AFE_CMD_RST             	0x0005
#define AFE_CMD_READ            	0x0006
#define AFE_CMD_WRITE           	0x0007
                                	
#define AFE_READ_REG0           	0x0000
#define AFE_READ_REG1           	0x0001
#define AFE_READ_REG2           	0x0002
#define AFE_READ_REG3           	0x0003
#define AFE_READ_REG4           	0x0004
#define AFE_READ_REG5           	0x0005
#define AFE_READ_REG6           	0x0006
#define AFE_READ_REG7           	0x0007
                                	
#define AFE_WRITE_REG0          	0x0000
#define AFE_WRITE_REG1          	0x0001
#define AFE_WRITE_REG2          	0x0002
#define AFE_WRITE_REG3          	0x0003
#define AFE_WRITE_REG4          	0x0004
#define AFE_WRITE_REG5          	0x0005
#define AFE_WRITE_REG6          	0x0006
//#define AFE_WRITE_REG7        	  0x0007
                                	
#define AFE_REG0_OEH_DIS        	0x0000
#define AFE_REG0_OEH_1MS        	0x0001
#define AFE_REG0_OEH_2MS        	0x0002
#define AFE_REG0_OEH_4MS        	0x0003
//#define AFE_REG0_OEL_1MS        	0x0000
#define AFE_REG0_OEL_1MS        	0x0001
#define AFE_REG0_OEL_2MS        	0x0002
#define AFE_REG0_OEL_4MS        	0x0003
#define AFE_REG0_ALARM_DIS      	0x0000
#define AFE_REG0_ALARM_EN       	0x0001
#define AFE_REG0_LCZEN_EN       	0x0000
#define AFE_REG0_LCZEN_DIS      	0x0001
#define AFE_REG0_LCYEN_EN       	0x0000
#define AFE_REG0_LCYEN_DIS      	0x0001
#define AFE_REG0_LCXEN_EN       	0x0000
#define AFE_REG0_LCXEN_DIS      	0x0001
                                	
#define AFE_REG1_DATOUT_MODUL   	0x0000
#define AFE_REG1_DATOUT_CARRY   	0x0001
#define AFE_REG1_DATOUT_RSSI    	0x0002
//#define AFE_REG1_DATOUT_RSSI  	  0x0003
                                	
#define LC_TUNE_0PF   				0x0000
#define LC_TUNE_1PF   				0x0001
#define LC_TUNE_2PF   				0x0002
#define LC_TUNE_3PF   				0x0003
#define LC_TUNE_4PF   				0x0004
#define LC_TUNE_5PF   				0x0005
#define LC_TUNE_6PF   				0x0006
#define LC_TUNE_7PF   				0x0007
#define LC_TUNE_8PF   				0x0008
#define LC_TUNE_9PF   				0x0009
#define LC_TUNE_10PF   				0x000a
#define LC_TUNE_11PF   				0x000b
#define LC_TUNE_12PF   				0x000c
#define LC_TUNE_13PF   				0x000d
#define LC_TUNE_14PF   				0x000e
#define LC_TUNE_15PF   				0x000f
#define LC_TUNE_16PF   				0x0010
#define LC_TUNE_17PF   				0x0011
#define LC_TUNE_18PF   				0x0012
#define LC_TUNE_19PF   				0x0013
#define LC_TUNE_20PF   				0x0014
#define LC_TUNE_21PF   				0x0015
#define LC_TUNE_22PF   				0x0016
#define LC_TUNE_23PF   				0x0017
#define LC_TUNE_24PF   				0x0018
#define LC_TUNE_25PF   				0x0019
#define LC_TUNE_26PF   				0x001a
#define LC_TUNE_27PF   				0x001b
#define LC_TUNE_28PF   				0x001c
#define LC_TUNE_29PF   				0x001d
#define LC_TUNE_30PF   				0x001e
#define LC_TUNE_31PF   				0x001f
#define LC_TUNE_32PF   				0x0020
#define LC_TUNE_33PF   				0x0021
#define LC_TUNE_34PF   				0x0022
#define LC_TUNE_35PF   				0x0023
#define LC_TUNE_36PF   				0x0024
#define LC_TUNE_37PF   				0x0025
#define LC_TUNE_38PF   				0x0026
#define LC_TUNE_39PF   				0x0027
#define LC_TUNE_40PF   				0x0028
#define LC_TUNE_41PF   				0x0029
#define LC_TUNE_42PF   				0x002a
#define LC_TUNE_43PF   				0x002b
#define LC_TUNE_44PF   				0x002c
#define LC_TUNE_45PF   				0x002d
#define LC_TUNE_46PF   				0x002e
#define LC_TUNE_47PF   				0x002f
#define LC_TUNE_48PF   				0x0030
#define LC_TUNE_49PF   				0x0031
#define LC_TUNE_50PF   				0x0032
#define LC_TUNE_51PF   				0x0033
#define LC_TUNE_52PF   				0x0034
#define LC_TUNE_53PF   				0x0035
#define LC_TUNE_54PF   				0x0036
#define LC_TUNE_55PF   				0x0037
#define LC_TUNE_56PF   				0x0038
#define LC_TUNE_57PF   				0x0039
#define LC_TUNE_58PF   				0x003a
#define LC_TUNE_59PF   				0x003b
#define LC_TUNE_60PF   				0x003c
#define LC_TUNE_61PF   				0x003d
#define LC_TUNE_62PF   				0x003e
#define LC_TUNE_63PF   				0x003f

#define AFE_REG2_RSSI_MOS_DW_DIS 	0x0000
#define AFE_REG2_RSSI_MOS_DW_EN 	0x0001
#define AFE_REG2_CLKDIV1 			0x0000
#define AFE_REG2_CLKDIV4			0x0001

#define RX_SENSI_REDUCE_0DB   		0x0000
#define RX_SENSI_REDUCE_2DB   		0x0001
#define RX_SENSI_REDUCE_4DB   		0x0002
#define RX_SENSI_REDUCE_6DB   		0x0003
#define RX_SENSI_REDUCE_8DB   		0x0004
#define RX_SENSI_REDUCE_10DB   		0x0005
#define RX_SENSI_REDUCE_12DB   		0x0006
#define RX_SENSI_REDUCE_14DB   		0x0007
#define RX_SENSI_REDUCE_16DB   		0x0008
#define RX_SENSI_REDUCE_18DB   		0x0009
#define RX_SENSI_REDUCE_20DB   		0x000a
#define RX_SENSI_REDUCE_22DB   		0x000b
#define RX_SENSI_REDUCE_24DB   		0x000c
#define RX_SENSI_REDUCE_26DB   		0x000d
#define RX_SENSI_REDUCE_28DB   		0x000e
#define RX_SENSI_REDUCE_30DB   		0x000f

#define AFE_REG5_AUTOCHSEL_DIS 		0x0000   
#define AFE_REG5_AUTOCHSEL_EN 		0x0001             
#define AFE_REG5_AGC_ANY 			0x0000   
#define AFE_REG5_AGC_20MV		    0x0001 
#define AFE_REG5_MODMIN_50			0x0000 
#define AFE_REG5_MODMIN_75			0x0001
#define AFE_REG5_MODMIN_25			0x0002                                    
#define AFE_REG5_MODMIN_12			0x0003

#define AFE_CMD(cmd)    			(Uint16)(cmd<<13)
#define AFE_ADD(add)    			(Uint16)(add<<9)

#define REG0_OEH(oeh)    			(Uint16)(oeh<<7)
#define REG0_OEL(oel)    			(Uint16)(oel<<5)
#define REG0_ALERT(alert)    		(Uint16)(alert<<4)
#define REG0_LCZEN(lczen)    		(Uint16)(lczen<<3)
#define REG0_LCYEN(lcyen)    		(Uint16)(lcyen<<2)
#define REG0_LCXEN(lcxen)    		(Uint16)(lcxen<<1)
#define REG0_ODD(oeh,oel,alert,lczen,lcyen,lcxen)   \
        (Uint16)(((oeh>>1)+(oeh&1)+(oel>>1)+(oel&1)+alert+lczen+lcyen+lcxen+1)&1)   			
#define REG1_DATOUT(datout)    		(Uint16)(datout<<7)
#define REG1_LCXTUN(lcxtun)    		(Uint16)(lcxtun<<1)
#define REG1_ODD(datout,lcxtun)                     \
		(Uint16)(((datout>>1)+(datout&1)+(lcxtun>>5)+((lcxtun&0x10)>>4)+((lcxtun&0x8)>>3)+((lcxtun&0x4)>>2)+((lcxtun&0x2)>>1)+(lcxtun&0x1)+1)&1)
#define REG2_RSSIFET(rssifet)		(Uint16)(rssifet<<8)
#define REG2_CLKDIV(clkdiv)    		(Uint16)(clkdiv<<7)
#define REG2_LCYTUN(lcytun)    		(Uint16)(lcytun<<1)
#define REG2_ODD(rssifet,clkdiv,lcytun)             \
		(Uint16)((rssifet+clkdiv+(lcytun>>5)+((lcytun&0x10)>>4)+((lcytun&0x8)>>3)+((lcytun&0x4)>>2)+((lcytun&0x2)>>1)+(lcytun&0x1)+1)&1)
#define REG3_LCZTUN(lcztun)    		(Uint16)(lcztun<<1)
#define REG3_ODD(lcztun)                            \
		(Uint16)(((lcztun>>5)+((lcztun&0x10)>>4)+((lcztun&0x8)>>3)+((lcztun&0x4)>>2)+((lcztun&0x2)>>1)+(lcztun&0x1)+1)&1)
#define REG4_LCXSEN(lcxsen)    		(Uint16)(lcxsen<<5)
#define REG4_LCYSEN(lcysen)    		(Uint16)(lcysen<<1)
#define REG4_ODD(lcxsen,lcysen)                     \
		(Uint16)(((lcxsen>>3)+((lcxsen&0x4)>>2)+((lcxsen&0x2)>>1)+(lcxsen&0x1)+(lcysen>>3)+((lcysen&0x4)>>2)+((lcysen&0x2)>>1)+(lcysen&0x1)+1)&1)
#define REG5_AUTOCHSEL(autochsel)  	(Uint16)(autochsel<<8)
#define REG5_AGCSIG(agcsig)  		(Uint16)(agcsig<<7)
#define REG5_MODMIN(modmin)  		(Uint16)(modmin<<5)
#define REG5_LCZSEN(lczsen)    		(Uint16)(lczsen<<1)
#define REG5_ODD(autochsel,agcsig,modmin,lczsen)    \
		(Uint16)((autochsel+agcsig+(modmin>>1)+(modmin&0x1)+(lczsen>>3)+((lczsen&0x4)>>2)+((lczsen&0x2)>>1)+(lczsen&0x1)+1)&1)

#define AFE_REG0_RMK(cmd,add,oeh,oel,alert,lczen,lcyen,lcxen)	\
(Uint16)(	AFE_CMD(cmd)			|							\
			AFE_ADD(add)			|							\
			REG0_OEH(oeh)			|							\
			REG0_OEL(oel)			|							\
			REG0_ALERT(alert)		|							\
			REG0_LCZEN(lczen)		|							\
			REG0_LCYEN(lcyen)		|							\
			REG0_LCXEN(lcxen)		|							\
			REG0_ODD(oeh,oel,alert,lczen,lcyen,lcxen)			\
		)

#define AFE_REG1_RMK(cmd,add,datout,lcxtun)						\
(Uint16)(	AFE_CMD(cmd)			|							\
			AFE_ADD(add)			|                           \
			REG1_DATOUT(datout)		|                           \
			REG1_LCXTUN(lcxtun)		|                           \
			REG1_ODD(datout,lcxtun)                             \
		)

#define AFE_REG2_RMK(cmd,add,rssifet,clkdiv,lcytun)				\
(Uint16)(	AFE_CMD(cmd)			|							\
			AFE_ADD(add)			|                           \
			REG2_RSSIFET(rssifet)	|                           \
			REG2_CLKDIV(clkdiv)		|                           \
			REG2_LCYTUN(lcytun) 	|                           \
			REG2_ODD(rssifet,clkdiv,lcytun)                     \
		)

#define AFE_REG3_RMK(cmd,add,lcztun)							\
(Uint16)(	AFE_CMD(cmd)			|							\
			AFE_ADD(add)			|                           \
			REG3_LCZTUN(lcztun)		|                           \
			REG3_ODD(lcztun)                                    \
		)

#define AFE_REG4_RMK(cmd,add,lcxsen,lcysen)						\
(Uint16)(	AFE_CMD(cmd)			|							\
			AFE_ADD(add)			|                           \
			REG4_LCXSEN(lcxsen)		|                           \
			REG4_LCYSEN(lcysen)		|                           \
			REG4_ODD(lcxsen,lcysen)                             \
		)

#define AFE_REG5_RMK(cmd,add,autochsel,agcsig,modmin,lczsen)	\
(Uint16)(	AFE_CMD(cmd)			|							\
			AFE_ADD(add)			|                           \
			REG5_AUTOCHSEL(autochsel)|                          \
			REG5_AGCSIG(agcsig)		|                           \
			REG5_MODMIN(modmin)		|                           \
			REG5_LCZSEN(lczsen)		|                           \
			REG5_ODD(autochsel,agcsig,modmin,lczsen)            \
		)
#endif



