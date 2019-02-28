#ifndef _FPGA_STRU_H__
#define _FPGA_STRU_H__

/*********************************************************
** Debug level(light, middle, high)
*********************************************************/
// debug level
#define DEBUG_LEVEL_NO					(0)
#define DEBUG_LEVEL_LI					(1)
#define DEBUG_LEVEL_ME					(2)
#define DEBUG_LEVEL_HI					(3)
#define DEBUG_LEVEL						(DEBUG_LEVEL_LI)

/***********************************************************
** Packet length
***********************************************************/
// maximum mac payload length
#define MAX_PACKET_LEN			(1023)
// maximum packet length for communication with com-module
#define MAX_PACKET_LEN_COM		(114)

/************************************************************
** fpga communication channel define
************************************************************/
#define FPGA_CHANNEL_0					(0)
#define FPGA_CHANNEL_1					(1)
#define FPGA_CHANNEL_2					(2)
#define FPGA_CHANNEL_3					(3)
#define FPGA_CHANNEL_4					(4)
#define FPGA_CHANNEL_5					(5)
#define FPGA_CHANNEL_6					(6)
#define FPGA_CHANNEL_7					(7)
#define FPGA_CHANNEL_8					(8)
#define FPGA_CHANNEL_9					(9)
#define FPGA_CHANNEL_10					(10)
#define FPGA_CHANNEL_11					(11)
#define FPGA_CHANNEL_ALL				(12)

/************************************************************
** status define
************************************************************/
#define FPGA_CHAN_NOTALIVE				(0)
#define FPGA_CHAN_ALIVE					(1)

#define FPGA_INT_LOW					(0)
#define FPGA_INT_HIGH					(1)

#define FPGA_OFF_LINE					(0)
#define FPGA_ON_LINE					(1)

/************************************************************
 * Device type
 ***********************************************************/
#define DEVICE_TYPE_BS_EP	(0)
#define DEVICE_TYPE_BS_OP	(1)
#define DEVICE_TYPE_COM		(2)
#define DEVICE_TYPE_LOC		(3)
#define DEVICE_TYPE_NONE	(4)

/************************************************************
** Control packet struct define
************************************************************/
typedef struct _TGetWriteCount {
	unsigned int count;
}TGetWriteCount, *PGETWRITECOUNT;

typedef struct _TGetReadCount {
	unsigned int count;
}TGetReadCount, *PGETREADCOUNT;

typedef struct _TGetLiveSta {
	unsigned int sta;
}TGetLiveSta, *PGETLIVESTA;

typedef struct _TGetIntSta {
	unsigned int sta;
}TGetIntSta, *PGETINTSTA;

typedef struct _TGetDestType {
	unsigned int type;
}TGetDestType, *PGETDESTTYPE;

typedef struct _TGetDestAddr {
	unsigned int addr;
}TGetDestAddr, *PGETDESTADDR;

typedef struct _TGetRdIntFlag {
	unsigned int flag;
}TGetRdIntFlag, *PGETRDINTFLAG;

typedef struct _TGetWrIntFlag {
	unsigned int flag;
}TGetWrIntFlag, *PGETWRINTFLAG;

typedef struct _TDetectDev {
	unsigned int sta;
}TDetectDev, *PDETECTDEV;

typedef struct _TSetFlowCtl 
{
	unsigned int flag;
} TSetFlowCtl, *PSETFLOWCTL;

typedef struct _TSetDebugPort {
	unsigned int chan;
}TSetDebugPort, *PSETDEBUGPORT;

/** control ID **/
#define	FPGA_IOCTL_WRITE				(0)
#define	FPGA_IOCTL_READ					(1)
#define FPGA_IOCTL_GET_CHAN_STATUS		(2)
#define FPGA_IOCTL_SET_MACADDR			(3)
#define FPGA_IOCTL_SET_DEBUGPORT		(4)

#define TS_MAX_PACKET_SIZE	1024
struct tspacket
{
	unsigned int channel;	/* channel must be the first element!! */
	unsigned int len;
	char data[TS_MAX_PACKET_SIZE];
};

struct fpgaio_rd
{
	unsigned int ipnum;
	struct tspacket __user *packets;
};

struct fpgaio_wr
{
	unsigned int channel;
	unsigned int len;
	char __user *buf;
};

struct fpgaio_stat
{
	unsigned int channel;
	unsigned int wrcount;
	unsigned int rdcount;
	unsigned int livestat;
	unsigned int intstat;
	unsigned int totalcnt;
	unsigned int lostcnt;
	unsigned short dsttype;
	unsigned short dstaddr;
};

struct fpgaio_macaddr
{
	unsigned int addr;
};

struct fpgaio_dbgport
{
	unsigned int channel;
};

typedef struct _TGetFrameTotalCnt {
		unsigned int count;
}TGetFrameTotalCnt, *PGETFRAMETOTALCNT;

typedef struct _TGetFrameCrcCnt {
		unsigned int count;
}TGetFrameCrcCnt, *PGETFRAMECRCCNT;

typedef struct _TGetFrameLostCnt {
		unsigned int count;
}TGetFrameLostCnt, *PGETFRAMELOSTCNT;

#endif	// _FPGA_STRU_H__

