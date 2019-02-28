#ifndef MENULIB_NV_H
#define MENULIB_NV_H

#include "Hal_types.h"
#include "App_Protocol.h"
#include "Menulib_global.h"

//************************************************************************//
#define LIST_ITEM_START          0
#define LIST_ITEM_END              0
#define LIST_ITEM_NULL		   0
#define MAX_NAME_LEN       9//8

#define SMS_LEN_TYPE_SIZE 1
//#define SMS_NV_LEN                    (SMS_MAX_LEN+NMBRDIGIT+SMS_LEN_TYPE_SIZE)//82

#define TIME_LEN                 8

#define   SMS_MAX_LEN   APP_SMS_MAX_LEN
#define MAX_SMS_LINES 	8
//#define MAX_SMS_LEN      SMS_NV_LEN

//#define SMS_MAX_NUM 6
#define MAX_CALL_NUM     10
#define MAX_CONTACT_NUM    100
#define MAX_SMS_NUM                    40// 15
#define MAX_SMS_NUM_TEMPLATE    3//5
#ifdef SMS_SENDBOX
#define MAX_SMS_NUM_SENDBOX     5
#endif

typedef enum
{
    SMSTYPE_SEND,
    SMSTYPE_INBOX,
    SMSTYPE_TEMPLATE,
}  sms_type;

typedef enum
{
    Record_type_DIALED,
    Record_type_MISSED,
    Record_type_ANSWERED,
}  Record_type;

typedef struct
{
    bool               isvalid;
    uint16            item_next;
    uint16            item_self;
} Item_head_t;

typedef struct
{
    //uint16             Contect_item;
    app_termNbr_t       num;
    uint8              Contect_item_L;
    uint8              time[TIME_LEN];
    Item_head_t   item_head;
} Record;


typedef struct
{
    app_termNbr_t       num;//uint8              num[NMBRDIGIT];
    uint8              name[MAX_NAME_LEN];
    Item_head_t   item_head;
} Contact_Node;

typedef struct
{
    bool 	isReaded;
    uint16 	seqnum;
    uint8  Contect_item_L;
    uint8 	len;
    app_termNbr_t nmbr;
    Item_head_t   item_head;
}  sms_head_t;

typedef struct
{
    sms_head_t	  	head;
    char 	              content[SMS_MAX_LEN];
}  sms_saved_t;

#define MP_STATUS_SUCCESS 			       ZSuccess  //0
#define MP_STATUS_OPER_FAILED		       NV_OPER_FAILED  //10
#define MP_STATUS_ALREADY_EXIST 	       ZFailure  // 1
#define MP_STATUS_INVALID_PARAM	       ZInvalidParameter   // 2
#define MP_STATUS_BUFFER_FULL               ZBufferFull 		// 0x11



//************************************************************************//
//  0x0401--0x040A
#define F_MP_NV_DIALED_01        0x0401
#define F_MP_NV_DIALED_02        0x0402
#define F_MP_NV_DIALED_03        0x0403
#define F_MP_NV_DIALED_04        0x0404
#define F_MP_NV_DIALED_05        0x0405
#define F_MP_NV_DIALED_06        0x0406
#define F_MP_NV_DIALED_07        0x0407
#define F_MP_NV_DIALED_08        0x0408
#define F_MP_NV_DIALED_09        0x0409
#define F_MP_NV_DIALED_0A        0x040A
#define MP_NV_DIALED_BASE        F_MP_NV_DIALED_01
#define MP_NV_DIALED_END         MP_NV_DIALED_BASE+ MAX_CALL_NUM-1  //0x040A

/*******************************************************************************/
//  0x040B--0x0414
#define F_MP_NV_MISSED_01        0x040B
#define F_MP_NV_MISSED_02        0x040C
#define F_MP_NV_MISSED_03        0x040D
#define F_MP_NV_MISSED_04        0x040E
#define F_MP_NV_MISSED_05        0x040F
#define F_MP_NV_MISSED_06        0x0410
#define F_MP_NV_MISSED_07        0x0411
#define F_MP_NV_MISSED_08        0x0412
#define F_MP_NV_MISSED_09        0x0413
#define F_MP_NV_MISSED_0A        0x0414
#define MP_NV_MISSED_BASE        F_MP_NV_MISSED_01
#define MP_NV_MISSED_END         MP_NV_MISSED_BASE+MAX_CALL_NUM-1  //0x0414

