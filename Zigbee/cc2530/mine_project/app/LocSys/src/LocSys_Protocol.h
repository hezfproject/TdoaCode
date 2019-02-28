#ifndef LOCSYS_PROTOCOL_H
#define LOCSYS_PROTOCOL_H

#define MAX_SPI_PAYLOAD_SIZE                    85   //this is identical to MAX_DATA_SIZE in AppProtocol.h
#define LOCSYS_MAX_DATA_LEN                     (MAX_SPI_PAYLOAD_SIZE - sizeof(app_LocSys_Data_t))
#define LOCSYS_MAX_CMD_LEN                      (MAX_SPI_PAYLOAD_SIZE - sizeof(app_LocSys_Cmd_t))

#define LOCSYS_MAX_CARD_STATUS_RECORDS          ((LOCSYS_MAX_DATA_LEN - sizeof(LocSys_Data_CardStatusRecords_t)) / sizeof(cardStatusRecord_t))
#define LOCSYS_MAX_CARD_CMD_RECORDS             100

/* high 4 bits is major version, low 4 bits is minor version*/
#define LOCSYS_CARD_VERSION             0x10
#define LOCSYS_ROUTER_VERSION           0x10
#define LOCSYS_STATION_VERSION          0x10
#define LOCSYS_HANDHELD_VERSION         0x00

#define LOCSYS_MAX_ADDRESS              0xFEFF
#define LOCSYS_MAX_CARDTYPE             0xF0
#define LOCSYS_CARDTYPE_ALL             0xFF
#define LOCSYS_CARDTYPE_DEFAULT         0x0

#define LOCSYS_MAX_CARDSTAGE            3

#define LOCSYS_MAX_CARDDESC_LEN         50

enum{
  LOCSYS_DATA_CARD_STATUS               = 0x00,
  LOCSYS_DATA_CARD_INFO                 = 0x01,
  LOCSYS_DATA_ROUTER_PATH               = 0x02,

  LOCSYS_DATA_SEARCH_CARD               = 0x10,
  LOCSYS_DATA_SET_CARD_INFO             = 0x1F,

  LOCSYS_DATA_SEARCH_CARD_ALERT         = 0x21,
  LOCSYS_DATA_SEARCH_CARD_CANCEL_ALERT  = 0x22,
  LOCSYS_DATA_SEARCH_CARD_QUERY_INFO    = 0x23,
  LOCSYS_DATA_SEARCH_CARD_CHANGE_STAGE  = 0x24,

  LOCSYS_DATA_CARD_INFO_ACK             = 0x81,

  LOCSYS_DATA_TUNNELLING                = 0xF0,
  LOCSYS_DATA_TUNNELLING2               = 0xF1,
};

typedef struct{
  uint8     lowpower:1;
  uint8     stage:2;
  uint8     reserved:4;
  uint8     hallactive:1;
}cardstatus_t;

typedef struct
{
  uint16        srcAddr;
  int8          rssi;
  cardstatus_t  status;
} cardStatusRecord_t;
typedef struct
{
  union{
    uint16  dstAddr;
    struct{
      uint8 type;
      uint8 bytype;
    };
  };
  uint8 cmdtype;
  uint8 param;
} cardSearchRecord_t;

typedef struct{
  app_LocSys_Cmd_t  hdr;
  uint8             cmdtype;
}LocSys_CmdHdr_t;

typedef struct{
  app_LocSys_Data_t hdr;
  uint8             datatype;
}LocSys_DataHdr_t;

typedef struct{
  uint8                 datatype;
  uint8                 cnt;
  cardSearchRecord_t    records[0];
}LocSys_Data_SearchCardRecords_t;

typedef struct{
  uint8                 datatype;
  uint8                 cnt;
  cardStatusRecord_t    records[0];
}LocSys_Data_CardStatusRecords_t;

typedef struct{
  uint8                 datatype;
  uint8                 cmdtype;
  uint8                 param;
}LocSys_Data_SearchCard_t;

typedef struct{
  uint8                 datatype;
  cardstatus_t          status;
  uint8                 type;
}LocSys_Data_CardStatus_t;

typedef struct{
  uint8                 datatype;
  uint8                 version;
  uint16                cardaddress;
  cardstatus_t          cardstatus;
  uint8                 cardtype;
  uint8                 desclen;
  uint8                 desc[0];
}LocSys_Data_CardInfo_t;

typedef struct{
  uint8                 datatype;
  uint16                cardaddress;
}LocSys_Data_CardInfoAck_t;

typedef struct{
  uint8                 datatype;
  uint8                 depth;
  uint16                path[0];
}LocSys_Data_RouterPath_t;

typedef struct{
  uint8                 datatype;
  uint8                 len;
  uint8                 desc[0];
}LocSys_Data_SetCardDesc_t;

typedef struct{
  uint8                 datatype;
  uint8                 cardstage;
}LocSys_Data_SetCardStage_t;

typedef struct{
  uint8                 datatype;
  uint8                 cardtype;
}LocSys_Data_SetCardType_t;

typedef struct{
  uint8                 datatype;
  uint8                 setstage:1;
  uint8                 settype:1;
  uint8                 setdesc:1;
  uint8                 dummy:5;
  uint8                 stage;
  uint8                 type;
  uint8                 desclen;
  uint8                 desc[0];
}LocSys_Data_SetCardInfo_t;

typedef struct{
  uint8                 datatype;
  uint8                 payload[0];
}LocSys_Data_Tunnelling_t;
#endif
