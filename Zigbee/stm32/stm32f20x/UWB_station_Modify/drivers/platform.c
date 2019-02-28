#include <rtthread.h>
#include "board.h"

#ifdef RT_USING_RTC
#include "stm32f2_rtc.h"
#endif /* RT_USING_RTC */

#ifdef RT_USING_LWIP
extern void rt_hw_stm32_eth_init(void);
#endif

#ifdef RT_USING_SPI
#include "stm32f20x_40x_spi.h"
#include "spi_flash_w25qxx.h"

/*
SPI2_MOSI: PB15
SPI2_MISO: PB14
SPI2_SCK : PB13

CS0: PG10  SPI FLASH
CS1: PB12  TOUCH
CS2: PG7   WIFI
*/
static void rt_hw_spi2_init(void)
{
    /* register spi bus */
    {
        static struct stm32_spi_bus stm32_spi;
        GPIO_InitTypeDef GPIO_InitStructure;

        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

        /*!< SPI SCK pin configuration */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
        GPIO_Init(GPIOB, &GPIO_InitStructure);
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
        GPIO_Init(GPIOC, &GPIO_InitStructure);

        /* Connect alternate function */
        GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_SPI2);
        GPIO_PinAFConfig(GPIOC, GPIO_PinSource2, GPIO_AF_SPI2);
        GPIO_PinAFConfig(GPIOC, GPIO_PinSource3, GPIO_AF_SPI2);

        stm32_spi_register(SPI2, &stm32_spi, "spi2");
    }

    /* attach cs */
    {
        static struct rt_spi_device spi_device;
        static struct stm32_spi_cs  spi_cs;

        GPIO_InitTypeDef GPIO_InitStructure;

        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

        /* spi21: PG10 */
        spi_cs.GPIOx = GPIOB;
        spi_cs.GPIO_Pin = GPIO_Pin_9;
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

        GPIO_InitStructure.GPIO_Pin = spi_cs.GPIO_Pin;
        GPIO_SetBits(spi_cs.GPIOx, spi_cs.GPIO_Pin);
        GPIO_Init(spi_cs.GPIOx, &GPIO_InitStructure);

        rt_spi_bus_attach_device(&spi_device, "spi20", "spi2", (void*)&spi_cs);
    }
}

static void rt_hw_spi1_init(void)
{
    /* register spi bus */
    {
        static struct stm32_spi_bus stm32_spi;
        GPIO_InitTypeDef GPIO_InitStructure;

        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

        /*!< SPI SCK pin configuration */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
        GPIO_Init(GPIOA, &GPIO_InitStructure);
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
        GPIO_Init(GPIOB, &GPIO_InitStructure);

        /* Connect alternate function */
        GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);
        GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_SPI1);
        GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_SPI1);

        stm32_spi_register(SPI1, &stm32_spi, "spi1");
    }

    /* attach cs */
    {
        static struct rt_spi_device spi_device;
        static struct stm32_spi_cs  spi_cs;

        GPIO_InitTypeDef GPIO_InitStructure;

        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

        /* spi21: PG10 */
        spi_cs.GPIOx = GPIOA;
        spi_cs.GPIO_Pin = GPIO_Pin_4;
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

        GPIO_InitStructure.GPIO_Pin = spi_cs.GPIO_Pin;
        GPIO_SetBits(spi_cs.GPIOx, spi_cs.GPIO_Pin);
        GPIO_Init(spi_cs.GPIOx, &GPIO_InitStructure);

        rt_spi_bus_attach_device(&spi_device, "spi10", "spi1", (void*)&spi_cs);
    }
}

#endif /* RT_USING_SPI */

void rt_platform_init(void)
{
#ifdef RT_USING_SPI
    rt_hw_spi2_init();
    rt_hw_spi1_init();
#ifdef RT_USING_DFS
    w25qxx_init("flash0", "spi20");
#endif /* RT_USING_DFS */

#ifdef RT_USING_RTGUI
    /* initilize touch panel */
    rtgui_touch_hw_init("spi21");
#endif /* RT_USING_RTGUI */
#endif /* RT_USING_SPI */

#ifdef RT_USING_USB_HOST
    /* register stm32 usb host controller driver */
    rt_hw_susb_init();
#endif

#ifdef RT_USING_DFS
    /* initilize sd card */
    //rt_hw_sdcard_init();
#endif /* RT_USING_DFS */

#ifdef RT_USING_RTGUI
    /* initilize ra8875 lcd controller */
    ra8875_init();

    /* initilize key module */
    rt_hw_key_init();
#endif /* RT_USING_RTGUI */

#ifdef RT_USING_RTC
    rt_hw_rtc_init();
#endif /* RT_USING_RTC */

#ifdef RT_USING_LWIP
    rt_hw_stm32_eth_init();
#endif

    rt_thread_delay(50);
    rt_device_init_all();
}

