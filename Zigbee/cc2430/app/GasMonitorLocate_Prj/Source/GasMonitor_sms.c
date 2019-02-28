/**************************************************************************************************
Filename:       GasMonitor_sms.c
Revised:        $Date: 2011/06/01 22:53:46 $
Revision:       $Revision: 1.1 $

Description:   This module provided an sms processing interface  for gasmonitor project

**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "AppProtocol.h"
#include "App_cfg.h"
#include "ZComDef.h"
#include "string.h"
#include "osal_Nv.h"
#include "GasMonitor_sms.h"
#include "GasMenuLib_tree.h"
#include "lcd_serial.h"
/*************************************************************************************************
*CONSTANTS
*/

/*************************************************************************************************
*TYPEDEFS
*/
typedef struct
{
	bool		isValid;            // must be the first, sometimes will read this byte only !!!
	bool 	isReaded;
	uint16 	seqnum;
	uint8 	len;
	termNbr_t 	nmbr;
}  gassms_nv_head_t;
typedef struct
{
	gassms_nv_head_t  	head;		// must be the first, sometimes read head only!!
	char 	content[GASSMS_MAX_LEN];
}  gassms_nv_t;

typedef struct
{
	uint8 num;
	uint8 unread_num;
}  gassms_nv_flags_t;

typedef struct
{
	uint8 lineNum;
	char* splitPoshead[GASSMS_MAX_LINES+1];
	char* splitPostail[GASSMS_MAX_LINES+1];
}  print_info_t;

/*********************************************************************
* LOCAL VARIABLES
*/
// always the same as FLASH, only use to reduce the FLASH access time
// when it is writed, must also write to FLASH
static gassms_nv_flags_t  nv_flag;
static print_info_t  print_info;
static gassms_nv_head_t  overflow_sms;

/*********************************************************************
* LOCAL FUNCTIONS
*/
static uint8 GasSms_SyncNvFlag ( void );
static bool GasSms_isValid ( uint8 real_num );
static uint8  GasSms_GetRealNum ( uint8 *pRealNum,uint8 num );
static uint8 GasSms_GetSmsHeader ( gassms_nv_head_t * phead, uint8 num );
static uint8  GasSms_SplitIntoLines ( const char* pSms, uint8 len,uint8 width );
static char* GasSms_GetNextLine ( const char* psms, uint8 len, uint8 LineLimit );
#ifdef SMS_NVDEFRAG
static uint8 GasSms_NvDefrag (void);
#endif
/*********************************************************************
* @fn      GasSms_NV_init
*
* @brief   Initialization of NV items.
*
* @param   none.
*
* @return  ZSuccess or GASSMS_STATUES_OPER_FAILED
*/
uint8 GasSms_NV_init ( void )
{
	uint8 flag,returnflag;
	returnflag = ZSuccess;

	for ( uint8 i=0; i< GASSMS_MAX_NUM; i++ )
	{
		flag = osal_nv_item_init ( GASMONITOR_NV_SMS_BASE + i, sizeof ( gassms_nv_t ), NULL );
		if ( flag == NV_ITEM_UNINIT )
		{
			gassms_nv_head_t nv_head;
			nv_head.isValid =  false;
			if ( ZSuccess!=osal_nv_write ( GASMONITOR_NV_SMS_BASE + i,0, sizeof ( gassms_nv_head_t ), ( void* ) &nv_head ) )
			{
				returnflag = GASSMS_STATUES_OPER_FAILED;
			}
		}
		else if ( flag == NV_OPER_FAILED )
		{
			returnflag = GASSMS_STATUES_OPER_FAILED;
		}
	}

	flag = osal_nv_item_init ( GASMONITOR_NV_SMS_FLAGS, sizeof ( gassms_nv_flags_t ), NULL );
	if ( flag == NV_OPER_FAILED )
	{
		returnflag =   GASSMS_STATUES_OPER_FAILED;
	}
	else
	{
		if ( GasSms_SyncNvFlag() !=ZSuccess )
		{
			returnflag = GASSMS_STATUES_OPER_FAILED;
		}
	}
	return returnflag;
}

/*********************************************************************
* @fn      GasSms_SaveSms
*
* @brief   Save a new received sms
*
* @param   input:	pContant,len,seqnum,nmbr
*
* @return  ZSuccess, GASSMS_STATUES_INVALID_PARAM,GASSMS_STATUES_INVALID_LEN, GASSMS_STATUES_BUFFER_FULL
*/

