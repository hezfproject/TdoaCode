#include <AppHardwareApi.h>
#include "i2c_printf_util.h"
#include <string.h>
#include <printf_util.h>
#include <timer_util.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define OD2101_BUFF_LEN (64)
#define OD2101_ADRESS (0x50)
#define OD2101_WR     (0x00) /*数据寄存器*/
#define OD2101_RD	  (0x00)
#define OD2101_UARTBUF (0x01) /*UART接收缓存接收字节数*/
#define OD2101_I2CBUF (0x02) /*I2C可加载字节数*/
#define OD2101_CTRL   (0x03) /*UART接口控制寄存器*/

#define I2C_BUFF_LEN 512

typedef struct
{
    char buf[I2C_BUFF_LEN];
    uint16 head;
    uint16 tail;
    uint16 u16Size;
} od2101_hdl_t;
PRIVATE bool OD2101_Write_Byte(char date, bool_t bStop, bool_t bAck);
PRIVATE void OD2101_Write_Command(uint8 u8Command);
PRIVATE void OD2101_Write_Regester(uint8 u8Command, uint8 u8regester);
PRIVATE uint8 OD2101_Read_Regester(uint8 u8Command);
PRIVATE void vNumString(uint32 u32Number, uint8 u8Base);
PRIVATE void OD2101_Push(char data);
PRIVATE bool OD2101_BufEmpty(void);
PRIVATE bool OD2101_BufFull(void);
PRIVATE void vSBWaitForComplete(uint8 u8SlaveAddress);

od2101_hdl_t g_hdl;
/****************************************************************************
 *
 * NAME: OD2101_Write_Command
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 ****************************************************************************/
PRIVATE void OD2101_Write_Command(uint8 u8Command)
{
    vAHI_SiMasterWriteSlaveAddr(OD2101_ADRESS >> 1, // ADS1100 IIC address 1001000_R/W
                                FALSE); // 1.TRUE - configure a read 2.FALSE - configure a write
    bAHI_SiMasterSetCmdReg(
                           E_AHI_SI_START_BIT, // Start followed by Write
                           E_AHI_SI_NO_STOP_BIT, E_AHI_SI_NO_SLAVE_READ,
                           E_AHI_SI_SLAVE_WRITE, E_AHI_SI_SEND_NACK,
                           E_AHI_SI_NO_IRQ_ACK);
    while (bAHI_SiMasterPollTransferInProgress())
        ; // wait while busy
    if (bAHI_SiMasterCheckRxNack())
    {
        return; // check to see if we get an ACK back
    }
    if (bAHI_SiMasterPollArbitrationLost())
    {
        return; // check to see if anyone else has taken the bus
    }

    vAHI_SiMasterWriteData8(u8Command); // ADS1100 ConfigReg BSY 0 0 SC DR1 DR0 PGA1 PGA0
    bAHI_SiMasterSetCmdReg(
                           E_AHI_SI_NO_START_BIT, // Write only
                           E_AHI_SI_NO_STOP_BIT, E_AHI_SI_NO_SLAVE_READ,
                           E_AHI_SI_SLAVE_WRITE, E_AHI_SI_SEND_NACK,
                           E_AHI_SI_NO_IRQ_ACK);
    while (bAHI_SiMasterPollTransferInProgress())
        ; // wait while busy
    if (bAHI_SiMasterCheckRxNack())
    {
        return; // check to see if we get an ACK back
    }
    if (bAHI_SiMasterPollArbitrationLost())
    {
        return; // check to see if anyone else has taken the bus
    }
}

