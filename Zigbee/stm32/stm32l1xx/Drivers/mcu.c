#include "stm32l1xx.h"
#include "mcu.h"
#include "spi_nano.h"
#include "sleep.h"
#include "timer_event.h"
#include "hal_io_i2c.h"
#include "led.h"
#include "port.h"
#include "CC_DEF.h"
// nano radio interruptextern Bool isMoving
extern Bool isMoving;

void EXTI0_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line0) != RESET)
    {
        /* Clear the EXTI line 1 pending bit */

        process_deca_irq();

        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}

void EXTI1_IRQHandler(void)
{
    // help key interrupt
    if (EXTI_GetITStatus(EXTI_Line1) != RESET)
    {
		
		isMoving = True;
		event_timer_set(EVENT_MMA8452Q_EVENT);
        /* Clear the EXTI line 1 pending bit */
			
        EXTI_ClearITPendingBit(EXTI_Line1);
    }
}

void EXTI3_IRQHandler(void)    //help
{
    // help key interrupt
    if (EXTI_GetITStatus(EXTI_Line3) != RESET)
    {
    	isMoving = True;
        event_timer_set(EVENT_BUTTON_MSG);

        /* Clear the EXTI line 15 pending bit */
        EXTI_ClearITPendingBit(EXTI_Line3);
    }
}



void EXTI9_5_IRQHandler(void)    //确认
{
    // confirm key interrupt
    if (EXTI_GetITStatus(EXTI_Line6) != RESET)
    {
				isMoving = True;
        event_timer_set(EVENT_BUTTON_MSG);

        /* Clear the EXTI line 5 pending bit */
        EXTI_ClearITPendingBit(EXTI_Line6);
    }
		
		if (EXTI_GetITStatus(EXTI_Line9) != RESET)
    {
        event_timer_set(EVENT_EXCIT_EVENT);

        /* Clear the EXTI line 5 pending bit */
        EXTI_ClearITPendingBit(EXTI_Line9);
    }
}


static void NVIC_Configuration(void)
{
    typedef void (*__IsrFunction)(void) ;
    extern __IsrFunction __Vectors[] ;        // 在stm32f10x_vector.s文件中定义
#ifdef  VECT_TAB_RAM
    /* Set the Vector Table base location at 0x20000000 */
    NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);
#else  /* VECT_TAB_FLASH  */
    /* Set the Vector Table base location at 0x08000000 */
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
    //NVIC_SetVectorTable( NVIC_VectTab_FLASH, (uint32_t)(&__Vectors[0]) - NVIC_VectTab_FLASH ) ;
#endif

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
}

/**
  * @brief  Configures the different system clocks.
  * @param  None
  * @retval : None
  */
static void RCC_Configuration(void)
{

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
    //RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);
    //RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOE, ENABLE);

    //RCC_APB2PeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    /* Enable the SPI periph */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
    /* Enable SCK, MOSI and MISO GPIO clocks */
    // RCC_AHBPeriphClockCmd(SPIx_SCK_GPIO_CLK | SPIx_MISO_GPIO_CLK | SPIx_MOSI_GPIO_CLK, ENABLE);
    /* Enable SYSCFG clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM9, ENABLE);
	/* Enable ADC1 clock */
    //RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

}

enum {
    HW_IWDG = 0,    // IWDG硬件看门狗特性，系统加电后自动启动
    SW_IWDG,
};

static void _config_iwdg(int iwdg_config)
{
    uint8_t ob_iwdg = 0;
    uint8_t ob_stop = 0;
    uint8_t ob_stdby = 0;
    uint8_t user_opt = 0;
    uint8_t changed = 0;

    FLASH_OB_Unlock();

    user_opt = FLASH_OB_GetUser();

    if (iwdg_config == HW_IWDG) {
        if (user_opt & 0x1) {
            ob_iwdg = OB_IWDG_HW;
            changed = 1;
        }
    } else if (iwdg_config == SW_IWDG){
        if (!(user_opt & 0x1)) {
            ob_iwdg = OB_IWDG_SW;
            changed = 1;
        }
    }

    if (changed) {
        if (user_opt & 0x2) {
            ob_stop = OB_STOP_RST;
        } else {
            ob_stop = OB_STOP_NoRST;
        }
        if (user_opt & 0x4) {
            ob_stdby = OB_STDBY_RST;
        } else {
            ob_stdby = OB_STDBY_NoRST;
        }
        while (FLASH_OB_UserConfig(ob_iwdg, ob_stop, ob_stdby) != FLASH_COMPLETE);
        FLASH_OB_Launch();
    }

    FLASH_OB_Lock();
}