/*************************************************************************/
/*    				0x041B--0x0420								               */
/************************************************************************/

//  0x0415--0x041E
#define F_MP_NV_ANSWERED_01        0x0415
#define F_MP_NV_ANSWERED_02        0x0416
#define F_MP_NV_ANSWERED_03        0x0417
#define F_MP_NV_ANSWERED_04        0x0418
#define F_MP_NV_ANSWERED_05        0x0419
#define F_MP_NV_ANSWERED_06        0x041A
#define F_MP_NV_ANSWERED_07        0x041B
#define F_MP_NV_ANSWERED_08        0x041C
#define F_MP_NV_ANSWERED_09        0x041D
#define F_MP_NV_ANSWERED_0A        0x041E
#define MP_NV_ANSWERED_BASE        F_MP_NV_ANSWERED_01
#define MP_NV_ANSWERED_END          MP_NV_ANSWERED_BASE+MAX_CALL_NUM-1  //0x042A

/*************************************************************************/
/*    				0x041F--0x0430								               */
/************************************************************************/

#ifdef NEW_DOUBLE_NVID_OP
// 0x0431--0x0458
#define F_MP_NV_CONTACT_01      0x0431
#define F_MP_NV_CONTACT_02      0x0432
#define F_MP_NV_CONTACT_03      0x0433
#define F_MP_NV_CONTACT_04      0x0434
#define F_MP_NV_CONTACT_05      0x0435
#define F_MP_NV_CONTACT_06      0x0436
#define F_MP_NV_CONTACT_07      0x0437
#define F_MP_NV_CONTACT_08      0x0438
#define F_MP_NV_CONTACT_09      0x0439
#define F_MP_NV_CONTACT_0A      0x043A

#define F_MP_NV_CONTACT_0B      0x043B
#define F_MP_NV_CONTACT_0C      0x043C
#define F_MP_NV_CONTACT_0D      0x043D
#define F_MP_NV_CONTACT_0E      0x043E
#define F_MP_NV_CONTACT_0F      0x043F
#define F_MP_NV_CONTACT_10      0x0440
#define F_MP_NV_CONTACT_11      0x0441
#define F_MP_NV_CONTACT_12      0x0442
#define F_MP_NV_CONTACT_13      0x0443
#define F_MP_NV_CONTACT_14      0x0444

#define F_MP_NV_CONTACT_15      0x0445
#define F_MP_NV_CONTACT_16      0x0446
#define F_MP_NV_CONTACT_17      0x0447
#define F_MP_NV_CONTACT_18      0x0448
#define F_MP_NV_CONTACT_19      0x0449
#define F_MP_NV_CONTACT_1A      0x044A
#define F_MP_NV_CONTACT_1B      0x044B
#define F_MP_NV_CONTACT_1C      0x044C
#define F_MP_NV_CONTACT_1D      0x044D
#define F_MP_NV_CONTACT_1E      0x044E

#define F_MP_NV_CONTACT_1F      0x044F
#define F_MP_NV_CONTACT_20      0x0450
#define F_MP_NV_CONTACT_21      0x0451
#define F_MP_NV_CONTACT_22      0x0452
#define F_MP_NV_CONTACT_23      0x0453
#define F_MP_NV_CONTACT_24      0x0454
#define F_MP_NV_CONTACT_25      0x0455
#define F_MP_NV_CONTACT_26      0x0456
#define F_MP_NV_CONTACT_27      0x0457
#define F_MP_NV_CONTACT_28      0x0458

#define F_MP_NV_CONTACT_29      0x0459
#define F_MP_NV_CONTACT_2A      0x045A
#define F_MP_NV_CONTACT_2B      0x045B
#define F_MP_NV_CONTACT_2C      0x045C
#define F_MP_NV_CONTACT_2D      0x045D
#define F_MP_NV_CONTACT_2E      0x045E
#define F_MP_NV_CONTACT_2F      0x045F
#define F_MP_NV_CONTACT_30      0x0460
#define F_MP_NV_CONTACT_31      0x0461
#define F_MP_NV_CONTACT_32      0x0462