/****************************************************************************
 *
 * NAME: OD2101_Write_Regester
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 ****************************************************************************/
PRIVATE void OD2101_Write_Regester(uint8 u8Command, uint8 u8regester)
{
    vAHI_SiMasterWriteSlaveAddr(OD2101_ADRESS >> 1, // OD2101_ADRESS 0x50
                                FALSE); // 1.TRUE - configure a read 2.FALSE - configure a write
    bAHI_SiMasterSetCmdReg(
                           E_AHI_SI_START_BIT, // Start followed by Write
                           E_AHI_SI_NO_STOP_BIT, E_AHI_SI_NO_SLAVE_READ,
                           E_AHI_SI_SLAVE_WRITE, E_AHI_SI_SEND_NACK,
                           E_AHI_SI_NO_IRQ_ACK);
    while (bAHI_SiMasterPollTransferInProgress())
        ; // wait while busy
    if (bAHI_SiMasterCheckRxNack())
    {
        return; // check to see if we get an ACK back
    }
    if (bAHI_SiMasterPollArbitrationLost())
    {
        return; // check to see if anyone else has taken the bus
    }

    vAHI_SiMasterWriteData8(u8Command); // ADS1100 ConfigReg BSY 0 0 SC DR1 DR0 PGA1 PGA0
    bAHI_SiMasterSetCmdReg(
                           E_AHI_SI_NO_START_BIT, // Write only
                           E_AHI_SI_NO_STOP_BIT, E_AHI_SI_NO_SLAVE_READ,
                           E_AHI_SI_SLAVE_WRITE, E_AHI_SI_SEND_NACK,
                           E_AHI_SI_NO_IRQ_ACK);
    while (bAHI_SiMasterPollTransferInProgress())
        ; // wait while busy
    if (bAHI_SiMasterCheckRxNack())
    {
        return; // check to see if we get an ACK back
    }
    if (bAHI_SiMasterPollArbitrationLost())
    {
        return; // check to see if anyone else has taken the bus
    }

    vAHI_SiMasterWriteData8(u8regester); // ADS1100 ConfigReg BSY 0 0 SC DR1 DR0 PGA1 PGA0
    bAHI_SiMasterSetCmdReg(
                           E_AHI_SI_NO_START_BIT, // Write flowed by stop
                           E_AHI_SI_STOP_BIT, E_AHI_SI_NO_SLAVE_READ,
                           E_AHI_SI_SLAVE_WRITE, E_AHI_SI_SEND_NACK,
                           E_AHI_SI_NO_IRQ_ACK);
    while (bAHI_SiMasterPollTransferInProgress())
        ; // wait while busy
    if (bAHI_SiMasterCheckRxNack())
    {
        return; // check to see if we get an ACK back
    }
    if (bAHI_SiMasterPollArbitrationLost())
    {
        return; // check to see if anyone else has taken the bus
    }
}

/****************************************************************************
 *
 * NAME: OD2101_Read_Regester
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 ****************************************************************************/
PRIVATE uint8 OD2101_Read_Regester(uint8 u8Command)
{
    OD2101_Write_Command(u8Command);

    vAHI_SiMasterWriteSlaveAddr(OD2101_ADRESS >> 1, // OD2101_ADRESS (0x50)
                                TRUE); // 1.TRUE - configure a read 2.FALSE - configure a write
    bAHI_SiMasterSetCmdReg(
                           E_AHI_SI_START_BIT, // Start followed by Write
                           E_AHI_SI_NO_STOP_BIT, E_AHI_SI_NO_SLAVE_READ,
                           E_AHI_SI_SLAVE_WRITE, E_AHI_SI_SEND_NACK,
                           E_AHI_SI_NO_IRQ_ACK);
    while (bAHI_SiMasterPollTransferInProgress())
        ; // wait while busy
    if (bAHI_SiMasterCheckRxNack())
    {
        return 0; // check to see if we get an ACK back
    }
    if (bAHI_SiMasterPollArbitrationLost())
    {
        return 0; // check to see if anyone else has taken the bus
    }

    bAHI_SiMasterSetCmdReg(
                           E_AHI_SI_NO_START_BIT, //Read followed by Stop
                           E_AHI_SI_STOP_BIT, E_AHI_SI_SLAVE_READ,
                           E_AHI_SI_NO_SLAVE_WRITE, E_AHI_SI_SEND_NACK, //Send NACK
                           E_AHI_SI_NO_IRQ_ACK);
    while (bAHI_SiMasterPollTransferInProgress())
        ; // wait while busy
    return u8AHI_SiMasterReadData8();
}