uint8 GasSms_SaveSms ( const char * pContant, uint8 len, uint16 seqnum, termNbr_t nmbr )
{     	   
	if ( pContant == NULL )
	{
		return GASSMS_STATUES_INVALID_PARAM;
	}
	if ( len==0 || len>GASSMS_MAX_LEN )
	{
		return GASSMS_STATUES_INVALID_LEN;
	}
	
	gassms_nv_head_t  nv_head;
	for ( uint8 i=0; i< GASSMS_MAX_NUM; i++ )
	{
		osal_nv_read ( GASMONITOR_NV_SMS_BASE + i , 0, sizeof ( gassms_nv_head_t ), &nv_head );
		if ( nv_head.isValid  &&  ( nv_head.seqnum == seqnum && strcmp ( ( void* ) nmbr.Nmbr, ( void* ) nv_head.nmbr.Nmbr ) ==0 ) )
		{
			return GASSMS_STATUES_ALREADY_EXIST;
		}
	}
	
	if ( nv_flag.num >= GASSMS_MAX_NUM)
	{			  
	      GasSms_DelSms(0);
	}

#ifdef SMS_NVDEFRAG
	uint8 flag=GasSms_NvDefrag();
	if(flag!=ZSUCCESS)
	{
             return GASSMS_STATUES_OPER_FAILED;
	}
#endif	

	for ( uint8 i=0; i< GASSMS_MAX_NUM; i++ )
	{
		if ( !GasSms_isValid ( i ) )
		{
			gassms_nv_t  nv_item;
			nv_item.head.seqnum = seqnum;
			nv_item.head.isValid= true;
			nv_item.head.isReaded = false;
			nv_item.head.len = len;
			strcpy ( ( char* ) nv_item.head.nmbr.Nmbr, ( char* ) nmbr.Nmbr );
			memcpy ( ( void* ) nv_item.content, ( void * ) pContant, len );

			uint8 flag;

			flag = osal_nv_write ( GASMONITOR_NV_SMS_BASE + i, 0, sizeof ( gassms_nv_t ), ( void * ) &nv_item );
			
			if ( flag == ZSuccess )
			{
			       nv_flag.num++;
                            nv_flag.unread_num++;
				return  osal_nv_write ( GASMONITOR_NV_SMS_FLAGS, 0, sizeof ( gassms_nv_flags_t ), ( void * ) &nv_flag );
			}
			else  // write fail
			{
				return flag;
			}
		}
	}

	/* if program can run here, nv_flag is incorrect */
	GasSms_SyncNvFlag();
	return  GASSMS_STATUES_OPER_FAILED;
}
/*********************************************************************
* @fn      GasSms_ReadSms
*
* @brief   Read a sms from flash
*
* @param   input:	num
* 		    output: pContant, pLen
*
* @return  ZSuccess, GASSMS_STATUES_INVALID_PARAM,GASSMS_STATUES_INVALID_LEN
*/

uint8 GasSms_ReadSms ( char * pContant, uint8 *pLen, uint8 num )
{
	uint8 flag;
	if ( pContant== NULL || num >= nv_flag.num || num>=GASSMS_MAX_NUM )
	{
		return GASSMS_STATUES_INVALID_PARAM;
	}
	uint8 realnum;
	if ( ( flag = GasSms_GetRealNum ( &realnum, num ) ) != ZSuccess )
	{
		return flag;
	}

	gassms_nv_t  nv_item;
	flag = osal_nv_read ( GASMONITOR_NV_SMS_BASE + realnum, 0, sizeof ( gassms_nv_t ), ( void * ) &nv_item );
	if ( flag != ZSuccess )  // read fail
	{
		return flag;
	}

	if ( nv_item.head.len <= GASSMS_MAX_LEN )
	{
		*pLen  = nv_item.head.len;
		memcpy ( ( void* ) pContant, ( void* ) nv_item.content,nv_item.head.len );
	}
	else // length is error
	{
		return GASSMS_STATUES_INVALID_LEN;
	}

	if ( nv_item.head.isReaded == false )  // update the read flag
	{
		nv_item.head.isReaded = true;
		flag = osal_nv_write ( GASMONITOR_NV_SMS_BASE +realnum , 0, sizeof ( gassms_nv_head_t ), ( void * ) &nv_item.head );
		if ( flag == ZSuccess )
		{
			if ( nv_flag.unread_num>0 )
			{
				nv_flag.unread_num--;
				return osal_nv_write ( GASMONITOR_NV_SMS_FLAGS, 0, sizeof ( gassms_nv_flags_t ), ( void * ) &nv_flag );
			}
		}
		else
		{
			return flag;
		}
	}
	return ZSuccess;
}
/*********************************************************************
* @fn      GasSms_DelSms
*
* @brief   delete  a sms
*
* @param   input:	num
*
* @return  ZSuccess, GASSMS_STATUES_INVALID_PARAM
*/