#define F_MP_NV_CONTACT_33      0x0463
#define F_MP_NV_CONTACT_34      0x0464
#define F_MP_NV_CONTACT_35      0x0465
#define F_MP_NV_CONTACT_36      0x0466
#define F_MP_NV_CONTACT_37      0x0467
#define F_MP_NV_CONTACT_38      0x0468
#define F_MP_NV_CONTACT_39      0x0469
#define F_MP_NV_CONTACT_3A      0x046A
#define F_MP_NV_CONTACT_3B      0x046B
#define F_MP_NV_CONTACT_3C      0x046C

#define F_MP_NV_CONTACT_3D      0x046D
#define F_MP_NV_CONTACT_3E      0x046E
#define F_MP_NV_CONTACT_3F      0x046F
#define F_MP_NV_CONTACT_40      0x0470
#define F_MP_NV_CONTACT_41      0x0471
#define F_MP_NV_CONTACT_42      0x0472
#define F_MP_NV_CONTACT_43      0x0473
#define F_MP_NV_CONTACT_44      0x0474
#define F_MP_NV_CONTACT_45      0x0475
#define F_MP_NV_CONTACT_46      0x0476

#define F_MP_NV_CONTACT_47      0x0477
#define F_MP_NV_CONTACT_48      0x0478
#define F_MP_NV_CONTACT_49      0x0479
#define F_MP_NV_CONTACT_4A      0x047A
#define F_MP_NV_CONTACT_4B      0x047B
#define F_MP_NV_CONTACT_4C      0x047C
#define F_MP_NV_CONTACT_4D      0x047D
#define F_MP_NV_CONTACT_4E      0x047E
#define F_MP_NV_CONTACT_4F      0x047F
#define F_MP_NV_CONTACT_50      0x0480

#define F_MP_NV_CONTACT_51      0x0481
#define F_MP_NV_CONTACT_52      0x0482
#define F_MP_NV_CONTACT_53      0x0483
#define F_MP_NV_CONTACT_54      0x0484
#define F_MP_NV_CONTACT_55      0x0485
#define F_MP_NV_CONTACT_56      0x0486
#define F_MP_NV_CONTACT_57      0x0487
#define F_MP_NV_CONTACT_58      0x0488
#define F_MP_NV_CONTACT_59      0x0489
#define F_MP_NV_CONTACT_5A      0x048A

#define F_MP_NV_CONTACT_5B      0x048B
#define F_MP_NV_CONTACT_5C      0x048C
#define F_MP_NV_CONTACT_5D      0x048D
#define F_MP_NV_CONTACT_5E      0x048E
#define F_MP_NV_CONTACT_5F      0x048F
#define F_MP_NV_CONTACT_60      0x0490
#define F_MP_NV_CONTACT_61      0x0491
#define F_MP_NV_CONTACT_62      0x0492
#define F_MP_NV_CONTACT_63      0x0493
#define F_MP_NV_CONTACT_64      0x0494

#define MP_NV_CONTACT_BASE      F_MP_NV_CONTACT_01
#define MP_NV_CONTACT_END        MP_NV_CONTACT_BASE+MAX_CONTACT_NUM-1//0x0494


/*******************************************************
//  0x0495--0x0497
*******************************************************/
#define F_MP_NV_SMS_TEMPLATE_01                0x0495
#define F_MP_NV_SMS_TEMPLATE_02                0x0496
#define F_MP_NV_SMS_TEMPLATE_03                0x0497
#define MP_NV_SMS_TEMPLATE_BASE                F_MP_NV_SMS_TEMPLATE_01
#define MP_NV_SMS_TEMPLATE_END                  MP_NV_SMS_TEMPLATE_BASE+MAX_SMS_NUM_TEMPLATE-1    //0x0497


