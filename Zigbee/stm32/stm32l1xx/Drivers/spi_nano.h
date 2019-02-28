#ifndef _SPI_NANO_H_
#define _SPI_NANO_H_

/* SPIx Communication boards Interface */
#define SPIx                           SPI1
#define SPIx_CLK                       RCC_APB2Periph_SPI1
#define SPIx_IRQn                      SPI1_IRQn
#define SPIx_IRQHANDLER                SPI1_IRQHandler

#define SPIx_SCK_PIN                   GPIO_Pin_5
#define SPIx_SCK_GPIO_PORT             GPIOA
#define SPIx_SCK_GPIO_CLK              RCC_AHBPeriph_GPIOA
#define SPIx_SCK_SOURCE                GPIO_PinSource5
#define SPIx_SCK_AF                    GPIO_AF_SPI1

#define SPIx_MISO_PIN                  GPIO_Pin_7
#define SPIx_MISO_GPIO_PORT            GPIOA
#define SPIx_MISO_GPIO_CLK             RCC_AHBPeriph_GPIOA
#define SPIx_MISO_SOURCE               GPIO_PinSource7
#define SPIx_MISO_AF                   GPIO_AF_SPI1

#define SPIx_MOSI_PIN                  GPIO_Pin_6
#define SPIx_MOSI_GPIO_PORT            GPIOA
#define SPIx_MOSI_GPIO_CLK             RCC_AHBPeriph_GPIOA
#define SPIx_MOSI_SOURCE               GPIO_PinSource6
#define SPIx_MOSI_AF                   GPIO_AF_SPI1


/* SPIy Communication boards Interface */
#define SPIy                           SPI2
#define SPIy_CLK                       RCC_APB1Periph_SPI2
#define SPIy_IRQn                      SPI2_IRQn
#define SPIy_IRQHANDLER                SPI2_IRQHandler

#define SPIy_CS_PIN                   GPIO_Pin_12
#define SPIy_CS_GPIO_PORT             GPIOB
#define SPIy_CS_GPIO_CLK              RCC_AHBPeriph_GPIOB
#define SPIy_CS_SOURCE                GPIO_PinSource12
#define SPIy_CS_AF                    GPIO_AF_SPI2

#define SPIy_SCK_PIN                   GPIO_Pin_13
#define SPIy_SCK_GPIO_PORT             GPIOB
#define SPIy_SCK_GPIO_CLK              RCC_AHBPeriph_GPIOB
#define SPIy_SCK_SOURCE                GPIO_PinSource13
#define SPIy_SCK_AF                    GPIO_AF_SPI2

#define SPIy_MISO_PIN                  GPIO_Pin_14
#define SPIy_MISO_GPIO_PORT            GPIOB
#define SPIy_MISO_GPIO_CLK             RCC_AHBPeriph_GPIOB
#define SPIy_MISO_SOURCE               GPIO_PinSource14
#define SPIy_MISO_AF                   GPIO_AF_SPI2

#define SPIy_MOSI_PIN                  GPIO_Pin_15
#define SPIy_MOSI_GPIO_PORT            GPIOB
#define SPIy_MOSI_GPIO_CLK             RCC_AHBPeriph_GPIOB
#define SPIy_MOSI_SOURCE               GPIO_PinSource15
#define SPIy_MOSI_AF                   GPIO_AF_SPI2

//void SPI_RCC_Configuration(void);

void SPI_Configuration(void);
void SPI2_Configuration(void);
void close_spi2_as3933(void);
#endif

