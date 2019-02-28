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


//传感器个数
#define MIN_SENSOR_NUM          1
#define MAX_SENSOR_NUM          8

//分站ID的上下限
#define MIN_SENSOR_SITEID       1
#define MAX_SENSOR_SITEID       255

//监听端口
#define SENSOR_DEFAULT_PORT		6666		//ARM连接端口

//连接类型
enum
{
	SENSOR_LINK_TYPE_DIANLIU = 0,	//电流
	SENSOR_LINK_TYPE_PINLV,			//频率
	SENSOR_LINK_TYPE_WUYUAN			//无源触点
};

//端口类型
enum
{
	ANALOG_SENSOR_TYPE = 0,
	SWITCH_SENSOR_TYPE,
	CUMULANT_SENSOR_TYPE,
	UNKNOWN_SENSOR_TYPE,
};

enum
{
	//传感分站ID
	COMM_UP_PAN_ID = 0,
	//传感分站向SensorServer上报配置
	COMM_UP_SENSOR_CONFIG,
	//传感分站向SensorServer上报值、SensorServer向APP上报值
	COMM_UP_SENSOR_VALUE,

	// server下发配置
	COMM_DOWN_SENSOR_CONFIG,
	// server下发传感分站控制模式
	COMM_DOWN_CONTROL_MODE,
	//server下发同步时间
	COMM_DOWN_SYN_TIME,
	//上报版本
	COMM_REPORT_DEV_VERSION,
};

//服务器连接状态
enum
{
	SENSOR_OUTLINK = 0,	//监控分站未连接服务器
	SENSOR_LINK	   = 1,	//监控分站连接服务器
};

//控制模式类型
enum
{
	AUTO_CONTROL = 0,	//自动模式
	MANUAL_OFF_CONTROL,	//手动断电模式
	MANUAL_ON_CONTROL,	//手动复电模式
	SERVER_OFF_CONTROL,	//服务器远程断电模式,跨分站
	SERVER_ON_CONTROL,	//服务器远程复电模式，跨分站
};

//网络数据上报、下发的协议头
struct CommHeader
{
	int tag;
	int len;
};

//分站上报的采集数据结构
struct ST_Collect
{
	//每个端口2个short，第二short代表小数点位置，从左往右
	short  sValue[MAX_SENSOR_NUM * 2];
	//分站ID，1-255
	unsigned char cSiteId;
	//0不报警1报警,低位为端口1
	char cWarn;
	//0不断电1断电
	char cOutage;
	//0不复电1复电
	char cReset;
	//0数据正常1数据异常
	char cAbnormal;
	//0不断线1断线
	char cOutLink;
	//标识该端口有没启动,0未添加1添加
	char cInstalled;
	//保留位
	char cPadding;
	//标识端口传感器类型,0模拟量，1开关量，2累积量
	char cSensorType[MAX_SENSOR_NUM];
	//0自动   1手动断电2手动复电      3服务器远程断电 4服务器远程复电
	char cControlMode[MAX_SENSOR_NUM];
	//采集时间
	time_t tOccTime;
}__PACKED;

//上传传感分站ID
struct ST_Up_SiteID
{
	unsigned char clen; 
	//unsigned char cSiteId;
}__PACKED;


//传感分站配置
struct ST_Down_Config
{
	unsigned char cSiteId;
	unsigned char cPort;
	time_t        tSetTime;//上报时跟数据库配置时间不一致的话则重新下发
	//char *pData;           //下发时的具体配置值，上报时没有这一项
}__PACKED;

//传感分站配置
struct ST_Up_Config
{
	unsigned char cSiteId;
	time_t        tSetTime[8];// 1~8  如果没有配置  时间也为0
}__PACKED;

//下发控制模式
struct ST_Control_Mode
{
	unsigned char cSiteId;		//传感分站ID
	unsigned char cPort;		//传感器端口
	unsigned char cMode;		//传感分站控制模式
}__PACKED;

//上报版本
struct ST_Dev_Version
{
	unsigned short nSiteId;	
	unsigned short nLen;
}__PACKED;

#endif

