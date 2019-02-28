#include "stm32l1xx.h"
#include "cc_def.h"
#include "CommonTypes.h"
#include "spi_nano.h"
#if 0
void SPI_RCC_Configuration(void)
{
    /* Enable the SPI periph */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
}
#endif

void SPI_Configuration(void)
{

#if 1
    SPI_InitTypeDef  SPI_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_PinAFConfig(SPIx_SCK_GPIO_PORT, SPIx_SCK_SOURCE, SPIx_SCK_AF);
    GPIO_PinAFConfig(SPIx_MOSI_GPIO_PORT, SPIx_MOSI_SOURCE, SPIx_MOSI_AF);
    GPIO_PinAFConfig(SPIx_MISO_GPIO_PORT, SPIx_MISO_SOURCE, SPIx_MISO_AF);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;

    /* SPI SCK pin configuration */
    GPIO_InitStructure.GPIO_Pin = SPIx_SCK_PIN;
    GPIO_Init(SPIx_SCK_GPIO_PORT, &GPIO_InitStructure);

    /* SPI  MOSI pin configuration */
    GPIO_InitStructure.GPIO_Pin =  SPIx_MOSI_PIN;
    GPIO_Init(SPIx_MOSI_GPIO_PORT, &GPIO_InitStructure);

    /* SPI MISO pin configuration */
    GPIO_InitStructure.GPIO_Pin = SPIx_MISO_PIN;
    GPIO_Init(SPIx_MISO_GPIO_PORT, &GPIO_InitStructure);

    SPI_I2S_DeInit(SPIx);
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;

if(SystemCoreClock > 24000000L)        //HSE_Value     HSE_VALUE
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
else
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;

    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPIx, &SPI_InitStructure);

    // Enable SPI1
    SPI_Cmd(SPIx, ENABLE);


    //GPIO_ResetBits(GPIOA, GPIO_Pin_4);
#endif
}

void SPI2_Configuration(void)
{

    SPI_InitTypeDef  SPI_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_PinAFConfig(SPIy_SCK_GPIO_PORT, SPIy_SCK_SOURCE, SPIy_SCK_AF);
    GPIO_PinAFConfig(SPIy_MOSI_GPIO_PORT, SPIy_MOSI_SOURCE, SPIy_MOSI_AF);
    GPIO_PinAFConfig(SPIy_MISO_GPIO_PORT, SPIy_MISO_SOURCE, SPIy_MISO_AF);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;

    /* SPI SCK pin configuration */
    GPIO_InitStructure.GPIO_Pin = SPIy_SCK_PIN;
    GPIO_Init(SPIy_SCK_GPIO_PORT, &GPIO_InitStructure);

    /* SPI  MOSI pin configuration */
    GPIO_InitStructure.GPIO_Pin =  SPIy_MOSI_PIN;
    GPIO_Init(SPIy_MOSI_GPIO_PORT, &GPIO_InitStructure);

    /* SPI MISO pin configuration */
    GPIO_InitStructure.GPIO_Pin = SPIy_MISO_PIN;
    GPIO_Init(SPIy_MISO_GPIO_PORT, &GPIO_InitStructure);

    SPI_I2S_DeInit(SPIy);
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;

if(SystemCoreClock > 24000000L)        //HSE_Value     HSE_VALUE
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
else
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;

    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPIy, &SPI_InitStructure);

    // Enable SPI1
    SPI_Cmd(SPIy, ENABLE);


}

void close_spi2_as3933(void)
{
	/* SPI2 Disable */
   SPI_Cmd(SPIy, DISABLE);
   RCC_APB1PeriphClockCmd(SPIy_CLK, DISABLE);

}
//=============================================================================
// SPI Read/Write Routines



/*void SPI_ReadBytes(uint_8 Addr, NtrxBufferPtr Buffer, uint_8 Len)
{
    if ((Len > 0x80) || (Len == 0))
        Len = 1 ;
    SPI_SendByte(Len) ;
    SPI_SendByte(Addr) ;
    while (Len--)
    {
        *Buffer = SPI_SendByte(0xFF) ;
        Buffer ++ ;
    }
}*/

/*void SPI_WriteBytes(uint_8 Addr, NtrxBufferPtr Buffer, uint_8 Len)
{
    if ((Len > 0x80) || (Len == 0))
        Len = 1 ;

    //Ntrx SPI帧的格式
    SPI_SendByte(0x80 | Len) ;      //发送长度最大128bits，最高位是写使能
    SPI_SendByte(Addr) ;

    while (Len--)
    {
        SPI_SendByte(*Buffer) ;
        Buffer++;
    }

    //写完后可以读取结果
}*/

/*void SPI_WriteCodes(uint_8 Addr, const NtrxFlashCode *Ptr, uint_8 Len)
{
    if ((Len > 0x80) || (Len == 0))
        Len = 1 ;
    SPI_SendByte(0x80 | Len) ;
    SPI_SendByte(Addr) ;

    while (Len--)
        SPI_SendByte(NtrxReadFlash(Ptr++)) ;
}*/