/****************************************************************************
 *
 * NAME: OD2101_Write_Byte
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 ****************************************************************************/
PRIVATE bool OD2101_Write_Byte(char date, bool_t bStop, bool_t bAck)
{
    vAHI_SiMasterWriteData8(date);
    bAHI_SiMasterSetCmdReg(
                           E_AHI_SI_NO_START_BIT, //Write followed with stop
                           bStop, E_AHI_SI_NO_SLAVE_READ, E_AHI_SI_SLAVE_WRITE,
                           bAck, E_AHI_SI_NO_IRQ_ACK);
    while (bAHI_SiMasterPollTransferInProgress())
        ; // wait while busy
    if (bAHI_SiMasterCheckRxNack() || bAHI_SiMasterPollArbitrationLost())
    {
        // check to see if we get an ACK back// check to see if anyone else has taken the bus
        return FALSE;
    }

    return TRUE;
}

/****************************************************************************
 *
 * NAME: vNum2String
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 ****************************************************************************/
PRIVATE void vNumString(uint32 u32Number, uint8 u8Base)
{
    char buf[33];
    uint8 u8Idx = 0;
    uint32 u32TempNum;

    if (u8Base < 2 || u8Base > 16)
        return;

    do
    {
        u32TempNum = u32Number;
        u32Number /= u8Base;
        /* not using the modulus operator (%) but rather calculating
         it by hand saving a division. This does speed things up a little
         */
        buf[u8Idx++] = "0123456789ABCDEF"[u32TempNum - u32Number * u8Base];
    } while (u32Number);

    while (u8Idx--)
    {
        OD2101_Push(buf[u8Idx]);
    }
}

PRIVATE void OD2101_Push(char data)
{
    if (!OD2101_BufFull())
    {
        g_hdl.u16Size++;
        g_hdl.buf[g_hdl.head++] = data;

        if (I2C_BUFF_LEN == g_hdl.head)
        {
            g_hdl.head = 0;
        }
    }
    else
    {
        uint16 u16PreIdx = g_hdl.head ? g_hdl.head - 1 : I2C_BUFF_LEN - 1;
        g_hdl.buf[u16PreIdx--] = '\r';
        g_hdl.buf[u16PreIdx] = '\n';
    }
}

PRIVATE bool OD2101_BufEmpty(void)
{
    return !(g_hdl.u16Size);
}

PRIVATE bool OD2101_BufFull(void)
{
    return (I2C_BUFF_LEN == g_hdl.u16Size);
}

/****************************************************************************
 *
 * NAME: i2c_vPrintf
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 ****************************************************************************/
PUBLIC void i2c_vPrintf(const char *fmt, ...)
{
    char *bp = (char *) fmt;
    va_list ap;
    char c;
    char *p;
    int32 i;

    va_start(ap, fmt);

    while ((c = *bp++))
    {
        if (c != '%')
        {
            OD2101_Push(c);
            if ('\n' == c)
            {
                OD2101_Push('\r');
            }
            continue;
        }

        switch ((c = *bp++)) {
            /* %d - show a decimal value */
            case 'd':
                vNumString(va_arg(ap, uint32), 10);
                break;

                /* %x - show a value in hex */
            case 'x':
                OD2101_Push('0');
                OD2101_Push('x');
                vNumString(va_arg(ap, uint32), 16);
                break;

            case 'X':
                vNumString(va_arg(ap, uint32), 16);
                break;

                /* %b - show a value in binary */
            case 'b':
                OD2101_Push('0');
                OD2101_Push('b');
                vNumString(va_arg(ap, uint32), 2);
                break;

                /* %c - show a character */
            case 'c':
                OD2101_Push( va_arg( ap, int));
                break;

            case 'i':
                i = va_arg(ap, int32);
                if (i < 0)
                {
                    OD2101_Push('-');
                    vNumString((~i) + 1, 10);
                }
                else
                {
                    vNumString(i, 10);
                }
                break;

                /* %s - show a string */
            case 's':
                p = va_arg(ap, char *);
                do
                {
                    OD2101_Push(*p++);
                } while (*p);
                break;

                /* %% - show a % character */
            case '%':
                OD2101_Push('%');
                break;

                /* %something else not handled ! */
            default:
                OD2101_Push('?');
                break;
        }
    }
    return;
}