uint8 GasSms_DelSms ( uint8 num )
{
	uint8 flag;
	if ( num >= nv_flag.num || num>=GASSMS_MAX_NUM )
	{
		return GASSMS_STATUES_INVALID_PARAM;
	}
	overflow_sms.isValid = false;    // when delete sms, the overflow buff will be flushed

	uint8 realnum;
	if ( ( flag = GasSms_GetRealNum ( &realnum, num ) ) != ZSuccess )
	{
		return flag;
	}

	gassms_nv_head_t  nv_head;
	nv_head.isValid = false;
	flag = osal_nv_write ( GASMONITOR_NV_SMS_BASE+realnum, 0, sizeof ( gassms_nv_head_t ), ( void * ) &nv_head );

	if ( flag == ZSuccess )
	{
		if ( nv_flag.num>0 )
			nv_flag.num--;
		if ( nv_flag.unread_num>0 && !nv_head.isReaded )
			nv_flag.unread_num--;
		return osal_nv_write ( GASMONITOR_NV_SMS_FLAGS, 0, sizeof ( gassms_nv_flags_t ), ( void * ) &nv_flag );
	}
	else
	{
		return flag;
	}
}
/*********************************************************************
* @fn      GasSms_ClearSms
*
* @brief   clear all sms
*
* @param   none
*
* @return  sms number
*/
uint8 GasSms_ClearSms ( void )
{
	uint8 flag,returnflag;
	returnflag = ZSuccess;

	overflow_sms.isValid = false;    // when delete sms, the overflow buff will be flushed

	for ( uint8 i=0; i< GASSMS_MAX_NUM; i++ )
	{
		gassms_nv_head_t  nv_head;
		nv_head.isValid = false;
		flag = osal_nv_write ( GASMONITOR_NV_SMS_BASE+i, 0, sizeof ( gassms_nv_head_t ), ( void * ) &nv_head );
		if ( flag != ZSuccess )
		{
			returnflag = GASSMS_STATUES_OPER_FAILED;
		}
	}
	nv_flag.num = 0;
	nv_flag.unread_num = 0;
	flag = osal_nv_write ( GASMONITOR_NV_SMS_FLAGS, 0, sizeof ( gassms_nv_flags_t ), ( void * ) &nv_flag );
	if ( flag != ZSuccess )
	{
		returnflag = GASSMS_STATUES_OPER_FAILED;
	}
	return returnflag;

}
/*********************************************************************
* @fn      GasSms_GetSmsNmbr
*
* @brief    get sms send number
*
* @param   input:	num
* 		    output: pNmbr
*
* @return  ZSuccess, GASSMS_STATUES_INVALID_PARAM
*/

uint8 GasSms_GetSmsNmbr ( termNbr_t* pNmbr, uint8 num )
{
	if ( num >= nv_flag.num || num>=GASSMS_MAX_NUM )
	{
		return GASSMS_STATUES_INVALID_PARAM;
	}
	gassms_nv_head_t nv_head;
	if ( GasSms_GetSmsHeader ( &nv_head, num ) == ZSuccess )
	{
		*pNmbr = nv_head.nmbr;
		return ZSuccess;
	}
	return  GASSMS_STATUES_OPER_FAILED;
}

uint8 GasSms_GetSmsReadFlag ( bool* pIsReaded, uint8 num )
{
	if ( num >= nv_flag.num || num>=GASSMS_MAX_NUM )
	{
		return GASSMS_STATUES_INVALID_PARAM;
	}
	gassms_nv_head_t nv_head;
	if ( GasSms_GetSmsHeader ( &nv_head, num ) == ZSuccess )
	{
		*pIsReaded = nv_head.isReaded;
		return ZSuccess;
	}
	return  GASSMS_STATUES_OPER_FAILED;
}

static uint8 GasSms_GetSmsHeader ( gassms_nv_head_t * phead, uint8 num )
{
	uint8 flag;
	if ( num >= nv_flag.num || num>=GASSMS_MAX_NUM )
	{
		return GASSMS_STATUES_INVALID_PARAM;
	}
	uint8 realnum;
	if ( ( flag=GasSms_GetRealNum ( &realnum, num ) ) !=ZSuccess )
	{
		return flag;
	}
	flag = osal_nv_read ( GASMONITOR_NV_SMS_BASE+realnum, 0, sizeof ( gassms_nv_head_t ), ( void * ) phead );
	if ( flag == ZSuccess )
	{
		return ZSuccess;
	}
	else
	{
		return flag;
	}
}

