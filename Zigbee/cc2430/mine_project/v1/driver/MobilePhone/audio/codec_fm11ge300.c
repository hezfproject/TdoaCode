/*******************************************************************************
** driver for codec of fm11ge300, provide initial and control interface
*******************************************************************************/

#include <ioCC2430.h>
#include "codec_fm11ge300.h"
#include "codec_i2c.h"
#include "delay.h"

//#define CODEC_USE_UART                    1

#ifdef CODEC_USE_UART
// OD2101 I2C address define
#define I2C_SLAVE_ADDR_OD2101       (0x28)      // no r/w bit (0x50)

// OD2101 register address define
#define OD2101_SUBADDR_WR           (0x00)      // data write address
#define OD2101_SUBADDR_RD           (0x00)      // data read address
#define OD2101_SUBADDR_UARTBUF      (0x01)      // valid number of uart receive buf
#define OD2101_SUBADDR_I2CBUF       (0x02)      // free space of I2C buf 
#define OD2101_SUBADDR_CTRL         (0x03)      // control register address

// OD2101 control id define
#define OD2101_SET_BAUDRATE         (0x00)
#define OD2101_GET_BAUDRATE         (0x01)

#else

#define I2C_SLAVE_ADDR_FM11GE300    (0x60)      // no r/w bit (0xC0)
#endif

// FM11GE300 command define
#define FM11GE300_MEM_WRITE         (0x3B)
#define FM11GE300_MEM_READ          (0x37)
#define FM11GE300_SREG_WRITE        (0x68)
#define FM11GE300_LREG_WRITE        (0x6A)
#define FM11GE300_REG_READ          (0x60)


// FM11GE300 register addr and value define
/*the following parameter can only be changed at initialization stage*/
#define FM11GE300_DVENABLE          (0x1E30)    //General enable
#define DVENABLE_UART_ENABLE        (0x400)
#define DVENABLE_PARSER_ENABLE      (0x200)
#define DVENABLE_SER_ENABLE         (0x100)
#define DVENABLE_LINEOUT_ENABLE     (0x20)
#define DVENABLE_LINEIN_ENABLE      (0x10)
#define DVENABLE_SPKOUT_ENABLE      (0x08)
#define DVENABLE_MIC1_ENABLE        (0x02)
#define DVENABLE_MIC0_ENABLE        (0x01)

#define FM11GE300_MICRVMODE         (0x1E32)    // Revert mode, and mic1 will be selected

#define FM11GE300_MICPGAGAIN        (0x1E34)    // SW gain to preset. It will transfer to HW gain later

#define FM11GE300_LINEINPGAGAIN     (0x1E35)    // as above

#define FM11GE300_LINEOUTPGAGAIN    (0x1E36)    // as above

#define FM11GE300_SPKPGAGAIN        (0x1E37)    // as above

#define FM11GE300_DIGICTRL          (0x1E38)    // Choose clk, fsync, and digi length 
#define DIGICTRL_SERCLK_INT         (0x4000)
#define DIGICTRL_SERSYN_INT         (0x100)
#define DIGICTRL_SERCLK_DLY         (0x10)
#define DIGICTRL_SERWORD_LEN16      (0x0f)
#define DIGICTRL_SERWORD_LEN8       (0x07)

#define FM11GE300_DVFLAG            (0x1E3A)    // set it 0 to start device
#define DVFLAG_READY                (0x8000)

#define FM11GE300_PWDSET            (0x1E51)    // put to powerdonw mode
#define PWDSET_NEEDRELOAD           (0xD000)
#define PWDSET_NONEEDRELOAD         (0xC000)

#define FM11GE300_DSPMIPS           (0x1E52)

#define FM11GE300_GPI               (0x1E6A)

#define FM11GE300_EARSPKGAIN        (0x1E75)

/*the following parameters can be changed at run time only with precaution*/
#define FM11GE300_HW_MICPGAGAIN     (0x3FC0)    // Transfered from SW gain. Do not modify