/****************************************************************************
 *
 * NAME: OD2101_Read
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 ****************************************************************************/
PUBLIC bool i2c_printf_init(/*OD2101_BaudRate emBaudRate*/)
{
    OD2101_BaudRate emBaudRate = BR115200;
    if (emBaudRate < BR9600 || emBaudRate > BR115200)
        return FALSE;

    vAHI_SiMasterConfigure(FALSE, //1.Enable pulse suppression filter
                           FALSE, //2.Disable Serial Interface interruptEnable
                           0x07); //3.200Kps BraudRate Operating frequency = 16/[(PreScaler + 1) x 5] MHz
    OD2101_Write_Regester(OD2101_CTRL, emBaudRate); //Configured UART as 115200 baud rate
    memset((void*) &g_hdl, 0, sizeof(g_hdl));

    return TRUE;
}

PUBLIC void i2c_vPrintMem(uint8 *p, uint16 len)
{
    uint32 i;

    for (i = 0; i < len; i++)
    {
        i2c_vPrintf("%X ", *(p + i));
    }

    i2c_vPrintf("\n");
}

PUBLIC void i2c_vPrintPoll(void)
{
    static uint32 u32PreTimer;
    uint32 u32CurrentTimer;

    if (!g_hdl.u16Size)
        return;

    u32CurrentTimer = TimerUtil_GetSystemTimer();
    if (u32CurrentTimer - u32PreTimer < 2500)
        return;

    u32PreTimer = u32CurrentTimer;

    uint8 u8Count = 8;
    bool_t bStop = E_AHI_SI_NO_STOP_BIT;
    bool_t bAck = E_AHI_SI_SEND_ACK;

    OD2101_Write_Command(0x00);

    while (u8Count && g_hdl.u16Size)
    {
        --u8Count;
        --g_hdl.u16Size;

        if (!(u8Count && g_hdl.u16Size))
        {
            bStop = E_AHI_SI_STOP_BIT;
            bAck = E_AHI_SI_SEND_ACK;
        }

        OD2101_Write_Byte(g_hdl.buf[g_hdl.tail], bStop, bAck);

        g_hdl.tail = (I2C_BUFF_LEN == g_hdl.tail) ? 0 : g_hdl.tail + 1;

    }
    //vSBWaitForComplete(OD2101_ADRESS);
}

PRIVATE void vSBWaitForComplete(uint8 u8SlaveAddress)
{
    u8SlaveAddress = u8SlaveAddress >> 1;
    do
    {
        vAHI_SiWriteSlaveAddr(u8SlaveAddress, FALSE);
        vAHI_SiSetCmdReg(E_AHI_SI_START_BIT, E_AHI_SI_NO_STOP_BIT,
                         E_AHI_SI_NO_SLAVE_READ, E_AHI_SI_SLAVE_WRITE,
                         E_AHI_SI_SEND_NACK, E_AHI_SI_NO_IRQ_ACK);

        while (bAHI_SiPollTransferInProgress())
            ; /* wait while busy */
    } while (bAHI_SiMasterCheckRxNack());

    /* now free up the bus */
    vAHI_SiWriteSlaveAddr(u8SlaveAddress, FALSE);
    vAHI_SiSetCmdReg(E_AHI_SI_NO_START_BIT, E_AHI_SI_STOP_BIT,
                     E_AHI_SI_NO_SLAVE_READ, E_AHI_SI_SLAVE_WRITE,
                     E_AHI_SI_SEND_ACK, E_AHI_SI_NO_IRQ_ACK);

    while (bAHI_SiPollTransferInProgress())
        ; /* wait while busy */
}

