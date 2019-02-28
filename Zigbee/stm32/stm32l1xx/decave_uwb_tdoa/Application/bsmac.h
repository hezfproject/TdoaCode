#ifndef BSMAC_H
#define BSMAC_H

void UwbWriteDataToStm32(uint8* pPackBuf, uint16 u16PackLen);

void UwbBsmacBuildPacketMacHead(UWB_BSMAC_PACKET_HEADER_S *pstBsmacHead, 
								 uint16 u16DataLen, 
								 uint8 u8FrameType);

void UwbBsmacBuildPacketNetHead(UWB_NET_PACKET_HEADER_S *pstNetHead, 
                                 uint16 u16OwnAddress,
                                 uint16 u16DataLen, 
                                 uint16 u16NetType);

void UwbBsmacBuildPacketData(uint8* pAppData, 
							  uint16 u16DataLen, 
							  uint8  u8FrameType,
							  UWB_BSMAC_BUILD_PACK_S *pstBsmacPack,
							  uint16 *pu16DataTxLen);

uint8 UwbBsmacSendPacketProc( uint8 * pAppUwbData,
                            uint16  u16OwnAddress,
                            uint16  u16UwbDataLen,
                            uint8   u8FrameType);

void UwbBsmacBlidPacketHeadProc(uint8 u8ProtocolType, uint8 u8MsgType, uint16 u16SendDataLen, 
                                APP_HEADER_S* pstAppHead);
#endif


