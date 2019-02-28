#ifndef _GROUND_PACKED_H_
#define _GROUND_PACKED_H_

PUBLIC uint8 packed_card_loc(uint8 * const pu8Buf,
                            uint8 const * const pCard_data,
                            uint16 u16CopyLen);

PUBLIC uint8 packed_card_info(MAC_RxFrameData_s const * const psFrame,
                            uint8 * const pu8Buf, uint16 size);

PUBLIC uint8 packed_card_ver(MAC_RxFrameData_s const * const psFrame,
                            uint8 * const pu8Buf, uint16 size);


#endif