#define FM11GE300_HW_LINEINPGAGAIN  (0x3FC1)    // as above

#define FM11GE300_HW_SPKPGAGAIN     (0x3FC6)    // as above

#define FM11GE300_UARTBAUDRATE      (0x3FD9)    

/*the following parameters need to be preset according to application, placement and component*/
#define FM11GE300_SPKOUTDRCSLANT    (0x1E00)    // 

#define FM11GE300_SPKOUTDRCLEVEL    (0x1E01)

#define FM11GE300_LOUTDRCSLANT      (0x1E07)

#define FM11GE300_LOUTDRCLEVEL      (0x1E08)

#define FM11GE300_SPKDBDROP         (0x1E0F)

#define FM11GE300_SPKDBDECAY        (0x1E10)

#define FM11GE300_LECREFPOWTH        (0x1E1D)

#define FM11GE300_MICHPFTYPE        (0x1E3B)

#define FM11GE300_MICVOLUME         (0x1E3D)

#define FM11GE300_SPKVOLUME         (0x1E3E)

#define FM11GE300_SPKMUTE           (0x1E3F)

#define FM11GE300_MICMUTE           (0x1E40)

#define FM11GE300_NUMOFMICS         (0x1E41)    //quite important, must preset

#define FM11GE300_MURXEXC           (0x1E42)

#define FM11GE300_MUTXEXC           (0x1E43)

#define FM11GE300_KLCONFIG          (0x1E44)

#define FM11GE300_SPFLAG            (0x1E45)

#define FM11GE300_FTFLAG            (0x1E46)

#define FM11GE300_MICNSSLEVEL       (0x1E47)

#define FM11GE300_MICNSLOWBANDGAIN  (0x1E48)

#define FM11GE300_MICNSHIGHBANDGAIN (0x1E49)

#define FM11GE300_AECREFGAIN        (0x1E4D)

#define FM11GE300_LECREFGAIN        (0x1E4E)

#define FM11GE300_FFTIFFT           (0x1E4F)

#define FM11GE300_SPKVOLUMECAP      (0x1E50)

#define FM11GE300_MICSATTH          (0x1E57)

#define FM11GE300_LOUTCLIPTH        (0x1E59)

#define FM11GE300_YOUTSATTH         (0x1E5A)

#define FM11GE300_STHDTIME          (0x1E5C)

#define FM11GE300_MICTESTFLAG       (0x1E61)

#define FM11GE300_AECDELAYLENGTH    (0x1E63)    // extra time to compensate AD DA delay

#define FM11GE300_LECDELAYLENGTH    (0x1E64)    // extra time to compensate AD DA delay

#define FM11GE300_VOLINCSTEP        (0x1E6E)    // Step for volup and voldn button

#define FM11GE300_PWDDEVICEOFF      (0x1E70)

#define FM11GE300_PFZFACTOREXPHIGH  (0x1E86)

#define FM11GE300_PFZFACTOREXPLOW   (0x1E87)

#define FM11GE300_FETHYOUT          (0x1E88)

#define FM11GE300_PFCOEFGAIN        (0x1E8B)

#define FM11GE300_FEVADTHBIG        (0x1E8C)

#define FM11GE300_AECNWSHIFT        (0x1E90)

#define FM11GE300_AECFEVADSHIFT     (0x1E91)

#define FM11GE300_LECPFZFACTOREXP   (0x1E94)

#define FM11GE300_LECPOSTMU         (0x1E95)

#define FM11GE300_LECMUSHIFT        (0x1E96)

#define FM11GE300_NOISEGAIN         (0x1E9A)

#define FM11GE300_MICGAIN0          (0x1E9B)

#define FM11GE300_MICGAIN1          (0x1E9C)

#define FM11GE300_MICAGCMINAGC      (0x1E9F)

#define FM11GE300_MICAGCMAXAGC      (0x1EA0)

#define FM11GE300_MICAGCREFLOW      (0x1EA3)