void set_flash_read_protection(void)
{

    #if 0
#ifdef USING_CODE_SECURITY
    FLASH_Unlock();
    FLASH_OB_Unlock();

    if (FLASH_OB_GetRDP() != SET)
    {
        while (FLASH_OB_RDPConfig(OB_RDP_Level_1) != FLASH_COMPLETE);

        FLASH_OB_Launch();
    }

    FLASH_OB_Lock();
    FLASH_Lock();
#endif
#endif
}

void unset_flash_read_protection(void)
{
    FLASH_Unlock();
    FLASH_OB_Unlock();

    if (FLASH_OB_GetRDP() != RESET)
    {
        while (FLASH_OB_RDPConfig(OB_RDP_Level_0) != FLASH_COMPLETE);
        FLASH_OB_Launch();
    }
}

static void IWDG_Configuration(void)
{
  
#ifdef WATCHDOG_ENABLE

    RCC_LSICmd(ENABLE);
    /* Wait till LSE is ready */
    while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
    {
    }

#ifdef CONFIG_IWDG_HW
    _config_iwdg(HW_IWDG);
#else
    _config_iwdg(SW_IWDG);
#endif

    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    IWDG_SetPrescaler(IWDG_Prescaler_256);
    IWDG_SetReload(0x0FFF);//28sec
    IWDG_ReloadCounter();
    IWDG_Enable();
#endif
}

/*******************************************************************************
 * Function Name  : SysTick_Configuration
 * Description    : Configures the SysTick for OS tick.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void SysTick_Configuration(void)
{
    RCC_ClocksTypeDef   rcc_clocks;
    uint_32              cnts;

    RCC_GetClocksFreq(&rcc_clocks);

    cnts = (uint_32)rcc_clocks.HCLK_Frequency / (BASE_TIME_TICK);

    SysTick_Config(cnts);
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
}

/*******************************************************************************
*   chip core init
*******************************************************************************/
void chip_core_init(void)
{
    NVIC_Configuration();
    SysTick_Configuration();
    IWDG_Configuration();
    //set_flash_read_protection();
    SleepInit();
}

void chip_interface_init(void)
{
    //开启所有功能的RCC时钟
    RCC_Configuration();

    //UART_Configuration();


    HAL_I2C_Init();
}

void chip_lowpower_ready(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* SPI Disable */
    SPI_Cmd(SPIx, DISABLE);
    RCC_APB2PeriphClockCmd(SPIx_CLK, DISABLE);

	
    //port_SPIx_clear_chip_select();	//CS low
    //GPIO_ResetBits(DW1000_RSTn_GPIO, DW1000_RSTn);
	// Enable GPIO used for DW1000 reset
	/*GPIO_InitStructure.GPIO_Pin = DW1000_RSTn;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
	GPIO_Init(DW1000_RSTn_GPIO, &GPIO_InitStructure);
	//drive the RSTn pin low
	GPIO_ResetBits(DW1000_RSTn_GPIO, DW1000_RSTn);*/

	/*GPIO_InitStructure.GPIO_Pin = SPIx_MISO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
	GPIO_Init(SPIx_MISO_GPIO_PORT, &GPIO_InitStructure);
	GPIO_ResetBits(SPIx_MISO_GPIO_PORT, SPIx_MISO_PIN);
	

    /*UART2 Disable*/
    USART_Cmd(USART2, DISABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, DISABLE);


    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;

    GPIO_ResetBits(GPIOC,GPIO_Pin_13);    //PA Disable
    GPIO_SetBits(GPIOB,GPIO_Pin_9);    //TCXO disable

    GPIO_ResetBits(GPIOB, GPIO_Pin_4);  //AD stop

    /* SPI SCK pin configuration */
    //GPIO_InitStructure.GPIO_Pin = SPIx_SCK_PIN | SPIx_MOSI_PIN | SPIx_MISO_PIN;
    //GPIO_Init(SPIx_SCK_GPIO_PORT, &GPIO_InitStructure);

    /* SPI  MOSI pin configuration */
    // GPIO_InitStructure.GPIO_Pin =  SPIx_MOSI_PIN;
    // GPIO_Init(SPIx_MOSI_GPIO_PORT, &GPIO_InitStructure);

    /* SPI MISO pin configuration */
    // GPIO_InitStructure.GPIO_Pin = SPIx_MISO_PIN;
    // GPIO_Init(SPIx_MISO_GPIO_PORT, &GPIO_InitStructure);

    //GPIO_InitStructure.GPIO_Pin = NTRX_UCIRQ_PIN;
    //GPIO_Init(NTRX_UCIRQ_PORT, &GPIO_InitStructure);

    // GPIO_InitStructure.GPIO_Pin = NTRX_RST_PIN;
    // GPIO_Init(NTRX_RST_PORT, &GPIO_InitStructure);

    //GPIO_InitStructure.GPIO_Pin = NTRX_UCRESET_PIN;
    //GPIO_Init(NTRX_UCRESET_PORT, &GPIO_InitStructure);

    // GPIO_ResetBits(NTRX_RST_PORT, NTRX_RST_PIN);
    //GPIO_ResetBits(NTRX_UCIRQ_PORT, NTRX_UCIRQ_PIN);
    //GPIO_ResetBits(NTRX_UCRESET_PORT, NTRX_UCRESET_PIN);

    //GPIO_ResetBits(SPIx_SCK_GPIO_PORT, SPIx_SCK_PIN);
    //GPIO_ResetBits(SPIx_MOSI_GPIO_PORT, SPIx_MOSI_PIN);
    //GPIO_ResetBits(SPIx_MISO_GPIO_PORT, SPIx_MISO_PIN);


    /* TIM9 Disable */
    //TIM_Cmd(TIM9, DISABLE);
    //RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM9, DISABLE);

    /* Disable the HSI oscillator */
    //RCC_HSICmd(DISABLE);
    /* Disable ADC1 */
    //ADC_Cmd(ADC1, DISABLE);
    //ADC_DeInit(ADC1);
}

