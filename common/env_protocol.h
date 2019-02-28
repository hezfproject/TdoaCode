#ifndef _TRANS_INTF_H_
#define _TRANS_INTF_H_

#ifndef __PACKED
#define __PACKED
#endif

// one byte alignment in Win32 and Wince app
#if (defined(WIN32) || defined(WINCE))
#pragma pack(push)
#pragma pack(1)
#endif


//����������
#define MIN_SENSOR_NUM          1
#define MAX_SENSOR_NUM          8

//��վID��������
#define MIN_SENSOR_SITEID       1
#define MAX_SENSOR_SITEID       255

//�����˿�
#define SENSOR_DEFAULT_PORT		6666		//ARM���Ӷ˿�

//��������
enum
{
	SENSOR_LINK_TYPE_DIANLIU = 0,	//����
	SENSOR_LINK_TYPE_PINLV,			//Ƶ��
	SENSOR_LINK_TYPE_WUYUAN			//��Դ����
};

//�˿�����
enum
{
	ANALOG_SENSOR_TYPE = 0,
	SWITCH_SENSOR_TYPE,
	CUMULANT_SENSOR_TYPE,
	UNKNOWN_SENSOR_TYPE,
};

enum
{
	//���з�վID
	COMM_UP_PAN_ID = 0,
	//���з�վ��SensorServer�ϱ�����
	COMM_UP_SENSOR_CONFIG,
	//���з�վ��SensorServer�ϱ�ֵ��SensorServer��APP�ϱ�ֵ
	COMM_UP_SENSOR_VALUE,

	// server�·�����
	COMM_DOWN_SENSOR_CONFIG,
	// server�·����з�վ����ģʽ
	COMM_DOWN_CONTROL_MODE,
	//server�·�ͬ��ʱ��
	COMM_DOWN_SYN_TIME,
	//�ϱ��汾
	COMM_REPORT_DEV_VERSION,
};

//����������״̬
enum
{
	SENSOR_OUTLINK = 0,	//��ط�վδ���ӷ�����
	SENSOR_LINK	   = 1,	//��ط�վ���ӷ�����
};

//����ģʽ����
enum
{
	AUTO_CONTROL = 0,	//�Զ�ģʽ
	MANUAL_OFF_CONTROL,	//�ֶ��ϵ�ģʽ
	MANUAL_ON_CONTROL,	//�ֶ�����ģʽ
	SERVER_OFF_CONTROL,	//������Զ�̶ϵ�ģʽ,���վ
	SERVER_ON_CONTROL,	//������Զ�̸���ģʽ�����վ
};

//���������ϱ����·���Э��ͷ
struct CommHeader
{
	int tag;
	int len;
};

//��վ�ϱ��Ĳɼ����ݽṹ
struct ST_Collect
{
	//ÿ���˿�2��short���ڶ�short����С����λ�ã���������
	short  sValue[MAX_SENSOR_NUM * 2];
	//��վID��1-255
	unsigned char cSiteId;
	//0������1����,��λΪ�˿�1
	char cWarn;
	//0���ϵ�1�ϵ�
	char cOutage;
	//0������1����
	char cReset;
	//0��������1�����쳣
	char cAbnormal;
	//0������1����
	char cOutLink;
	//��ʶ�ö˿���û����,0δ���1���
	char cInstalled;
	//����λ
	char cPadding;
	//��ʶ�˿ڴ���������,0ģ������1��������2�ۻ���
	char cSensorType[MAX_SENSOR_NUM];
	//0�Զ�   1�ֶ��ϵ�2�ֶ�����      3������Զ�̶ϵ� 4������Զ�̸���
	char cControlMode[MAX_SENSOR_NUM];
	//�ɼ�ʱ��
	time_t tOccTime;
}__PACKED;

//�ϴ����з�վID
struct ST_Up_SiteID
{
	unsigned char clen; 
	//unsigned char cSiteId;
}__PACKED;


//���з�վ����
struct ST_Down_Config
{
	unsigned char cSiteId;
	unsigned char cPort;
	time_t        tSetTime;//�ϱ�ʱ�����ݿ�����ʱ�䲻һ�µĻ��������·�
	//char *pData;           //�·�ʱ�ľ�������ֵ���ϱ�ʱû����һ��
}__PACKED;

//���з�վ����
struct ST_Up_Config
{
	unsigned char cSiteId;
	time_t        tSetTime[8];// 1~8  ���û������  ʱ��ҲΪ0
}__PACKED;

//�·�����ģʽ
struct ST_Control_Mode
{
	unsigned char cSiteId;		//���з�վID
	unsigned char cPort;		//�������˿�
	unsigned char cMode;		//���з�վ����ģʽ
}__PACKED;

//�ϱ��汾
struct ST_Dev_Version
{
	unsigned short nSiteId;	
	unsigned short nLen;
}__PACKED;

#endif