#define FM11GE300_MICAGCREFHIGH     (0x1EA4)

#define FM11GE300_LINEINAGCREF      (0x1EA8)

#define FM11GE300_LINEINAGCMINAGC   (0x1EA9)

#define FM11GE300_LINEINAGCMAXAGC   (0x1EAA)

#define FM11GE300_FQPERIOD          (0x1EBC)

#define FM11GE300_FQBETAV           (0x1EBD)

#define FM11GE300_FQBETAUV          (0x1EBE)

#define FM11GE300_FEVADGAINLIMIT    (0x1EC5)

#define FM11GE300_FQFEVADGAINLOW    (0x1EC6)

#define FM11GE300_FQFEVADGAINHIGH   (0x1EC7)

#define FM11GE300_LINEOUTEQUAL0     (0x1EC9)

#define FM11GE300_LINEOUTEQUAL1     (0x1ECA)

#define FM11GE300_LINEOUTEQUAL2     (0x1ECB)

#define FM11GE300_LINEOUTEQUAL3     (0x1ECC)

#define FM11GE300_LINEOUTEQUAL4     (0x1ECD)

#define FM11GE300_VAD0CEILLOW       (0x1ED4)

#define FM11GE300_VAD0CEILHIGH      (0x1ED5)

#define FM11GE300_VAD0RATTHRDFE     (0x1ED6)

#define FM11GE300_VAD0RATTHRDNOFE   (0x1ED7)

#define FM11GE300_INVCONST1         (0x1ED8)

#define FM11GE300_CLIPTH1           (0x1EDA)

#define FM11GE300_CLIPTH2           (0x1EDB)

#define FM11GE300_YOUTDESCON0HIGH   (0x1EDE)

#define FM11GE300_YOUTDESCON0LOW    (0x1EDF)

#define FM11GE300_VAD3RATTHRD       (0x1EED)

#define FM11GE300_VAD3MULT          (0x1EEE)

#define FM11GE300_VAD12DIFFCEIL     (0x1EF3)

#define FM11GE300_VAD12DIFFMAX      (0x1EF4)

#define FM11GE300_VAD1RATTHRD       (0x1EFC)

#define FM11GE300_VAD1ADDTHRD       (0x1EFE)

#define FM11GE300_VAD2RATTHRD       (0x1F06)

#define FM11GE300_VAD2ADDTHRD       (0x1F07)

#define FM11GE300_LECPFMINGAIN      (0x1F08)

/*Parameters associated with PCM extention mode*/
#define FM11GE300_PCMEXTMODE        (0x1E77)

#define FM11GE300_PCMEXTMICSWITCH   (0x1E78)

#define FM11GE300_PCMEXTMICGAIN     (0x1E79)

/*Read only parameters*/
#define FM11GE300_FRAMECOUNTER      (0x1E65)

#define FM11GE300_WATCHDOGCOUNT     (0x1E5E)

#define FM11GE300_MIC_SAT_COUNT     (0x1E5F)

#define FM11GE300_VADLEDFLAGS       (0x1E80)

#define FM11GE300_STRAPOPTIONSTATUS (0x3FCE)

#define FM11GE300_REFMICCALIBRATIONGAIN (0xB82)

#define FM11GE300_LINEINAGCGAIN     (0xB41)

#define FM11GE300_MICINAGCGAIN      (0x1BC3)



/*******************************************************************************
**  Local function define
*******************************************************************************/
#ifdef CODEC_USE_UART
static int8 od2101_write_data(uint8 *pdata, uint8 len);
static int8 od2101_read_data(uint8 *pdata, uint8 len);
static int8 od2101_control(uint8 ctlid, uint8 *pdata, uint8 len);
#endif

