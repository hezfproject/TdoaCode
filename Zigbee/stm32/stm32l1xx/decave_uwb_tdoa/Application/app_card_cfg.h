/*******************************************************************************
    Filename:     app_card_cfg.h

    Description:  ���澫ȷ��λϵͳӦ�ó���ͷ�ļ�

*******************************************************************************/

#ifndef _APP_CARD_CFG_H_
#define _APP_CARD_CFG_H_

/*******************************************************************************
* INCLUDES
*/

/*******************************************************************************
* TYPEDEFS
*/

/*******************************************************************************
* CONSTANTS AND DEFINES
*/
#define OPEN_SLEEP

// ������ͣ��һ��ʱ���ٿ�ʼ�ϱ����ȴ���ѹ�������
#define BOOT_DURATION_TIME          2000

#define LED_FLASH_TIME              20      //LED����˸ʱ������λms

//��λ�ϱ�ʱ��
#define LOC_REPORT_HIGH_PERIOD      2000            //��Χ: 1000~10000
#define LOC_REPORT_LOW_PERIOD       (5L*60*1000)    //5����

#define MAX_RANGING_FAIL_COUNT      5
#define CLOSE_SLEEP_FOR_RCV_TIME    10      //�յ���λ�ظ������ݱ�־�󣬹����ߵȴ����յ�ʱ��

#define BEACON_RCV_DURATION         55     //��beacon�󣬵ȴ���beacon�ظ���ʱ��
#define BEACON_SEND_EXPIRE_HIGH_TIME    10000
#define BEACON_SEND_EXPIRE_LOW_TIME     (10L*60*1000)   //10����

//�汾�ϱ�����
#define VERSION_REPORT_HIGH_PERIOD      (10L*60*1000)   //10����
#define VERSION_REPORT_LOW_PERIOD       (60L*60*1000)   //60����

//��ѹ���
#define ADC_CHECK_HIGH_PERIOD           (10L*60*1000)   //10����
#define ADC_CHECK_LOW_PERIOD            (60L*60*1000)   //60����
//#define ADC_CHECK_TICK              500
//#define ADC_CHECK_CNT               1

//�͵�
#define VDD_LIMIT                   36          // ���ޣ�0.1v��λ
#define HUNGER_FLASH_PERIOD         1000        // �͵磬�����˸���ʱ��
#define HUNGER_FLASH_TIME           15000       // �͵磬�����˸��ʱ��
#define MAX_HUNGER_FLASH_CNT        (HUNGER_FLASH_TIME/HUNGER_FLASH_PERIOD)

// ���
#define BUTTON_PRESS_HELP_TIME      2500    // ��Ȱ���ʱ��
#define BUTTON_PRESS_TEST_PERIOD    100     // �������̼������
#define HELP_MSG_REPORT_PERIOD      2000    // �ϱ������Ϣ����
#define HELP_FLASH_RED_PERIOD       500
#define HELP_FLASH_GREEN_PERIOD     500

#define MIN_HELP_TIMES              3       // ��3��
#define MAX_HELP_TIMES              (MIN_HELP_TIMES + 12)      // ��15��
#define HELP_RESPONSED_BEEP_FLASH_TIMES   (MAX_HELP_TIMES + 3)

//����
#define MAX_URGENT_ID_NUM           5       // ���泷��id�ĸ���

#define URGENT_BEEP_RED_FLASH_PERIOD    200
#define URGENT_CNF_IMMUNITY_NEW_TIME    30000
#define URGENT_CNF_IMMUNITY_OLD_TIME    (15L*60*1000)

#define URGENT_CNF_REPORT_TIMES 3

// motion detection
#define MOTION_DETECT_PERIOD            (1L*30*1000)    //30��
#define REST_TO_LOW_POWER_TIME          (1L*60*1000)    //1���ӣ�����ֹ��ʡ��״̬��ʱ��
#define MOTION_DETECT_CNT_MAX           (REST_TO_LOW_POWER_TIME/MOTION_DETECT_PERIOD)
#endif

