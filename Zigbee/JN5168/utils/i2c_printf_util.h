#ifndef I2C_PRINTF_UTIL_H_
#define I2C_PRINTF_UTIL_H_

/****************************************************************************/
/***              Variables                                               ***/
/****************************************************************************/
typedef enum
{
    BR9600,
    BR300,
    BR600,
    BR900,
    BR1200,
    BR1800,
    BR2400,
    BR3600,
    BR4800,
    BR7200,
    BR14400,
    BR19200,
    BR28800,
    BR38400,
    BR57600,
    BR115200
}OD2101_BaudRate;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC bool i2c_printf_init(/* OD2101_BaudRate emBaudRate*/);
PUBLIC void i2c_vPrintf(const char *fmt, ...);
PUBLIC void i2c_vPrintMem(uint8 *p, uint16 len);
PUBLIC void i2c_vPrintPoll(void);

#endif /* I2C_PRINTF_UTIL_H_ */