/*********************************************************************
* @fn      GasSms_GetSMSNum
*
* @brief   Read current sms number
*
* @param   none
*
* @return  sms number
*/
uint8 GasSms_GetSMSCnt ( void )
{
	return nv_flag.num;
}
/*********************************************************************
* @fn      GasSms_GetUnReadSMSNum
*
* @brief   Read current unread  sms number
*
* @param   none
*
* @return unread sms number
*/
uint8 GasSms_GetUnReadSMSCnt ( void )
{
	return nv_flag.unread_num;
}

/*********************************************************************
* @fn      GasSms_Print
*
* @brief   print sms in current LCD
*
* @param  pSms,len,StartX,StartY,ScreenHeight,width
*
* @return printed smslength
*/
uint8  GasSms_LCDPrint ( const char* pSms, uint8 len, uint8 smsstartLine,uint8 screenstartline, uint8 screenHeight,uint8 screenwidth )
{
	uint8 flag;

	if ( pSms==NULL || len==0 )
	{
		return GASSMS_STATUES_INVALID_PARAM;
	}

	if ( ( flag = GasSms_SplitIntoLines ( pSms, len, screenwidth ) ) != ZSuccess )
	{
		return flag;
	}
	if ( screenstartline > print_info.lineNum )
	{
		return GASSMS_STATUES_INVALID_PARAM;
	}

	if ( print_info.lineNum-smsstartLine <= screenHeight-screenstartline )  // can print all sms in a screen page
	{
		for ( uint8 i = smsstartLine,j=screenstartline; i<print_info.lineNum; i++,j++ )
		{
			uint8 len = print_info.splitPostail[i]-print_info.splitPoshead[i];
			LCD_Memory_Print ( ( uint8 * ) print_info.splitPoshead[i],len, 0, j );
		}
	}
	else   // cannot print all sms
	{
		for ( uint8 i = smsstartLine,j=screenstartline; j<screenHeight; i++,j++ )
		{
			uint8 len = print_info.splitPostail[i]-print_info.splitPoshead[i];
			LCD_Memory_Print ( ( uint8 * ) print_info.splitPoshead[i],len, 0, j );
		}
	}

	return ZSuccess;
}

uint8 GasSms_GetLineCnt ( const char* pSms, uint8 len,uint8 width )
{
       memset(&print_info,0,sizeof(print_info));
	   
       GasSms_SplitIntoLines ( pSms, len, width );
       return print_info.lineNum;	   
}

static uint8  GasSms_SplitIntoLines ( const char* pSms, uint8 len,uint8 width )
{
	char* p= ( char * ) pSms;
	uint8  totalLen = 0;

	print_info.lineNum = 0;


	for ( uint8 i=0; i<GASSMS_MAX_LINES; i++ )
	{
		char *p1;
		p1 = GasSms_GetNextLine ( p,len-totalLen,width );
		totalLen += ( p1-p );

		print_info.splitPoshead[i] = p;
		print_info.splitPostail[i]=p1;

		if ( p1 >= pSms+len ) 				// reach the end
		{
			print_info.lineNum = i+1;
			print_info.splitPoshead[i+1] = ( char * ) pSms+len;
			print_info.splitPostail[i+1]= ( char * ) pSms+len;
			return ZSuccess;
		}
		if ( ( * p1==0x0d ) && ( * ( p1+1 ) ==0x0a ) )
		{
			p=p1+2;
			totalLen+=2;
		}
		else
		{
			p = p1;
		}
	}
	return GASSMS_STATUES_OPER_FAILED;
}

static char* GasSms_GetNextLine ( const char* psms, uint8 len, uint8 LineLimit )
{
	char* p = ( char* ) psms;

	if ( len<=LineLimit )
	{
		for ( uint8 i=0; i<len; i++ )
		{
			p++;
			if ( *p==0x0d && * ( p+1 ) ==0x0a )
			{
				return p;
			}

		}
		return p;
	}

	while ( 1 )
	{
		if ( *p > 0x80 )  // start of GBK
		{
			p+=2;
			if ( p-psms == LineLimit )
			{
				return p;
			}
			else if ( ( p-psms ) > LineLimit )
			{
				return  p-2;
			}
		}
		else
		{
			p++;
			if ( *p==0x0d && * ( p+1 ) ==0x0a )
			{
				return p;
			}

			if ( p - psms>= LineLimit )
			{
				return p;
			}

		}
	}

}