static int8 fm11ge300_write_memory(uint16 *paddr, uint16 *pdata, uint8 len);
//static int8 fm11ge300_read_memory(uint16 *paddr, uint16 *pdata, uint8 len);
#ifdef CODEC_USE_UART
static int8 fm11ge300_read_reg(uint8 addr, uint8 *pdata);
static int8 fm11ge300_write_lreg(uint8 addr, uint16 data);
static int8 fm11ge300_write_sreg(uint8 addr, uint8 data);
#endif
/*******************************************************************************
**  Delay uitility
*******************************************************************************/
/*
static void DelayUs(uint32 n)
{
    uint32 i = 0;
    for (i=0; i<n; i++) {
        asm("nop");
        asm("nop");
        asm("nop");
    }
}

static void DelayMs(uint32 n)
{
    uint32 i = 0;
    uint32 j = 0;
    
    for (i=0; i<n; i++) {
      for (j=0; j<3000; j++) {
            asm("nop");
      }
    }
}

*/

/******************************************************************************
** Description: write data into od2101
** Input param: 
        uint8 *pdata    pointer of write-buf
        uint8 len       length of write buf
** Return value:
        0 :  success
        others : fail
*******************************************************************************/
#ifdef CODEC_USE_UART
static int8 od2101_write_data(uint8 *pdata, uint8 len)
{
    uint8 len_1 = 0;
    uint8 brs = 0;
    uint8 cnt = 0;
    uint8 tmp = 0;

    do {
        // 1, read i2cbuf register to get free space of od2101 write-fifo
        tmp = OD2101_SUBADDR_I2CBUF;
        brs = I2C_Gets(I2C_SLAVE_ADDR_OD2101, &tmp, 1, (char *)&len_1, 1);
        if (brs != 0) {
            return -1;  // i2c read error
        }
        if (len_1 > len) {
            tmp = OD2101_SUBADDR_WR;
            brs = I2C_Puts(I2C_SLAVE_ADDR_OD2101, &tmp, 1, (char *)pdata, len);
            if (brs != 0) {
                return -1;  // i2c write error
            }
            len -= len;
        } else {
            tmp = OD2101_SUBADDR_WR;
            brs = I2C_Puts(I2C_SLAVE_ADDR_OD2101, &tmp, 1, (char *)pdata, len_1);
            if (brs != 0) {
                return -1;
            }
            len -= len_1;
            pdata += len_1;
        }
        cnt++;
    } while ((len > 0) && (cnt < 10));
    
    if (cnt == 10) {
        return -1;
    } else {
        return 0;
    }
}

/******************************************************************************
** Description: read data from od2101
** Input param: 
        uint8 *pdata    pointer of read-buf
        uint8 len       length of read buf
** Return value:
        0 :  success
        others : fail
*******************************************************************************/
static int8 od2101_read_data(uint8 *pdata, uint8 len)
{
    uint8 len_1 = 0;
    uint8 brs = 0;
    uint8 cnt = 0;
    uint8 tmp = 0;
    
    do {
        // 1, read uartbuf register to get valid data cnt of od2101 read-fifo
        tmp = OD2101_SUBADDR_UARTBUF;
        brs = I2C_Gets(I2C_SLAVE_ADDR_OD2101, &tmp, 1, (char *)&len_1, 1);
        if (brs != 0) {
            return -1;  // i2c read error
        }
        if (len_1 > len) {
            tmp = OD2101_SUBADDR_RD;
            brs = I2C_Gets(I2C_SLAVE_ADDR_OD2101, &tmp, 1, (char *)pdata, len);
            if (brs != 0) {
                return -1;  // i2c write error
            }
            len -= len;
        } else {
            tmp = OD2101_SUBADDR_RD;
            brs = I2C_Gets(I2C_SLAVE_ADDR_OD2101, &tmp, 1, (char *)pdata, len_1);
            if (brs != 0) {
                return -1;
            }
            len -= len_1;
            pdata += len_1;
        }
        cnt++;
    } while ((len > 0) && (cnt < 10));
    
    if (cnt == 10) {
        return -1;
    } else {
        return 0;
    }
}