/*******************************************************
//  0x0498--0x049C
*******************************************************/
#define F_MP_NV_SMS_SEND_01                0x0498
#define F_MP_NV_SMS_SEND_02                0x0499
#define F_MP_NV_SMS_SEND_03                0x049A
#define F_MP_NV_SMS_SEND_04                0x049B
#define F_MP_NV_SMS_SEND_05                0x049C
#define MP_NV_SMS_SEND_BASE                F_MP_NV_SMS_SEND_01
#ifdef SMS_SENDBOX
#define MP_NV_SMS_SEND_END                  MP_NV_SMS_SEND_BASE+MAX_SMS_NUM_SENDBOX-1    //0x049C
#endif

// 0x049D--0x04BA
#define F_MP_NV_SMS_INBOX_01                0x049D
#define F_MP_NV_SMS_INBOX_02                0x049E
#define F_MP_NV_SMS_INBOX_03                0x049F
#define F_MP_NV_SMS_INBOX_04                0x04A0
#define F_MP_NV_SMS_INBOX_05                0x04A1
#define F_MP_NV_SMS_INBOX_06                0x04A2
#define F_MP_NV_SMS_INBOX_07                0x04A3
#define F_MP_NV_SMS_INBOX_08                0x04A4
#define F_MP_NV_SMS_INBOX_09                0x04A5
#define F_MP_NV_SMS_INBOX_0A                0x04A6

#define F_MP_NV_SMS_INBOX_0B                0x04A7
#define F_MP_NV_SMS_INBOX_0C                0x04A8
#define F_MP_NV_SMS_INBOX_0D                0x04A9
#define F_MP_NV_SMS_INBOX_0E                0x04AA
#define F_MP_NV_SMS_INBOX_0F                0x04AB
#define F_MP_NV_SMS_INBOX_10                0x04AC
#define F_MP_NV_SMS_INBOX_11                0x04AD
#define F_MP_NV_SMS_INBOX_12                0x04AE
#define F_MP_NV_SMS_INBOX_13                0x04AF
#define F_MP_NV_SMS_INBOX_14                0x04B0

#define F_MP_NV_SMS_INBOX_15                0x04B1
#define F_MP_NV_SMS_INBOX_16                0x04B2
#define F_MP_NV_SMS_INBOX_17                0x04B3
#define F_MP_NV_SMS_INBOX_18                0x04B4
#define F_MP_NV_SMS_INBOX_19                0x04B5

#if 1
#define F_MP_NV_SMS_INBOX_1A                0x04B6
#define F_MP_NV_SMS_INBOX_1B                0x04B7
#define F_MP_NV_SMS_INBOX_1C                0x04B8
#define F_MP_NV_SMS_INBOX_1D                0x04B9
#define F_MP_NV_SMS_INBOX_1E                0x04BA

#define F_MP_NV_SMS_INBOX_1F                0x04BB
#define F_MP_NV_SMS_INBOX_20                0x04BC
#define F_MP_NV_SMS_INBOX_21                0x04BD
#define F_MP_NV_SMS_INBOX_22                0x04BE
#define F_MP_NV_SMS_INBOX_23                0x04BF
#define F_MP_NV_SMS_INBOX_24                0x04C0
#define F_MP_NV_SMS_INBOX_25                0x04C1
#define F_MP_NV_SMS_INBOX_26                0x04C2
#define F_MP_NV_SMS_INBOX_27                0x04C3
#define F_MP_NV_SMS_INBOX_28                0x04C4

//#define F_MP_NV_SMS_INBOX_29                0x04C5
//#define F_MP_NV_SMS_INBOX_2A                0x04C6
//#define F_MP_NV_SMS_INBOX_2B                0x04C7
//#define F_MP_NV_SMS_INBOX_2C                0x04C8
//#define F_MP_NV_SMS_INBOX_2D                0x04C9

//#define F_MP_NV_SMS_INBOX_2E               0x04CA
//#define F_MP_NV_SMS_INBOX_2F                0x04CB
//#define F_MP_NV_SMS_INBOX_30                0x04CC
//#define F_MP_NV_SMS_INBOX_31                0x04CD
//#define F_MP_NV_SMS_INBOX_32                0x04CE

#endif