/*********************************************************************
* @fn      GasSms_SyncNvFlag
*
* @brief   check the valid and read flag in every item, and update the numbers in nv_flag
*
* @param   none
*
* @return  ZSUCCESS if successful, NV_ITEM_UNINIT if item did not
*              exist in NV and offset is non-zero, NV_OPER_FAILED if failure.
*/
static uint8 GasSms_SyncNvFlag ( void )
{
	nv_flag.num = 0;
	nv_flag.unread_num = 0;
	gassms_nv_head_t nv_head;
	for ( uint8 i=0; i< GASSMS_MAX_NUM; i++ )
	{
		osal_nv_read ( GASMONITOR_NV_SMS_BASE + i , 0, sizeof ( gassms_nv_head_t ), ( void * ) &nv_head );
		if ( nv_head.isValid )
		{
			nv_flag.num++;
			if ( !nv_head.isReaded )
			{
				nv_flag.unread_num++;
			}
		}
	}
	return  osal_nv_write ( GASMONITOR_NV_SMS_FLAGS, 0, sizeof ( gassms_nv_flags_t ), ( void * ) &nv_flag );
}

/*********************************************************************
* @fn      GasSms_GetRealNum
*
* @brief   get real nums  in flash
*
* @param   input: num
*		output:pRealnum
* @return
*/
static uint8  GasSms_GetRealNum ( uint8 *pRealNum,uint8 num )
{
	if ( num >= nv_flag.num || num>=GASSMS_MAX_NUM )
	{
		return GASSMS_STATUES_INVALID_PARAM;
	}

	for ( uint8 i=0,cnt = 0; i< GASSMS_MAX_NUM; i++ )
	{
		if ( GasSms_isValid ( i ) )
		{
			if ( num==cnt )
			{
				*pRealNum = i;
				return ZSuccess;
			}
			else
			{
				cnt++;
			}
		}
	}

	/* if program can run here, nv_flag is incorrect */
	GasSms_SyncNvFlag();
	return  GASSMS_STATUES_OPER_FAILED;

}
/*********************************************************************
* @fn      GasSms_isValid
*
* @brief   check the valid flag and return
*
* @param   input: num
*
* @return   the valid flag
*/
static bool GasSms_isValid ( uint8 real_num )
{
	bool isValid;
	osal_nv_read ( GASMONITOR_NV_SMS_BASE + real_num , 0, sizeof ( bool ), ( void * ) &isValid );
	return isValid;
}


uint8  GasSms_FillOverFlowSms ( uint16 seqnum,const  termNbr_t nmbr )
{
	if ( nv_flag.num < GASSMS_MAX_NUM )
	{
		return GASSMS_STATUES_OPER_FAILED;
	}

	if ( overflow_sms.isValid == TRUE )
	{
		return GASSMS_STATUES_OPER_FAILED;
	}
	else
	{
		overflow_sms.isValid = TRUE;
		overflow_sms.seqnum = seqnum;
		overflow_sms.nmbr = nmbr;
		return ZSuccess;
	}

}

bool  GasSms_IsHaveOverFlowSms ( void )
{
	if ( nv_flag.num <GASSMS_MAX_NUM )
	{
		overflow_sms.isValid = false;
	}
	return overflow_sms.isValid;
}

#ifdef SMS_NVDEFRAG
static uint8 GasSms_NvDefrag (void)
{
     gassms_nv_t  gassms_nv;
     bool               isValid;
     uint8             realnum=0;

     uint8 flag1,flag2,flag3;
	 
     for ( uint8 i=0; i< GASSMS_MAX_NUM; i++ )
	{
		if ( GasSms_isValid ( i ) )
		{
                  flag1= osal_nv_read ( GASMONITOR_NV_SMS_BASE + i , 0, sizeof ( gassms_nv_t), ( void * ) &gassms_nv );
				  
		    if(flag1==ZSUCCESS)
                  {
                   	       if(realnum==i)
			        {
                                  realnum++;
			        }
                   	       else
				{
     		                   flag2= osal_nv_write(GASMONITOR_NV_SMS_BASE+realnum, 0, sizeof ( gassms_nv_t), ( void * ) &gassms_nv);
     
     		                  if(flag2==ZSUCCESS)
     		       	    {
					realnum++;		
     		       	    }
     				   else
     			          {
                                   return flag2;
     				   }
                   	       }

                  }
		    else
		    {
                            return flag1;
		    }
		}
	}
	 
	for(uint8 i=realnum;i<GASSMS_MAX_NUM;i++)	      
	{
	     isValid = false;
            flag3= osal_nv_write(GASMONITOR_NV_SMS_BASE+i, 0, sizeof ( bool), ( void * ) &isValid);

	     if(flag3!=ZSUCCESS)
	     {
                return flag3;
	     }
	}

	return ZSUCCESS;

}
#endif