/******************************************************************************
** Description: control od2101
** Input param: 
        uint8 ctlid     control id
        uint8 *pdata    pointer of control-buf
        uint8 len       length of control-buf
** Return value:
        0 :  success
        others : fail
*******************************************************************************/
static int8 od2101_control(uint8 ctlid, uint8 *pdata, uint8 len)
{
    uint8 brs = 0;
    uint8 tmp = 0;
    
    switch (ctlid) {
    case OD2101_SET_BAUDRATE:
        if (len > 1) {
            return -1;  // only one payload for this id
        }
        if (pdata[0] > 15) {
            return -1;  // value should be inside [0, 15]
        }
        tmp = OD2101_SUBADDR_CTRL;
        brs = I2C_Puts(I2C_SLAVE_ADDR_OD2101, &tmp, 1, (char *)pdata, len);
        if (brs != 0) {
            return -1;
        }
        break;
    case OD2101_GET_BAUDRATE:
        if (len > 1) {
            return -1;  // only one payload for this id
        }
        tmp = OD2101_SUBADDR_CTRL;
        brs = I2C_Gets(I2C_SLAVE_ADDR_OD2101, &tmp, 1, (char *)pdata, len);
        if (brs != 0) {
            return -1;
        }
        break;
    default:
        return -1;
    }
}
#endif