#define MP_NV_SMS_INBOX_BASE                F_MP_NV_SMS_INBOX_01
#define MP_NV_SMS_INBOX_END                  MP_NV_SMS_INBOX_BASE+MAX_SMS_NUM-1    //0x04C4

#if 1
//#ifdef MENU_RF_DEBUG
#define ZCD_NV_SET_CHANLIST     0x04D0
#define ZCD_NV_SET_PANID           0x04D1
//#endif
#define MINEAPP_NV_SET_INFORMATION        0x04D2//set information include volume, backlight and time
#define MP_NV_R_OR_D_ITEM            		0x04D3
#define MP_SMS_SEQNUM					0x04D4
#define MP_STOREPARAM_ITEM                        0x04D5
#define MP_DISPLAY_PARAMETER		        0x04D6
#define ZCD_NV_SET_PHONENUM		       0x04D7
#else

/************************************************************************/
/* NV_ID settings					                                     */
/************************************************************************/

//#ifdef MENU_RF_DEBUG
#define ZCD_NV_SET_CHANLIST     0x04BC
#define ZCD_NV_SET_PANID           0x04BD
//#endif
#define MINEAPP_NV_SET_INFORMATION        0x04BE//set information include volume, backlight and time
#define MP_NV_R_OR_D_ITEM            		0x04BF
#define MP_SMS_SEQNUM					0x04C0
#endif
#endif

/* mobilephone */
#ifdef MOBILE_PROJECT
/* gas monitor */
#define GASSMS_MAX_NUM			   10
#define GASMONITOR_NV_SETTINGS        0x0206
#define GASMONITOR_NV_SMS_FLAGS	   0x0207
#define GASMONITOR_NV_SMS_BASE	   0x0208
#define GASMONITOR_NV_SMS_END	   (GASMONITOR_NV_SMS_BASE + GASSMS_MAX_NUM)
#endif


extern bool  isneed_judge_sms;

//extern uint8 menu_Contact_nv_init1(void);
extern uint8 int_contact_item_L(void);
extern uint8 menu_Contact_nv_init ( void);
extern uint8 menu_Contact_ReadContactNum(uint8 * pContactNum);
extern uint8 menu_Contact_ReadContact(Contact_Node *pContactNode, uint8 idx);
extern uint8 menu_Contact_AddContact(Contact_Node *pContactNode);
extern uint8 menu_Contact_DeleteContact(uint8 idx);
extern uint8 menu_Contact_SearchContactByNum(Contact_Node *pContactNode, uint8* pidx, const void* pNum);
extern bool Is_inContact_SearchContactByNum_and_item(Contact_Node *pContactNode,uint16 contact_item, const void* pNum);


void  Add_CallRecord(uint8, Record*);
extern uint8 menu_Record_nv_init(void);
extern uint8 menu_Record_AddRecord(Record *pRecord,Record_type recordtype);
extern uint8 menu_Record_DeleteRecord(uint8 idx,Record_type recordtype);
extern uint8 menu_Record_DeleteAll(Record_type recordtype);
extern uint8 Record_List_Search_End_Valid(Record *pRecord,uint16 *head_item,uint8 *pidx,Record_type recordtype );
extern uint8 menu_Record_ReadRecord(Record *pRecord, uint8 idx,Record_type recordtype);
extern uint8 menu_Record_Read_Num(uint8 *pNum,Record_type recordtype);

extern uint8 int_SMS_item_L(void);
extern uint8 SMS_List_init ( sms_type  smstype);
extern uint8 menu_SMS_Read_Num(uint8 *pNum,sms_type smstype);
extern uint8 menu_SMS_Read_unread(uint8* pNum);
extern uint8 menu_SMS_Search_Inbox(app_mpSMS_t *pSMS);
extern uint8 menu_SMS_Add(sms_saved_t *pSMS,sms_type smstype);
extern uint8 menu_SMS_Delete(uint8 idx,sms_type smstype);
extern uint8 menu_SMS_DeleteAll(sms_type smstype);
extern uint8 menu_SMS_Read_SMS(sms_saved_t *pSMS,uint8 idx,sms_type smstype);
extern uint8 SMS_List_Get_FirstValid_Item(uint16 *head_item,sms_type smstype);
#endif


