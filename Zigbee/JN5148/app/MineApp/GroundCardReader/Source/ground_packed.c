#include <jendefs.h>
#include "CRC.h"
#include "system_util.h"
#include "string.h"
#include "ground_crsys.h"

PUBLIC uint8 packed_card_loc(uint8 * const pu8Buf,
                             const uint8* const pCard_data,
                            uint16 u16CopyLen)
{
    ASSERT_RETV(pu8Buf != NULL && pCard_data != NULL && u16CopyLen > 0, 0);

    memcpy(pu8Buf, pCard_data, u16CopyLen);

    return u16CopyLen;
}

PUBLIC uint8 packed_card_info(MAC_RxFrameData_s const * const psFrame,
                            uint8 * const pu8Buf, uint16 size)
{
    RADIO_DEV_INFO_T* pstCardInfo = (RADIO_DEV_INFO_T*)(psFrame->au8Sdu);
    LPBSS_devicecard_info_t* pstDstCardInfo;
    uint16 u16DescLen;

    ASSERT_RETV(psFrame != NULL && pu8Buf != NULL, 0);

    u16DescLen = pstCardInfo->stPayload.stDescInfo.u16Len;

    CONVERT_ENDIAN(u16DescLen);

    if (u16DescLen + sizeof(LPBSS_devicecard_info_t) > size)
    {
        DBG_WARN("Err: CardInfo Lenth: %d\n", pstCardInfo->u8Len);
        return 0;
    }

    // packed ready
    pstDstCardInfo = (LPBSS_devicecard_info_t *)(pu8Buf);

    pstDstCardInfo->u16ShortAddr = psFrame->sSrcAddr.uAddr.u16Short;
    pstDstCardInfo->u16Seqnum = pstCardInfo->u16Seqnum;
    pstDstCardInfo->u8Device = DEVICE_CARD_DEVICE_ID;
    pstDstCardInfo->u8IsChangeInfo = pstCardInfo->u8IsChange;
    pstDstCardInfo->u16CRC = CRC16(((uint8*)pstCardInfo) + 5,
                                    pstCardInfo->u8Len, 0xFFFF);
    pstDstCardInfo->DevCardInfo.u8WorkType
        = pstCardInfo->stPayload.stBasicInfo.u8WorkType;
    pstDstCardInfo->DevCardInfo.u8padding1 = 0;
    pstDstCardInfo->DevCardInfo.u8padding2 = 0;

    CONVERT_ENDIAN(pstDstCardInfo->u16ShortAddr);
    CONVERT_ENDIAN(pstDstCardInfo->u16CRC);
    pstDstCardInfo->DevCardInfo.u8Len = u16DescLen;

    memcpy((uint8*)(pstDstCardInfo+1), pstCardInfo->stPayload.stDescInfo.u8Desc,
            u16DescLen);

    return u16DescLen + sizeof(LPBSS_devicecard_info_t);
}

PUBLIC uint8 packed_card_ver(MAC_RxFrameData_s const* const psFrame,
                            uint8 * const pu8Buf, uint16 size)
{
    CARD_VERSION_INFO_T* pstCardVer = (CARD_VERSION_INFO_T*)psFrame->au8Sdu;
    LPBSS_card_ver_t* pstVer;

    ASSERT_RETV(psFrame != NULL && pu8Buf != NULL, 0);

    if (sizeof(LPBSS_card_ver_t) + pstCardVer->u8VerInfoLen > size)
    {
        DBG_WARN("Err: version Lenth: %d\n", pstCardVer->u8VerInfoLen);
        return 0;
    }

    pstVer = (LPBSS_card_ver_t *)pu8Buf;

    pstVer->u16ShortAddr = psFrame->sSrcAddr.uAddr.u16Short;
    pstVer->u8Device = pstCardVer->u8DevType;
    pstVer->u8Len = pstCardVer->u8VerInfoLen;
    pstVer->u16padding = 0;
    pstVer->u8VerOffset = pstCardVer->u8VerOffset;
    pstVer->u8ReleaseOffset = pstCardVer->u8ReleaseOffset;

    CONVERT_ENDIAN(pstVer->u16ShortAddr);

    memcpy(pstVer + 1, pstCardVer->pu8VerInfo, pstCardVer->u8VerInfoLen);

    return sizeof(LPBSS_card_ver_t) + pstCardVer->u8VerInfoLen;
}