void chip_lowpower_finish(void)
{
    // GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(SPIx_CLK, ENABLE);
    SPI_Cmd(SPIx, ENABLE);

    /*UART2 Enable*/
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    USART_Cmd(USART2, ENABLE);

    //SPI_Configuration();

    //GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    //GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
    //GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    //GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;

    /* SPI SCK pin configuration */
    //GPIO_InitStructure.GPIO_Pin = SPIx_SCK_PIN | SPIx_MOSI_PIN | SPIx_MISO_PIN;
    //GPIO_Init(SPIx_SCK_GPIO_PORT, &GPIO_InitStructure);

    GPIO_SetBits(GPIOC,GPIO_Pin_13);    //PA enable
    GPIO_ResetBits(GPIOB,GPIO_Pin_9);    //TCXO enable

    GPIO_SetBits(GPIOB, GPIO_Pin_4);  //AD start
}

void WatchdogReset(void)
{
#ifdef WATCHDOG_ENABLE
    IWDG_ReloadCounter();
#endif
}

__ASM uint_32 interrupt_disable(void)
{
    MRS     r0, PRIMASK
    CPSID   I
    BX      LR
}

__ASM void interrupt_enable(uint_32 reg)
{
    MSR     PRIMASK, r0
    BX      LR
}

static uint_32 systicks;

/*void SysTick_Handler(void)              //中断处理函数
{
    systicks++;
}*/

void SysTick_Handler(void)
{
	systicks++;

}

void portResetTickCnt(void)
{
	systicks = 0;
}



void portSetTickCnt(uint_32 tick)

{
    systicks = tick;
}

uint_32 portGetTickCnt(void)
{
	return systicks;
}


uint_32 GetSysClock(void)
{
    return systicks;
}

void ElapsedSysClock(uint_32 tick)
{
    systicks += tick;
}

/**
 * This fucntion returns milliseconds since system passed
 */
static uint_32 _get_millisecond(void)
{
	uint_32 tick;
	uint_32 value;

#define TICK_MS (1000/BASE_TIME_TICK)

	tick = systicks;
	value = tick * TICK_MS + (SysTick->LOAD - SysTick->VAL) * TICK_MS / SysTick->LOAD;

	return value;
}

/**
 * This fucntion returns microseconds since system passed
 */
static uint_32 _get_microsecond(void)
{
	uint_32 tick;
	uint_32 value;

#define TICK_US	(1000000/BASE_TIME_TICK)

	tick = systicks;
	value = tick * TICK_US + (SysTick->LOAD - SysTick->VAL) * TICK_US / SysTick->LOAD;

	return value;
}


void Delay_ms(uint_16 ms)
{
    uint_32 s ;
    uint_32 value;
    uint_32 t = ms;

    s = _get_millisecond();

    while (1)
    {
        value = _get_millisecond() - s;
        if (value >= t)
            return;
    }
}

void Delay_us(uint_16 us)
{
    uint_32 s ;
    uint_32 value;
    uint_32 t = us;

    s = _get_microsecond();

    while (1)
    {
        value = _get_microsecond() - s;
        if (value >= t)
            return;
    }
}