/******************************************************************************
** Description: write fm11ge300 memory
** Input param: 
        uint16 addr      address of writing-memory
        uint8 *pdata    pointer of write-buf
        uint8 len       length of write-buf
** Return value:
        0 :  success
        others : fail
*******************************************************************************/
#ifdef CODEC_USE_UART
int8 fm11ge300_write_memory(uint16 *paddr, uint16 *pdata, uint8 len)
{
    uint8 i = 0;
    uint8 rs = 0;
    uint8 tmp[2];
    uint16 tmp16 = 0;
    
    // 1, write sync word
    tmp[0] = 0xFC;
    tmp[1] = 0xF3;
    rs = od2101_write_data(&tmp[0], 2);
    if (rs != 0) {
        return -1;
    }
    
    // 2, write command
    tmp[0] = FM11GE300_MEM_WRITE;
    rs = od2101_write_data(&tmp[0], 1);
    if (rs != 0) {
        return -1;
    }
    
    for (i=0; i<len; i++) {
        // 3, write memory address
        tmp16 = *paddr + i;
        tmp[0] = tmp16 & 0xff;
        tmp[1] = (tmp16 >> 8) & 0xff;
        tmp16 = (tmp[0] << 8) | tmp[1];
        rs = od2101_write_data((unsigned char*)(&tmp16), 2);
        if (rs != 0) {
            return -1;
        }
        // 4, write data
        tmp16 = *(pdata + i);
        tmp[0] = tmp16 & 0xff;
        tmp[1] = (tmp16 >> 8) & 0xff;
        tmp16 = (tmp[0] << 8) | tmp[1];        
        rs = od2101_write_data((uint8 *)&tmp16, 2);
        if (rs != 0) {
            return -1;
        }
    }
    return 0;
}
#else
int8 fm11ge300_write_memory(uint16 *paddr, uint16 *pdata, uint8 len)
{
    uint8 i = 0;
    uint8 rs = 0;
    uint8 tmp[7];
    uint16 tmp16 = 0;
    
    tmp[0] = 0xFC;
    tmp[1] = 0xF3;
    tmp[2] = FM11GE300_MEM_WRITE;
    
    for (i=0; i<len; i++) {
        tmp16 = *paddr + i;
        tmp[3] = (tmp16 >> 8) & 0xff;
        tmp[4] = tmp16 & 0xff;

        tmp16 = *(pdata + i);
        tmp[5] = tmp16 & 0xff;
        tmp[6] = (tmp16 >> 8) & 0xff;
        tmp16 = (tmp[5] << 8) | tmp[6];        
        
        rs = I2C_Puts(I2C_SLAVE_ADDR_FM11GE300, tmp, 5, (char *)&tmp16, 2);
        if (rs != 0) {
            return -1;
        }
    }
    return 0;
}
#endif
/******************************************************************************
** Description: read fm11ge300 memory
** Input param: 
        uint16 addr      address of reading-memory
        uint8 *pdata    pointer of read-buf
        uint8 len       length of read-buf
** Return value:
        0 :  success
        others : fail
*******************************************************************************/
#ifdef CODEC_USE_UART
int8 fm11ge300_read_memory(uint16 *paddr, uint16 *pdata, uint8 len)
{
    uint8 i = 0;
    uint8 rs = 0;
    uint8 tmp[2];
    uint16 tmp16 = 0;
    
    // 1, write sync word
    tmp[0] = 0xFC;
    tmp[1] = 0xF3;
    rs = od2101_write_data(&tmp[0], 2);
    if (rs != 0) {
        return -1;
    }
    
    // 2, write command
    tmp[0] = FM11GE300_MEM_READ;
    rs = od2101_write_data(&tmp[0], 1);
    if (rs != 0) {
        return -1;
    }
    
    
    for (i=0; i<len; i++) {
        // 3, write memory address
        tmp16 = (*paddr) + i;
        tmp[0] = tmp16 & 0xff;
        tmp[1] = (tmp16 >> 8) & 0xff;
        tmp16 = (tmp[0] << 8) | tmp[1];        
        rs = od2101_write_data((unsigned char*)(&tmp16), 2);
        if (rs != 0) {
            return -1;
        }
        // 4, read register 26 to get MSB
        rs = fm11ge300_read_reg(0x26, &tmp[0]);
        if (rs != 0) {
            return -1;
        }
        // 5, read register 25 to get LSB
        rs = fm11ge300_read_reg(0x25, &tmp[1]);
        if (rs != 0) {
            return -1;
        }
        pdata[i] = (tmp[0] << 8) | tmp[1];
    }
    return 0;
}
#else
#if 0
int8 fm11ge300_read_memory(uint16 *paddr, uint16 *pdata, uint8 len)
{
    uint8 i = 0;
    uint8 rs = 0;
    uint8 tmp[6];
    uint16 tmp16 = 0;
    
    tmp[0] = 0xFC;
    tmp[1] = 0xF3;
    
    for (i=0; i<len; i++) {
        tmp[2] = FM11GE300_MEM_READ;
        tmp16 = *paddr + i;
        tmp[3] = tmp16 & 0xff;
        tmp[4] = (tmp16 >> 8) & 0xff;    
        tmp16 = (tmp[3] << 8) | tmp[4];
        rs = I2C_Puts(I2C_SLAVE_ADDR_FM11GE300, tmp, 3, (char *)&tmp16, 2);
        if (rs != 0) {
            return -1;
        }
        
        tmp[2] = FM11GE300_REG_READ;
        tmp[3] = 0x25;
        tmp[4] = 0;
        rs = I2C_Gets(I2C_SLAVE_ADDR_FM11GE300, tmp, 4, (char *)&tmp[4], 1);
        if (rs != 0) {
            return -1;
        }
        
        tmp[3] = 0x26;
        tmp[5] = 0;
        rs = I2C_Gets(I2C_SLAVE_ADDR_FM11GE300, tmp, 4, (char *)&tmp[5], 1);
        if (rs != 0) {
            return -1;
        }
        pdata[i] = (tmp[5] << 8) | tmp[4];
    }
    return 0;
}
#endif
#endif
/******************************************************************************
** Description: read fm11ge300 register
** Input param: 
        uint8 addr      register addreess
        uint8 *pdata    pointer of read-buf
** Return value:
        0 :  success
        others : fail
*******************************************************************************/
#ifdef CODEC_USE_UART
int8 fm11ge300_read_reg(uint8 addr, uint8 *pdata)
{
    uint8 rs = 0;
    uint8 tmp[2];
    
    // 1, write sync word
    tmp[0] = 0xFC;
    tmp[1] = 0xF3;
    rs = od2101_write_data(&tmp[0], 2);
    if (rs != 0) {
        return -1;
    }
    
    // 2, write command
    tmp[0] = FM11GE300_REG_READ;
    rs = od2101_write_data(&tmp[0], 1);
    if (rs != 0) {
        return -1;
    }
    
     // 3, write register address
    rs = od2101_write_data(&addr, 1);
    if (rs != 0) {
        return -1;
    }
    
    // 4, read data
    rs = od2101_read_data(pdata, 1);
    if (rs != 0) {
        return -1;
    }

    return 0;
}
#endif
/******************************************************************************
** Description: write fm11ge300 long-register
** Input param: 
        uint8 addr      register addreess
        uint16 data    data to be write
** Return value:
        0 :  success
        others : fail
*******************************************************************************/
#ifdef CODEC_USE_UART
int8 fm11ge300_write_lreg(uint8 addr, uint16 data)
{
    uint8 rs = 0;
    uint8 tmp[2];
    
    // 1, write sync word
    tmp[0] = 0xFC;
    tmp[1] = 0xF3;
    rs = od2101_write_data(&tmp[0], 2);
    if (rs != 0) {
        return -1;
    }
    
    // 2, write command
    tmp[0] = FM11GE300_LREG_WRITE;
    rs = od2101_write_data(&tmp[0], 1);
    if (rs != 0) {
        return -1;
    }
    
     // 3, write register address
    rs = od2101_write_data(&addr, 1);
    if (rs != 0) {
        return -1;
    }
    
    // 4, write data
    rs = od2101_write_data((unsigned char *)&data, 2);
    if (rs != 0) {
        return -1;
    }

    return 0;
}
#endif
/******************************************************************************
** Description: write fm11ge300 short-register
** Input param: 
        uint8 addr      register addreess
        uint8 data    data to be write
** Return value:
        0 :  success
        others : fail
*******************************************************************************/
#ifdef CODEC_USE_UART
int8 fm11ge300_write_sreg(uint8 addr, uint8 data)
{
    uint8 rs = 0;
    uint8 tmp[2];
    
    // 1, write sync word
    tmp[0] = 0xFC;
    tmp[1] = 0xF3;
    rs = od2101_write_data(&tmp[0], 2);
    if (rs != 0) {
        return -1;
    }
    
    // 2, write command
    tmp[0] = FM11GE300_SREG_WRITE;
    rs = od2101_write_data(&tmp[0], 1);
    if (rs != 0) {
        return -1;
    }
    
     // 3, write register address
    rs = od2101_write_data(&addr, 1);
    if (rs != 0) {
        return -1;
    }
    
    // 4, write data
    rs = od2101_write_data(&data, 1);
    if (rs != 0) {
        return -1;
    }

    return 0;
}
#endif
/******************************************************************************
** Description: initial fm11ge300 
** Return value:
        0 :  success
        others : fail
*******************************************************************************/
int8 InitialCodec(void)
{
    uint16 tmp = 0;
    uint16 tmp2 = 0;
    uint8 rs = 0;
    //uint8 cnt = 0;
    
    DelayMs(20);   // wait for FM11GE300 setup
    
    I2C_Init();

#ifdef CODEC_USE_UART    
    cnt = 0x00;
    rs = od2101_control(OD2101_SET_BAUDRATE, &cnt, 1);
    if (rs != 0) {
        return -1;
    }
    
    cnt = 0;
    rs = od2101_control(OD2101_GET_BAUDRATE, &cnt, 1);
    if (rs != 0) {
        return -1;
    }
#endif
/*    
    tmp = 0x7C0;
    tmp2 = FM11GE300_PWDDEVICEOFF;
    rs = fm11ge300_write_memory(&tmp2, &tmp, 1);  // power down
    if (rs != 0) {
        return -1;
    }
*/
    
    tmp = DVENABLE_UART_ENABLE | DVENABLE_SER_ENABLE
            | DVENABLE_SPKOUT_ENABLE | DVENABLE_MIC0_ENABLE;
    tmp2 = FM11GE300_DVENABLE;
    rs = fm11ge300_write_memory(&tmp2, &tmp, 1);
    if (rs != 0) {
        return -1;
    }
    
    /*
    tmp = 0;
    rs = fm11ge300_read_memory(&tmp2, &tmp, 1);
    if (rs != 0) {
        return -1;
    }
    */
    
    //tmp = 0x8B;
    tmp = 0x38;
    tmp2 = FM11GE300_MICPGAGAIN;
    rs = fm11ge300_write_memory(&tmp2, &tmp, 1);
    if (rs != 0) {
        return -1;
    }

    tmp = 0x1D;
    tmp2 = FM11GE300_SPKPGAGAIN;
    rs = fm11ge300_write_memory(&tmp2, &tmp, 1);
    if (rs != 0) {
        return -1;
    }    

    
    tmp = DIGICTRL_SERCLK_INT | DIGICTRL_SERSYN_INT| DIGICTRL_SERWORD_LEN16 |DIGICTRL_SERCLK_DLY;
    tmp2 = FM11GE300_DIGICTRL;
    rs = fm11ge300_write_memory(&tmp2, &tmp, 1);
    if (rs != 0) {
        return -1;
    }       
    
    tmp = 0x02;
    tmp2 = FM11GE300_NUMOFMICS;
    rs = fm11ge300_write_memory(&tmp2, &tmp, 1);  // choose 2 omni mic, may be modified
    if (rs != 0) {
        return -1;
    }
    
    tmp = 0x13;
    tmp2 = FM11GE300_DSPMIPS;
    rs = fm11ge300_write_memory(&tmp2, &tmp, 1);  // select mips
    if (rs != 0) {
        return -1;
    }
    
/*   
    tmp = 0x07DE;
    tmp2 = FM11GE300_SPFLAG;
    rs = fm11ge300_write_memory(&tmp2, &tmp, 1);  // select mips
    if (rs != 0) {
        return -1;
    }
*/    
   
    tmp = 0x01;
    tmp2 = FM11GE300_FFTIFFT;
    rs = fm11ge300_write_memory(&tmp2, &tmp, 1);  // mic0 bypass to lineout
    if (rs != 0) {
        return -1;
    }


    
    tmp = 0x200;
    tmp2 = FM11GE300_MICVOLUME;
    rs = fm11ge300_write_memory(&tmp2, &tmp, 1);  // mic volume
    if (rs != 0) {
        return -1;
    }

    //tmp = 0x100;
    tmp = 0x200;
    tmp2 = FM11GE300_SPKVOLUME;
    rs = fm11ge300_write_memory(&tmp2, &tmp, 1);  // speaker volume
    if (rs != 0) {
        return -1;
    }

    tmp = 0x2000;
    tmp2 = FM11GE300_LECREFPOWTH;
    rs = fm11ge300_write_memory(&tmp2, &tmp, 1);  // better than default value 0x2000
    if (rs != 0) {
        return -1;
    }
    
/*    
    tmp = 0x54;
    tmp2 = FM11GE300_FTFLAG;
    rs = fm11ge300_write_memory(&tmp2, &tmp, 1);  // choose 2 omni mic, may be modified
    if (rs != 0) {
        return -1;
    }
*/   
/*   
    tmp = 0xFFFF;
    tmp2 = FM11GE300_MICRVMODE;
    rs = fm11ge300_write_memory(&tmp2, &tmp, 1); // mic revert mode
    if (rs != 0) {
        return -1;
    }   
*/   
    
    // let configer go
    tmp = 0;    // DVFLAG_READY;
    tmp2 = FM11GE300_DVFLAG;
    rs = fm11ge300_write_memory(&tmp2, &tmp, 1);
    if (rs != 0) {
        return -1;
    }  

    return 0;
}

int8 CloseCodec()
{
    I2C_Close();
    return 0;
}
