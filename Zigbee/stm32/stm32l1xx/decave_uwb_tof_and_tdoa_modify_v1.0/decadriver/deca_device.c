/*! ------------------------------------------------------------------------------------------------------------------
 * FILE: deca_device.c - DecaWave device control functions
 *
 * Copyright 2008, 2013 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * Author(s): Billy Verso / Zoran Skrba
 */

#include "deca_types.h"
#include "deca_param_types.h"
#include "deca_device_api.h"
#include "deca_regs.h"
#include "port.h"


// Defines for enable_clocks function
#define FORCE_SYS_XTI  0
#define ENABLE_ALL_SEQ 1
#define FORCE_SYS_PLL  2
#define READ_LDE_ON    5
#define READ_LDE_OFF   6
#define READ_ACC_ON    7
#define READ_ACC_OFF   8
#define FORCE_LDE_ON   9
#define FORCE_LDE_OFF  10
#define FORCE_NVM_ON   11
#define FORCE_NVM_OFF  12
#define FORCE_TX_PLL   13

// #define DWT_API_ERROR_CHECK     // define so API checks config input parameters

// -------------------------------------------------------------------------------------------------------------------
//
// Internal functions for controlling and configuring the device
//
// -------------------------------------------------------------------------------------------------------------------

// Check if Clock PLL is locked
int _dwt_checkclkplllock(void);
// Enable and Configure specified clocks
void _dwt_enableclocks(int clocks) ;
// Configure the ucode (FP algorithm) parameters
void _dwt_configlde(int prf);
// load ucode from NVMEM/ROM
int _dwt_loaducodefromrom(void);
//read non-volatile memory
uint32 _dwt_nvmread(uint32 address);
//program the non-volatile memory
uint32 _dwt_nvmprogword32(uint32 data, uint16 address);
//upload the device configuration into always on memory
void _dwt_aonarrayupload(void);
void dwt_rxreset(void);
// -------------------------------------------------------------------------------------------------------------------

/*!
 * Static data for DW1000 DecaWave Transceiver control
 */

// -------------------------------------------------------------------------------------------------------------------
// structure to hold device data
typedef struct
{
    uint32      deviceID ;
    uint32      partID ;
    uint32      lotID ;
    uint8       chan;               // added chan here - used in the reading of acc
    uint8       longFrames ;        // flag in non-standard long frame mode
    uint32      txFCTRL ;           // keep TX_FCTRL register config
    uint16      rfrxDly;            // rf delay (delay though the RF blocks before the signal comes out of the antenna i.e. "antenna delay")
    uint16      rftxDly;            // rf delay (delay though the RF blocks before the signal comes out of the antenna i.e. "antenna delay")
    uint32      antennaDly;         // antenna delay read from NVM 64 PRF value is in high 16 bits and 16M PRF in low 16 bits
    uint8       xtrim;              // xtrim value read from NVM
    uint32      sysCFGreg ;         // local copy of system config register
    uint32      txPowCfg[12];       // stores the Tx power configuration read from NVM (6 channels consecutively with PRF16 then 64, e.g. Ch 1 PRF16 is index 0 and 64 index 1)
    uint8       dblbuffon;          // double rx buffer mode flag
                                    // the dwt_isr() will only process the events that "enabled" (i.e. the ones that generate interrupt)
    dwt_callback_data_t cdata;      // callback data structure

    uint32      states[3] ;         //MP workaround debug states register
    uint8       statescount ;
    int         prfIndex ;

    void (*dwt_txcallback)(const dwt_callback_data_t *txd);
    void (*dwt_rxcallback)(const dwt_callback_data_t *rxd);

} dwt_local_data_t ;

static dwt_local_data_t dw1000local ; // Static local device data

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_initialise()
 *
 * Description: This function initiates communications with the DW1000 transceiver
 * and reads its DEV_ID register (address 0x00) to verify the IC is one supported
 * by this software (e.g. DW1000 32-bit device ID value is 0xDECA0130).  Then it
 * does any initial once only device configurations needed for use and initialises
 * as necessary any static data items belonging to this low-level driver.
 * Note: it also reads any calibration data from OTP memory as specified by the input paramter
 *
 * input parameters
 * @param config    -   specifies what configuration to load form OTP memory
 *                  DWT_LOADUCODE     0x800 - load the LDE micro-code from ROM - enabled accurate RX timestamp
 *                  DWT_LOADTXCONFIG  0x4 - load the TX power values
 *                  DWT_LOADANTDLY    0x2 - load the antenna delay values
 *                  DWT_LOADXTALTRIM  0x1 - load XTAL trim values
 *                  DWT_LOADNONE      0x0 - do not load any values from OTP memory
 *
 * output parameters
 *
 * returns DWT_DECA_SUCCESS for success, or DWT_DECA_ERROR for error
 */
//OTP addresses definitions
#define PARTID_ADDRESS (0x06)
#define LOTID_ADDRESS  (0x07)
#define VBAT_ADDRESS   (0x08)
#define VTEMP_ADDRESS  (0x09)
#define TXCFG_ADDRESS  (0x10)
#define ANTDLY_ADDRESS (0x1C)
#define XTRIM_ADDRESS  (0x1E)
uint32 reg;


int dwt_initialise(uint16 config)
{
    //uint32 reg;

    int i = 0;

    dw1000local.statescount = 0;
    dw1000local.dblbuffon = 0; //double mode off by default
    dw1000local.prfIndex = 0; //16M
    dw1000local.cdata.aatset = 0;

    dw1000local.dwt_txcallback = NULL ;
    dw1000local.dwt_rxcallback = NULL ;

    dw1000local.deviceID =  dwt_readdevid() ;

    // read and validate device ID return -1 if not recognized
    if (DWT_DEVICE_ID != dw1000local.deviceID) // MP IC ONLY (i.e. DW1000) FOR THIS CODE
    {
        return DWT_ERROR ;
    }

    _dwt_enableclocks(FORCE_SYS_XTI); //NOTE: set system clock to XTI - this is necessary to make sure the values read by _dwt_nvmread are reliable

    dw1000local.partID = _dwt_nvmread(PARTID_ADDRESS);

    dw1000local.lotID = _dwt_nvmread(LOTID_ADDRESS);

    if(config & DWT_LOADANTDLY)
        dw1000local.antennaDly = _dwt_nvmread(ANTDLY_ADDRESS);
    else
        dw1000local.antennaDly = 0;

    if(config & DWT_LOADXTALTRIM)
        dw1000local.xtrim = _dwt_nvmread(XTRIM_ADDRESS) & 0x1F;
    else
        dw1000local.xtrim = 0;

    if(config & DWT_LOADTXCONFIG)
    {
        for(i=0; i<12; i++)
        {
            dw1000local.txPowCfg[i] = _dwt_nvmread(TXCFG_ADDRESS+i);
        }
    }
    else
    {
        for(i=0; i<12; i++)
        {
            dw1000local.txPowCfg[i] = 0;
        }
    }

    // load leading edge detect code
    if(config & DWT_LOADUCODE)
        _dwt_loaducodefromrom();

    reg = dwt_read32bitoffsetreg(PMSC_ID, 0x0);
    reg &= 0xFFB0FFFF; //clear the LDE write clock enable
    dwt_write32bitoffsetreg( PMSC_ID, 0x0, reg);

    _dwt_enableclocks(FORCE_SYS_PLL); //force system clock to use PLL

    _dwt_enableclocks(ENABLE_ALL_SEQ); //enable clocks for sequencing

    //read system register / store local copy
    dw1000local.sysCFGreg = dwt_read32bitreg(SYS_CFG_ID) ;            // read sysconfig register

    /*reg = dwt_read32bitoffsetreg(GPIO_CTRL_ID,8);

    reg |=0x1000;
    dwt_write32bitoffsetreg(GPIO_CTRL_ID,8,reg);
    reg &=(~0x100);
    dwt_write32bitoffsetreg(GPIO_CTRL_ID,8,reg);

    reg = dwt_read32bitoffsetreg(GPIO_CTRL_ID,8);*/

#if 1

    //while(1)
    //while(1)
    {
        //uint32 reg;
        // Set up MFIO


        //uint32 reg;
        // Set up MFIO
        reg = dwt_read32bitreg(GPIO_CTRL_ID);
        reg |= 0x00055000 ; //7 and 8 to mode
        //reg |= 0x00014000;
        dwt_write32bitreg(GPIO_CTRL_ID,reg);

        dwt_write16bitoffsetreg(PMSC_ID, 0x26,0);


        //reg = dwt_read32bitreg(PMSC_ID);

        //dwt_write16bitoffsetreg(PMSC_ID,0+2,0);

        //dw1000local.dwt_txcallback = NULL ;
    //dw1000local.dwt_rxcallback = NULL ;

        //Delay_ms(300);
    }
#endif
    return DWT_SUCCESS ;

} // end dwt_initialise()


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_getpartid()
 *
 *  Description: This is used to return the read part ID of the device
 *
 * input parameters
 *
 * output parameters
 *
 * returns the 32 bit part ID value as programmed in the factory
 */
uint32 dwt_getpartid(void)
{
    return dw1000local.partID;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_getlotid()
 *
 *  Description: This is used to return the read lot ID of the device
 *
 * input parameters
 *
 * output parameters
 *
 * returns the 32 bit lot ID value as programmed in the factory
 */
uint32 dwt_getlotid(void)
{
    return dw1000local.lotID;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_readdevid()
 *
 *  Description: This is used to return the read device tyoe and revision information of the DW1000 device (MP part is 0xDECA0130)
 *
 * input parameters:
 *
 * output parameters:
 *
 * returns the read value which for DW1000 is 0xDECA0130
 */
uint32 dwt_readdevid(void)
{
    return dwt_read32bitoffsetreg(DEV_ID_ID,0);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_configuretxrf()
 *
 * Description: This function provides the API for the configuration of the TX spectrum
 * including the power and pulse generator delay. The input is a pointer to the data structure
 * of type dwt_txconfig_t that holds all the configurable items.
 *
 * input parameters
 * @param config    -   pointer to the txrf configuration structure, which contains the tx rf config data
 *
 * output parameters
 *
 * no return value
 *
 */
void dwt_configuretxrf(dwt_txconfig_t *config)
{

    //Config RF TX PG_DELAY
    dwt_writetodevice(TX_CAL_ID, 0x0b, 1, &config->PGdly);

    //Configure TX power
    dwt_write32bitreg(TX_POWER_ID, config->power);

}


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_getotptxpower()
 *
 * Description: This API function returns the tx power value read from OTP memory as part of initialisation
 *
 * input parameters
 * @param prf   -   this is the PRF e.g. DWT_PRF_16M or DWT_PRF_64M
 * @param chan  -   this is the channel e.g. 1 to 7
 *
 * output parameters
 *
 * returns tx power value for a given PRF and channel
 */
uint32 dwt_getotptxpower(uint8 prf, uint8 chan)
{
    return dw1000local.txPowCfg[(prf - DWT_PRF_16M) + (chan_idx[chan] * 2)];
}


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_configure()
 *
 * Description: This function provides the main API for the configuration of the
 * DW1000 and this low-level driver.  The input is a pointer to the data structure
 * of type dwt_config_t that holds all the configurable items.
 * The dwt_config_t structure shows which ones are supported
 *
 * input parameters
 * @param config    -   pointer to the configuration structure, which contains the device configuration data.
 * @param useotp    -   specifies what configuration (read form OTP memory as part of initialisation) to configure
 *                  DWT_LOADANTDLY    0x2 - load the antenna delay values
 *                  DWT_LOADXTALTRIM  0x1 - load XTAL trim values
 *                  DWT_LOADNONE      0x0 - do not load any values from OTP memory
 *
 * output parameters
 *
 * returns DWT_DECA_SUCCESS for success, or DWT_DECA_ERROR for error
 */
int dwt_configure(dwt_config_t *config, uint8 use_nvmconfigvalues)
{
    uint8 nsSfd_result  = 0;
    uint8 useDWnsSFD = 0;
    uint8 chan = config->chan ;
    uint32 regval ;
    uint16 reg16 = lde_replicaCoeff[config->rxCode];
    uint8 prfIndex = dw1000local.prfIndex = config->prf - DWT_PRF_16M;
    uint8 bw = ((chan == 4) || (chan == 7)) ? 1 : 0 ; //select wide or narrow band

    dw1000local.chan = config->chan ;

#ifdef DWT_API_ERROR_CHECK
    if (config->dataRate > DWT_BR_6M8) return DWT_ERROR ; // validate datarate parameter
    if ((config->prf > DWT_PRF_64M) || (config->prf < DWT_PRF_16M)) return DWT_ERROR ;      // validate Pulse Repetition Frequency
    if (config->rxPAC > DWT_PAC64) return DWT_ERROR ;      // validate PAC size
    if ((chan < 1) || (chan > 7) || (6 == chan)) return DWT_ERROR ; // validate channel number parameter

    // validate TX and TX pre-amble codes selections
    if (config->prf == DWT_PRF_64M)
    {
        // at 64M PRF, codes should be 9 to 27 only
        // config->txCode
        // config->rxCode
    }
    else
    {
        // at 16M PRF, codes should be 0 to 8 only
    }
    switch (config->txPreambLength)
    {
    case DWT_PLEN_4096 :
    case DWT_PLEN_2048 :
    case DWT_PLEN_1536 :
    case DWT_PLEN_1024 :
    case DWT_PLEN_512  :
    case DWT_PLEN_256  :
    case DWT_PLEN_128  :
    case DWT_PLEN_64   : break ; // all okay
    default            : return DWT_ERROR ; // not a good preamble length parameter
    }

    if(config->phrMode > DWT_PHRMODE_EXT)
        return DWT_ERROR ;
#endif

    // for 110 kbps we need to special setup
    if(DWT_BR_110K == config->dataRate)
    {
        dw1000local.sysCFGreg |= SYS_CFG_F110K ;

        reg16 >>= 3;  //div by 8
    }
    else
    {
        dw1000local.sysCFGreg &= (~SYS_CFG_F110K) ;
    }

    dw1000local.longFrames = config->phrMode ;

    dw1000local.sysCFGreg |= (config->phrMode << 16) ;

    dwt_write32bitreg(SYS_CFG_ID,dw1000local.sysCFGreg) ;
    //write/set the lde_replicaCoeff
    dwt_write16bitoffsetreg(LDE_CFG_STS_ID, 0x2804, reg16 ) ;

    _dwt_configlde(prfIndex);

    //config RF pll (for a given channel)
    //configure PLL2/RF PLL block CFG
    dwt_writetodevice(SYNTH_CAL_ID, 0x07, 5, &pll2_config[chan_idx[chan]][0]);
    //configure PLL2/RF PLL block CAL
    dwt_writetodevice(SYNTH_CAL_ID, 0x0e, 1, &pll2calcfg);
    //PLL wont be enabled until a TX/RX enable is issued later on

    // Configure RF RX blocks (for specified channel/badwidth)
    dwt_writetodevice(RF_CFG_STS_ID, RF_CTRL2_SUB_ADDR, 1, &rx_config[bw]);

    // Configure RF TX blocks (for specified channel and prf)
    //Config RF TX control
    dwt_write32bitoffsetreg(RF_CFG_STS_ID, RF_CTRLTX_SUB_ADDR, tx_config[chan_idx[chan]]);

    // Configure the baseband parameters (for specified prf, bit rate, pac, and SFD settings)
    //DTUNE0
    dwt_write16bitoffsetreg(DRX_CFG_STS_ID, RX_DTUNE0_SUB_ADDR2, sftsh[config->dataRate][config->nsSFD]);

    //DTUNE1
    dwt_write16bitoffsetreg(DRX_CFG_STS_ID, RX_DTUNE1_SUB_ADDR, dtune1[prfIndex]);

    if(config->dataRate == DWT_BR_110K)
    {
        dwt_write16bitoffsetreg(DRX_CFG_STS_ID, RX_DTUNE1_SUB_ADDR6, 0x64);
    }
    else
    {
        if(config->txPreambLength == DWT_PLEN_64) //if preamble length is 64
        {
            uint8 temp = 0x10;
            dwt_write16bitoffsetreg(DRX_CFG_STS_ID, RX_DTUNE1_SUB_ADDR6, 0x10);
            dwt_writetodevice(DRX_CFG_STS_ID, 0x26, 1, &temp);
        }
        else
        {
            uint8 temp = 0x28;
            dwt_write16bitoffsetreg(DRX_CFG_STS_ID, RX_DTUNE1_SUB_ADDR6, 0x20);
            dwt_writetodevice(DRX_CFG_STS_ID, 0x26, 1, &temp);
        }
    }

    //DTUNE2
    dwt_write32bitoffsetreg(DRX_CFG_STS_ID,RX_DTUNE2_SUB_ADDR,digital_bb_config[prfIndex][config->rxPAC]);

    //DTUNE3
    if(config->sfdTO != DWT_SFDTOC_DEF) //if default value no need to program it
        dwt_write16bitoffsetreg(DRX_CFG_STS_ID,RX_DTUNE3_SUB_ADDR,config->sfdTO);
    //don't allow 0 - SFD timeout will always be enabled
    if(config->sfdTO == 0)
        dwt_write16bitoffsetreg(DRX_CFG_STS_ID,RX_DTUNE3_SUB_ADDR, DWT_SFDTOC_DEF);

    // configure AGC parameters
    dwt_write32bitoffsetreg( AGC_CFG_STS_ID, 0xC, agc_config.lo32);
    dwt_write16bitoffsetreg( AGC_CFG_STS_ID, 0x4, agc_config.target[prfIndex]);

    // set (non-standard) user SFD for improved performance,
    if(config->nsSFD)
    {
         //Write non standard (DW) SFD length
         dwt_writetodevice(USR_SFD_ID,0x00,1,&dwnsSFDlen[config->dataRate]);
         nsSfd_result = 3 ;
         useDWnsSFD = 1 ;
    }
    regval =  (chan << CHAN_CTRL_TXCHAN_SHIFT) |            // Transmit Channel
              (chan << CHAN_CTRL_RXCHAN_SHIFT) |            // Receive Channel
              (config->prf << CHAN_CTRL_RXFPRF_SHIFT) |     // RX PRF
              (nsSfd_result << CHAN_CTRL_TNS_SHIFT) |       // nsSFD enable RX&TX
              (useDWnsSFD << CHAN_CTRL_DW_SFD_SHIFT) |      // use DW nsSFD
              (config->txCode << CHAN_CTRL_TXPCOD_SHIFT) |  // TX Preamble Code
              (config->rxCode << CHAN_CTRL_RXPCOD_SHIFT) ;  // RX Preamble Code

    dwt_write32bitreg(CHAN_CTRL_ID,regval) ;

    // Set up TX Preamble Size and TX PRF
    // Set up TX Ranging Bit and Data Rate
    {
        uint32 x = (config->txPreambLength | config->prf)  <<  16;
        dw1000local.txFCTRL = x | TX_FCTRL_RANG_BIT |     // always set ranging bit !!!
                                (config->dataRate << TX_FCTRL_RATE_SHFT) ;

        dwt_write32bitoffsetreg(TX_FCTRL_ID,0,dw1000local.txFCTRL) ;
    }

    if (use_nvmconfigvalues & DWT_LOADXTALTRIM)
    {
        dwt_xtaltrim(dw1000local.xtrim);
    }

    if (use_nvmconfigvalues & DWT_LOADANTDLY)
    {
        //put half of the antenna delay value into tx and half into rx
        dwt_setrxantennadelay(((dw1000local.antennaDly >> (16*prfIndex)) & 0xFFFF) >> 1);
        dwt_settxantennadelay(((dw1000local.antennaDly >> (16*prfIndex)) & 0xFFFF) >> 1);
    }

    return DWT_SUCCESS ;

} // end dwt_configure()

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_setrxantennadelay()
 *
 * Description: This API function writes the antenna delay (in time units) to RX registers
 *
 * input parameters:
 * @param antennaDly - this is the total (RX) antenna delay value, which
 *                          will be programmed into the RX register
 *
 * output parameters
 *
 * no return value
 */
void dwt_setrxantennadelay(uint16 antennaDly)
{
    // -------------------------------------------------------------------------------------------------------------------
    // set the antenna delay
    dw1000local.rfrxDly = antennaDly;
    dwt_write16bitoffsetreg(LDE_CFG_STS_ID,0x1804,dw1000local.rfrxDly) ;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_settxantennadelay()
 *
 * Description: This API function writes the antenna delay (in time units) to TX registers
 *
 * input parameters:
 * @param antennaDly - this is the total (TX) antenna delay value, which
 *                          will be programmed into the TX delay register
 *
 * output parameters
 *
 * no return value
 *
 */
void dwt_settxantennadelay(uint16 antennaDly)
{
    // -------------------------------------------------------------------------------------------------------------------
    // set the tx antenna delay for auto tx timestamp adjustment
    dw1000local.rftxDly  = antennaDly;
    dwt_write16bitoffsetreg(TX_ANTD, 0x0, dw1000local.rftxDly) ;
}


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_readantennadelay()
 *
 * Description: This API function returns the antenna delay read from the OTP memory as part of device initialisation
 * Note: the antenna delay will only be read if dwt_initialise is called with DWT_LOADANTDLY bit set in the config parameter
 *
 * input parameters:
 * @param prf   -   this is the PRF e.g. DWT_PRF_16M or DWT_PRF_64M
 *
 */
uint16 dwt_readantennadelay(uint8 prf)
{
    // 32-bit antenna delay value previously read from OTP, high 16 bits is value for 64 MHz PRF, low 16-bits for 16 MHz PRF
    return (dw1000local.antennaDly >> (16*(prf-DWT_PRF_16M))) & 0xFFFF;
}


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_writetxdata()
 *
 * Description: This API function writes the supplied TX data into the DW1000's
 * TX buffer.  The input parameters are the data length in bytes and a pointer
 * to those data bytes.
 *
 * input parameters
 * @param txFrameLength  - This is the total frame length, including the two byte CRC.
 *                         Note: this is the length of TX message (including the 2 byte CRC) - max is 1023
 *                         standard PHR mode allows up to 127 bytes
 *                         if > 127 is programmed, DWT_PHRMODE_EXT needs to be set in the phrMode configuration
 *                         see dwt_configure function
 * @param txFrameBytes   - Pointer to the user’s buffer containing the data to send.
 * @param txBufferOffset - This specifies an offset in the DW1000’s TX Buffer at which to start writing data.
 *
 * output parameters
 *
 * returns DWT_DECA_SUCCESS for success, or DWT_DECA_ERROR for error
 */
int dwt_writetxdata(uint16 txFrameLength, uint8 *txFrameBytes, uint16 txBufferOffset)
{
#ifdef DWT_API_ERROR_CHECK
    if (dw1000local.longFrames)
    {
        if (txFrameLength > 1023) return DWT_ERROR ;
    }
    else
    {
        if (txFrameLength > 127) return DWT_ERROR ;
    }
    if (txFrameLength < 2) return DWT_ERROR ;
#endif

    if ((txBufferOffset + txFrameLength) > 1024)
        return DWT_ERROR ;

    // write the data to the IC TX buffer, (-2 bytes for auto generated CRC)
    dwt_writetodevice( TX_BUFFER_ID, txBufferOffset, txFrameLength-2, txFrameBytes) ;


    return DWT_SUCCESS ;
} // end dwt_writetxdata()

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_writetxfctrl()
 *
 * Description: This API function configures the TX frame control register before the transmission of a frame
 *
 * input parameters:
 * @param txFrameLength - this is the length of TX message (including the 2 byte CRC) - max is 1023
 *                              NOTE: standard PHR mode allows up to 127 bytes
 *                              if > 127 is programmed, DWT_PHRMODE_EXT needs to be set in the phrMode configuration
 *                              see dwt_configure function
 * @param txBufferOffset - the offset in the tx buffer to start writing the data
 *
 * output parameters
 *
 * returns DWT_DECA_SUCCESS for success, or DWT_DECA_ERROR for error
 *
 */
int dwt_writetxfctrl(uint16 txFrameLength, uint16 txBufferOffset)
{

#ifdef DWT_API_ERROR_CHECK
    if (dw1000local.longFrames)
    {
        if (txFrameLength > 1023) return DWT_ERROR ;
    }
    else
    {
        if (txFrameLength > 127) return DWT_ERROR ;
    }
    if (txFrameLength < 2) return DWT_ERROR ;
#endif

    // write the frame length to the TX frame control register
    // dw1000local.txFCTRL has kept configured bit rate information
    uint32 reg32 = dw1000local.txFCTRL | txFrameLength | (txBufferOffset << 22);
    dwt_write32bitoffsetreg(TX_FCTRL_ID,0,reg32) ;

    return DWT_SUCCESS ;

} // end dwt_writetxfctrl()


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_readrxdata()
 *
 *  Description: This is used to read the data from the RX buffer, from an offset location give by offset paramter
 *
 * input parameters
 * @param buffer - the buffer into which the data will be read
 * @param length - the length of data to read (in bytes)
 * @param rxBufferOffset - the offset in the rx buffer from which to read the data
 *
 * output parameters
 *
 * no return value
 */
void dwt_readrxdata(uint8 *buffer, uint16 length, uint16 rxBufferOffset)
{
    dwt_readfromdevice(RX_BUFFER_ID,rxBufferOffset,length,buffer) ;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_readaccdata()
 *
 *  Description: This is used to read the data from the Accumulator buffer, from an offset location give by offset parameter
 *
 * input parameters
 * @param buffer - the buffer into which the data will be read
 * @param length - the length of data to read (in bytes)
 * @param rxBufferOffset - the offset in the acc buffer from which to read the data
 *
 * output parameters
 *
 * no return value
 */
void dwt_readaccdata(uint8 *buffer, uint16 len, uint16 rxBufferOffset)
{
    // Force on the ACC clocks if we are sequenced
    _dwt_enableclocks(READ_ACC_ON);

    dwt_readfromdevice(RX_ACCD_ID,0,len,buffer) ;

    _dwt_enableclocks(READ_ACC_OFF); //revert clocks back
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_readdignostics()
 *
 *  Description: Description: this function reads the rx signal quality diagnostic data
 *
 * input parameters
 * @param diagnostics - diagnostic structure pointer, this will contain the diagnostic data read from the DW1000
 *
 * output parameters
 *
 * no return value
 */
void dwt_readdignostics(dwt_rxdiag_t *diagnostics)
{
    uint8 reg[2];
    uint16 fp;
    dwt_readfromdevice(RX_TIME_ID, 5, 2, reg) ; //read the HW FP index
    //LDE result 0 is in bytes 40->55
    // convert (and save) hardware estimate to show on graph
    fp = reg[0];
    fp = fp + (reg[1] << 8);
    diagnostics->firstPath = (double) fp * (1.0/64.0) ;

    //LDE diagnostic data
    diagnostics->maxNoise = dwt_read16bitoffsetreg(LDE_CFG_STS_ID, 0x0);

    //read all 8 bytes in one SPI transaction
    dwt_readfromdevice(RX_FQUAL_ID, 0x0, 8, (uint8*)&diagnostics->stdNoise);
    //diagnostics->stdNoise = dwt_read16bitoffsetreg(RX_FQUAL_ID, 0x0) ;
    diagnostics->firstPathAmp2 = dwt_read16bitoffsetreg(RX_FQUAL_ID, 0x2) ;
    diagnostics->firstPathAmp3 = dwt_read16bitoffsetreg(RX_FQUAL_ID, 0x4) ;
    diagnostics->maxGrowthCIR = dwt_read16bitoffsetreg(RX_FQUAL_ID, 0x6) ;

    diagnostics->firstPathAmp1 = dwt_read16bitoffsetreg(RX_TIME_ID, 0x7) ;

    diagnostics->rxPreamCount = (dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXP2SS_MASK) >> RX_FINFO_RXP2SS_SHIFT  ;

    //diagnostics->debug1 = dwt_read32bitoffsetreg(0x27, 0x28);
    //diagnostics->debug2 = dwt_read32bitoffsetreg(0x27, 0xc);

}



/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_readtxtimestamp()
 *
 *  Description: This is used to read the TX timestamp (adjusted with the programmed antenna delay)
 *
 * input parameters
 * @param timestamp - a pointer to a 5-byte buffer which will store the read TX timestamp time
 *
 * output parameters - the timestamp buffer will contain the value after the function call
 *
 * no return value
 */
void dwt_readtxtimestamp(uint8 * timestamp)
{
    dwt_readfromdevice(TX_TIME_ID, 0, TX_TIME_LEN, timestamp) ; // read bytes directly into buffer
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_readtxtimestamphi32()
 *
 *  Description: This is used to read the high 32-bits of the TX timestamp (adjusted with the programmed antenna delay)
 *
 * input parameters
 *
 * output parameters
 *
 * returns high 32-bits of TX timestamp
 */
uint32 dwt_readtxtimestamphi32(void)
{
    return dwt_read32bitoffsetreg(TX_TIME_ID, 1);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_readrxtimestamp()
 *
 *  Description: This is used to read the RX timestamp (adjusted time of arrival)
 *
 * input parameters
 * @param timestamp - a pointer to a 5-byte buffer which will store the read RX timestamp time
 *
 * output parameters - the timestamp buffer will contain the value after the function call
 *
 * no return value
 */
void dwt_readrxtimestamp(uint8 * timestamp)
{
    dwt_readfromdevice(RX_TIME_ID, 0, RX_TIME_LEN, timestamp) ; //get the adjusted time of arrival
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_readrxtimestamphi32()
 *
 *  Description: This is used to read the high 32-bits of the RX timestamp (adjusted with the programmed antenna delay)
 *
 * input parameters
 *
 * output parameters
 * uint32 rxtimestamp (hi 32-bits of RX timestamp)
 */
uint32 dwt_readrxtimestamphi32(void)
{
    return dwt_read32bitoffsetreg(RX_TIME_ID, 1);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_readsystimestamphi32()
 *
 *  Description: This is used to read the high 32-bits of the system time
 *
 * input parameters
 *
 * output parameters
 *
 * returns high 32-bits of system time timestamp
 */
uint32 dwt_readsystimestamphi32(void)
{
    return dwt_read32bitoffsetreg(SYS_TIME_L_ID, 1);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_readsystime()
 *
 *  Description: This is used to read the system time
 *
 * input parameters
 * @param timestamp - a pointer to a 5-byte buffer which will store the read system time
 *
 * output parameters
 * @param timestamp - the timestamp buffer will contain the value after the function call
 *
 * returns
 */
void dwt_readsystime(uint8 * timestamp)
{
    dwt_readfromdevice(SYS_TIME_L_ID, 0, SYS_TIME_LEN, timestamp) ; //get the adjusted time of arrival
}


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_writetodevice()
 *
 * Description:  this function is used to write to the DW1000 device registers
 * Notes:
 *        1. Firstly we create a header (the first byte is a header byte)
 *        a. check if sub index is used, if subindexing is used - set bit-6 to 1 to signify that the sub-index address follows the register index byte
 *        b. set bit-7 (or with 0x80) for write operation
 *        c. if extended sub address index is used (i.e. if index > 127) set bit-7 of the first sub-index byte following the first header byte
 *
 *        2. Write the header followed by the data bytes to the DW1000 device
 *
 *
 * input parameters:
 * @param recordNumber  - ID of register file or buffer being accessed
 * @param index         - byte index into register file or buffer being accessed
 * @param length        - number of bytes being written
 * @param buffer        - pointer to buffer containing the 'length' bytes to be written
 *
 * output parameters
 *
 * returns DWT_DECA_SUCCESS for success, or DWT_DECA_ERROR for error
 */
int dwt_writetodevice
(
    uint16      recordNumber,
    uint16      index,
    uint32      length,
    const uint8 *buffer
)
{
    uint8 header[3] ;                                       // buffer to compose header in
    int   cnt = 0;                                          // counter for length of header

    if (recordNumber > 0x3F) return DWT_ERROR ;                    // record number is limited to 6-bits.

    // Write message header selecting WRITE operation and addresses as appropriate (this is one to three bytes long)

    if (index == 0)                                         // for index of 0, no sub-index is required
    {
        header[cnt++] = 0x80 | recordNumber ;               // bit-7 is WRITE operation, bit-6 zero=NO sub-addressing, bits 5-0 is reg file id
    }
    else
    {
        if (index > 0x7FFF) return DWT_ERROR ;                     // index is limited to 15-bits.
        if ((index + length)> 0x7FFF) return DWT_ERROR ;           // sub-addressable area is limited to 15-bits.

        header[cnt++] = 0xC0 | recordNumber ;               // bit-7 is WRITE operation, bit-6 one=sub-address follows, bits 5-0 is reg file id

        if (index <= 127)                                   // for non-zero index < 127, just a single sub-index byte is required
        {
            header[cnt++] = (uint8)index ;                  // bit-7 zero means no extension, bits 6-0 is index.
        }
        else
        {
            header[cnt++] = 0x80 | (uint8)(index) ;           // bit-7 one means extended index, bits 6-0 is low seven bits of index.
            header[cnt++] =  (uint8) (index >> 7) ;           // 8-bit value = high eight bits of index.
        }
    }

    // write it to the SPI

    return writetospi(cnt,header,length,buffer);

} // end dwt_writetodevice()

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_readfromdevice()
 *
 * Description:  this function is used to read from the DW1000 device registers
 * Notes:
 *        1. Firstly we create a header (the first byte is a header byte)
 *        a. check if sub index is used, if subindexing is used - set bit-6 to 1 to signify that the sub-index address follows the register index byte
 *        b. set bit-7 (or with 0x80) for write operation
 *        c. if extended sub address index is used (i.e. if index > 127) set bit-7 of the first sub-index byte following the first header byte
 *
 *        2. Write the header followed by the data bytes to the DW1000 device
 *        3. Store the read data in the input buffer
 *
 * input parameters:
 * @param recordNumber  - ID of register file or buffer being accessed
 * @param index         - byte index into register file or buffer being accessed
 * @param length        - number of bytes being read
 * @param buffer        - pointer to buffer in which to return the read data.
 *
 * output parameters
 *
 * returns DWT_DECA_SUCCESS for success, or DWT_DECA_ERROR for error
 */
int dwt_readfromdevice
(
    uint16  recordNumber,
    uint16  index,
    uint32  length,
    uint8   *buffer
)
{
    uint8 header[3] ;                                       // buffer to compose header in
    int   cnt = 0;                                          // counter for length of header

    if (recordNumber > 0x3F) return DWT_ERROR ;                    // record number is limited to 6-bits.

    // Write message header selecting READ operation and addresses as appropriate (this is one to three bytes long)

    if (index == 0)                                         // for index of 0, no sub-index is required
    {
        header[cnt++] = (uint8) recordNumber ;              // bit-7 zero is READ operation, bit-6 zero=NO sub-addressing, bits 5-0 is reg file id
    }
    else
    {
        if (index > 0x7FFF) return DWT_ERROR ;                     // index is limited to 15-bits.
        if ((index + length)> 0x7FFF) return DWT_ERROR ;           // sub-addressible area is limited to 15-bits.

        header[cnt++] = (uint8)(0x40 | recordNumber) ;      // bit-7 zero is READ operation, bit-6 one=sub-address follows, bits 5-0 is reg file id

        if (index <= 127)                                   // for non-zero index < 127, just a single sub-index byte is required
        {
            header[cnt++] = (uint8) index ;                 // bit-7 zero means no extension, bits 6-0 is index.
        }
        else
        {
            header[cnt++] = 0x80 | (uint8)(index) ;           // bit-7 one means extended index, bits 6-0 is low seven bits of index.
            header[cnt++] =  (uint8) (index >> 7) ;           // 8-bit value = high eight bits of index.
        }
    }

    // do the read from the SPI

    return readfromspi(cnt, header, length, buffer);  // result is stored in the buffer

} // end dwt_readfromdevice()



/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_read32bitoffsetreg()
 *
 * Description:  this function is used to read 32-bit value from the DW1000 device registers
 *
 * input parameters:
 * @param regFileID - ID of register file or buffer being accessed
 * @param regOffset - the index into register file or buffer being accessed
 *
 * output parameters
 *
 * returns 32 bit register value (success), or DWT_DECA_ERROR for error
 */
uint32 dwt_read32bitoffsetreg(int regFileID,int regOffset)
{
    uint32  regval = DWT_ERROR ;
    int     j ;
    uint8   buffer[4] ;

    int result = dwt_readfromdevice(regFileID,regOffset,4,buffer); // read 4 bytes (32-bits) register into buffer

    if(result == DWT_SUCCESS)
    {
        for (j = 3 ; j >= 0 ; j --)
        {
            regval = (regval << 8) + buffer[j] ;        // sum
        }
    }
    return regval ;

} // end dwt_read32bitoffsetreg()

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_read16bitoffsetreg()
 *
 * Description:  this function is used to read 16-bit value from the DW1000 device registers
 *
 * input parameters:
 * @param regFileID - ID of register file or buffer being accessed
 * @param regOffset - the index into register file or buffer being accessed
 *
 * output parameters
 *
 * returns 16 bit register value (success), or DWT_DECA_ERROR for error
 */
uint16 dwt_read16bitoffsetreg(int regFileID,int regOffset)
{
    uint16  regval = DWT_ERROR ;
    uint8   buffer[2] ;

    int result = dwt_readfromdevice(regFileID,regOffset,2,buffer); // read 2 bytes (16-bits) register into buffer

    if(result == DWT_SUCCESS)
    {
        regval = (buffer[1] << 8) + buffer[0] ;        // sum
    }
    return regval ;

} // end dwt_read16bitoffsetreg()

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_write16bitoffsetreg()
 *
 * Description:  this function is used to write 16-bit value to the DW1000 device registers
 *
 * input parameters:
 * @param regFileID - ID of register file or buffer being accessed
 * @param regOffset - the index into register file or buffer being accessed
 * @param regval    - the value to write
 *
 * output parameters
 *
 * returns DWT_DECA_SUCCESS for success, or DWT_DECA_ERROR for error
 */
int dwt_write16bitoffsetreg(int regFileID,int regOffset,uint16 regval)
{
    int reg;
    uint8   buffer[2] ;

    buffer[0] = regval & 0xFF;
    buffer[1] = regval >> 8 ;

    reg = dwt_writetodevice(regFileID,regOffset,2,buffer);

    return reg;

} // end dwt_write16bitoffsetreg()

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_write32bitoffsetreg()
 *
 * Description:  this function is used to write 32-bit value to the DW1000 device registers
 *
 * input parameters:
 * @param regFileID - ID of register file or buffer being accessed
 * @param regOffset - the index into register file or buffer being accessed
 * @param regval    - the value to write
 *
 * output parameters
 *
 * returns DWT_DECA_SUCCESS for success, or DWT_DECA_ERROR for error
 */
int dwt_write32bitoffsetreg(int regFileID,int regOffset,uint32 regval)
{
    int     j ;
    int reg;
    uint8   buffer[4] ;

    for ( j = 0 ; j < 4 ; j++ )
    {
        buffer[j] = regval & 0xff ;
        regval >>= 8 ;
    }

    reg = dwt_writetodevice(regFileID,regOffset,4,buffer);

    return reg;

} // end dwt_write32bitoffsetreg()

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_enableframefilter()
 *
 *  Description: This is used to enable the frame filtering - (the default option is to
 *  accept any data and ack frames with correct destination address
 *
 * input parameters
 * @param - bitmask - enables/disables the frame filtering options according to
 *      DWT_FF_NOTYPE_EN        0x000   no frame types allowed
 *      DWT_FF_COORD_EN         0x002   behave as coordinator (can receive frames with no destination address (PAN ID has to match))
 *      DWT_FF_BEACON_EN        0x004   beacon frames allowed
 *      DWT_FF_DATA_EN          0x008   data frames allowed
 *      DWT_FF_ACK_EN           0x010   ack frames allowed
 *      DWT_FF_MAC_EN           0x020   mac control frames allowed
 *      DWT_FF_RSVD_EN          0x040   reserved frame types allowed
 *
 * output parameters
 *
 * no return value
 */
void dwt_enableframefilter(uint16 enable)
{
    uint32 sysconfig = dwt_read32bitreg(SYS_CFG_ID) ;            // read sysconfig register

    if(enable)
    {
        //enable frame filtering and configure frame types
        sysconfig &= ~(FF_ALL_EN);   //clear all
        sysconfig |= (enable & FF_ALL_EN) | SYS_CFG_FF;
    }
    else
        sysconfig &= ~SYS_CFG_FF;

    dw1000local.sysCFGreg = sysconfig ;
    dwt_write32bitreg(SYS_CFG_ID,sysconfig) ;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_setpanid()
 *
 *  Description: This is used to set the PAN ID and 16-bit (short) address
 *
 *
 * input parameters
 * @param panID - this is the PAN ID
 *
 * output parameters
 *
 * no return value
 */
void dwt_setpanid(uint16 panID)
{
    // PAN ID is high 16 bits of register
    dwt_write16bitoffsetreg(PANADR_ID, 2, panID) ;     // set the value
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_setaddress16()
 *
 *  Description: This is used to set 16-bit (short) address
 *
 *
 * input parameters
 * @param shortAddress - this sets the 16 bit short address
 *
 * output parameters
 *
 * no return value
 */
void dwt_setaddress16(uint16 shortAddress)
{
    // short address into low 16 bits
    dwt_write16bitoffsetreg(PANADR_ID, 0, shortAddress) ;     // set the value
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_seteui()
 *
 *  Description: This is used to set the EUI 64-bit (long) address
 *
 * input parameters
 * @param eui64 - this is the pointer to a buffer that contains the 64bit address
 *
 * output parameters
 *
 * no return value
 */
void dwt_seteui(uint8 *eui64)
{
    dwt_writetodevice(EUI_64_ID, 0x0, 8, eui64);
}


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_geteui()
 *
 *  Description: This is used to get the EUI 64-bit from the DW1000
 *
 * input parameters
 * @param eui64 - this is the pointer to a buffer that will contain the read 64-bit EUI value
 *
 * output parameters
 *
 * no return value
 */
void dwt_geteui(uint8 *eui64)
{
    dwt_readfromdevice(EUI_64_ID, 0x0, 8, eui64);
}

//
// _dwt_nvmread - function to read the NVM memory. Ensure that MR,MRa,MRb are reset to 0.
//
uint32 _dwt_nvmread(uint32 address)
{
    uint8 buf[4];
    uint32 ret_data;

    buf[1] = (address>>8) & 0xff;
    buf[0] = address & 0xff;

    // Write the address
    dwt_writetodevice(NVRAM_IF_ID,0x04,2,buf);

    // Assert NVM Read (self clearing)
    buf[1] = 0x20; // Enable slow clock
    buf[0] = 0x03; // 0x02 for auto read, 0x03 for manual drive of NVM_READ
    dwt_writetodevice(NVRAM_IF_ID,0x06,2,buf);
    buf[0] = 0x00; // Bit0 is not autoclearing, so clear it (Bit 1 is but we clear it anyway).
    dwt_writetodevice(NVRAM_IF_ID,0x06,1,buf);

    // Read read data, available 40ns after rising edge of NVM_READ
    ret_data=dwt_read32bitoffsetreg(NVRAM_IF_ID,0x0A);

    // Return the 32bit of read data
    return (ret_data);
}

// nvm_set_mr_regs
// Configure the MR registers for initial programming (enable charge pump).
// Read margin is used to stress the read back from the
// programmed bit. In normal operation this is relaxed.
uint32 _dwt_nvmsetmrregs(int mode)
{
    uint8 rd_buf[4];
    uint8 wr_buf[4];
    uint32 mra,mrb,mr;
    //printf("NVM SET MR: Setting MR,MRa,MRb for mode %2x\n",mode);

    // For mode we will set the following:
    // For mode we will set the following:
    // "0" : Reset all to 0x0:           MRA=0x0000, MRB=0x0000, MR=0x0000
    // "1" : Set for inital programming: MRA=0x9220, MRB=0x000E, MR=0x1024
    // "2" : Set for soak programming:   MRA=0x9220, MRB=0x0003, MR=0x1824
    // "3" : High Vpp:                   MRA=0x9220, MRB=0x004E, MR=0x1824
    // "4" : Low Read Margin:            MRA=0x0000, MRB=0x000E, MR=0x0000
    // "5" : Array Clean:                MRA=0x0049, MRB=0x0003, MR=0x0024
    // "4" : Very Low Read Margin:       MRA=0x0000, MRB=0x000E, MR=0x0000
    ///////////////////////////////////////////

    // PROGRAMME MRA
    // Set MRA, MODE_SEL
    wr_buf[0] = 0x03;
    dwt_writetodevice(NVRAM_IF_ID, 0x07,1,wr_buf);

    // Load data
    switch(mode&0x0f) {
    case 0x0 :
        mr =0x0000;
        mra=0x0000;
        mrb=0x0000;
        break;
    case 0x1 :
        mr =0x1024;
        mra=0x9220; // Enable CPP mon
        mrb=0x000e;
        break;
    case 0x2 :
        mr =0x1824;
        mra=0x9220;
        mrb=0x0003;
        break;
    case 0x3 :
        mr =0x1824;
        mra=0x9220;
        mrb=0x004e;
        break;
    case 0x4 :
        mr =0x0000;
        mra=0x0000;
        mrb=0x0003;
        break;
    case 0x5 :
        mr =0x0024;
        mra=0x0000;
        mrb=0x0003;
        break;
    default :
    //  printf("NVM SET MR: ERROR : Invalid mode selected\n",mode);
        break;
    }

    wr_buf[0] = mra & 0x00ff;
    wr_buf[1] = (mra & 0xff00)>>8;
    dwt_writetodevice(NVRAM_IF_ID, 0x00,2,wr_buf);


    // Set WRITE_MR
    wr_buf[0] = 0x08;
    dwt_writetodevice(NVRAM_IF_ID, 0x06,1,wr_buf);

    // Wait?

    // Set Clear Mode sel
    wr_buf[0] = 0x02;
    dwt_writetodevice(NVRAM_IF_ID,0x07,1,wr_buf);

    // Set AUX update, write MR
    wr_buf[0] = 0x88;
    dwt_writetodevice(NVRAM_IF_ID,0x06,1,wr_buf);
    // Clear write MR
    wr_buf[0] = 0x80;
    dwt_writetodevice(NVRAM_IF_ID,0x06,1,wr_buf);
    // Clear AUX update
    wr_buf[0] = 0x00;
    dwt_writetodevice(NVRAM_IF_ID,0x06,1,wr_buf);

    ///////////////////////////////////////////
    // PROGRAMME MRB
    // Set SLOW, MRB, MODE_SEL
    wr_buf[0] = 0x05;
    dwt_writetodevice(NVRAM_IF_ID,0x07,1,wr_buf);

    wr_buf[0] = mrb & 0x00ff;
    wr_buf[1] = (mrb & 0xff00)>>8;
    dwt_writetodevice(NVRAM_IF_ID,0x00,2,wr_buf);

    // Set WRITE_MR
    wr_buf[0] = 0x08;
    dwt_writetodevice(NVRAM_IF_ID,0x06,1,wr_buf);

    // Wait?

    // Set Clear Mode sel
    wr_buf[0] = 0x04;
    dwt_writetodevice(NVRAM_IF_ID,0x07,1,wr_buf);

    // Set AUX update, write MR
    wr_buf[0] = 0x88;
    dwt_writetodevice(NVRAM_IF_ID,0x06,1,wr_buf);
    // Clear write MR
    wr_buf[0] = 0x80;
    dwt_writetodevice(NVRAM_IF_ID,0x06,1,wr_buf);
    // Clear AUX update
    wr_buf[0] = 0x00;
    dwt_writetodevice(NVRAM_IF_ID,0x06,1,wr_buf);

    ///////////////////////////////////////////
    // PROGRAMME MR
    // Set SLOW, MODE_SEL
    wr_buf[0] = 0x01;
    dwt_writetodevice(NVRAM_IF_ID,0x07,1,wr_buf);
    // Load data

    wr_buf[0] = mr & 0x00ff;
    wr_buf[1] = (mr & 0xff00)>>8;
    dwt_writetodevice(NVRAM_IF_ID,0x00,2,wr_buf);

    // Set WRITE_MR
    wr_buf[0] = 0x08;
    dwt_writetodevice(NVRAM_IF_ID,0x06,1,wr_buf);

    // Wait?
    Delay_ms(10);
    // Set Clear Mode sel
    wr_buf[0] = 0x00;
    dwt_writetodevice(NVRAM_IF_ID,0x07,1,wr_buf);

    // Read confirm mode writes.
    //Set man override, MRA_SEL
    wr_buf[0] = 0x01;
    dwt_writetodevice(NVRAM_IF_ID,0x06,1,wr_buf);
    wr_buf[0] = 0x02;
    dwt_writetodevice(NVRAM_IF_ID,0x07,1,wr_buf);
    //dwt_readfromdevice(NVRAM_IF_ID,0x0A,4,rd_buf);
    //printf("NVM READ MR: %02x %02x MRA:%02x %02x\n",rd_buf[3], rd_buf[2], rd_buf[1], rd_buf[0]);
    //MRB_SEL
    wr_buf[0] = 0x04;
    dwt_writetodevice(NVRAM_IF_ID,0x07,1,wr_buf);
    Delay_ms(100);
    //dwt_readfromdevice(NVRAM_IF_ID,0x0A,4,rd_buf);
    //printf("NVM READ MR: %02x %02x MRB:%02x %02x\n",rd_buf[3], rd_buf[2], rd_buf[1], rd_buf[0]);

    // Clear mode sel
    wr_buf[0] = 0x00;
    dwt_writetodevice(NVRAM_IF_ID,0x07,1,wr_buf);
    // Clear MAN_OVERRIDE
    wr_buf[0] = 0x00;
    dwt_writetodevice(NVRAM_IF_ID,0x06,1,wr_buf);

    Delay_ms(10);

    if (((mode&0x0f) == 0x1) |((mode&0x0f) == 0x2)) {
        // Read status register
        dwt_readfromdevice(NVRAM_IF_ID,0x08,1,rd_buf);

        if((rd_buf[0]&0x02) == 0x02) {
    //      printf("(Info) NVM PROG 32: Charge Pump OK\n");
        } else {
    //      printf("(Info) NVM PROG 32: Charge Pump Not Ready\n");
         return 0;
        }
    }

    return 0;
}

//
// _dwt_nvmprogword32 - function to program the NVM memory. Ensure that MR,MRa,MRb are reset to 0.
// VNM Charge pump needs to be enabled(see nvm_set_mr_regs)
// Note the address is only 11 bits long.
uint32 _dwt_nvmprogword32(uint32 data, uint16 address)
{
    uint8 rd_buf[1];
    uint8 wr_buf[4];
    uint8 nvm_done;

    // Read status register
    dwt_readfromdevice(NVRAM_IF_ID, 0x08, 1, rd_buf);

    if((rd_buf[0] & 0x02) != 0x02)
    {
        printf("NVM PROG 32: ERROR VPP NOT OK, programming will fail. Are MR/MRA/MRB set?\n");
        return DWT_ERROR;
    }

    // Write the data
    wr_buf[3] = (data>>24) & 0xff;
    wr_buf[2] = (data>>16) & 0xff;
    wr_buf[1] = (data>>8) & 0xff;
    wr_buf[0] = data & 0xff;
    dwt_writetodevice(NVRAM_IF_ID, 0x00, 4, wr_buf);

    // Write the address [10:0]
    wr_buf[1] = (address>>8) & 0x07;
    wr_buf[0] = address & 0xff;
    dwt_writetodevice(NVRAM_IF_ID, 0x04, 2, wr_buf);

    // Enable Sequenced programming
    wr_buf[0] = 0x40;
    dwt_writetodevice(NVRAM_IF_ID, 0x06, 1, wr_buf);
    wr_buf[0] = 0x00; // And clear
    dwt_writetodevice(NVRAM_IF_ID, 0x06, 1, wr_buf);

    // WAIT for status to flag PRGM OK..
    nvm_done = 0;
    while(nvm_done == 0)
    {
        dwt_readfromdevice(NVRAM_IF_ID, 0x08, 1, rd_buf);

        if((rd_buf[0] & 0x01) == 0x01)
        {
            nvm_done = 1;
        }
    }

    return DWT_SUCCESS;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_otpwriteandverify()
 *
 *  Description: This is used to program 32-bit value into the DW1000 OTP memory.
 *
 * input parameters
 * @param value - this is the 32-bit value to be progarmmed into OTP
 * @param address - this is the 16-bit OTP address into which the 32-bit value is programmed
 *
 * output parameters
 *
 * returns DWT_DECA_SUCCESS for success, or DWT_DECA_ERROR for error
 */
uint32 dwt_otpwriteandverify(uint32 value, uint16 address)
{
    int prog_ok = 0;
    int retry = 0;
    //firstly set the system clock to crystal
    _dwt_enableclocks(FORCE_SYS_XTI); //set system clock to XTI

    //
    //!!!!!!!!!!!!!! NOTE !!!!!!!!!!!!!!!!!!!!!
    //Set the supply to 3.7V
    //

    _dwt_nvmsetmrregs(1); //set mode for programming

    //for each value to program - the readback/chack is done couple of times to verify it has programmed successfully
    while(prog_ok==0)
    {
        _dwt_nvmprogword32(value, address);

        if(_dwt_nvmread(address) == value)
        {
            prog_ok = 1;
        }
        retry++;
        if(retry==5)
            prog_ok = 2;
    }

    //even if the above does not exit before retry reaches 5, the programming has probably been sucessful

    _dwt_nvmsetmrregs(4); //set mode for reading

    if(_dwt_nvmread(address) == value)   //if this does not pass please check voltage supply on VDDIO
    {
        prog_ok = DWT_SUCCESS;
    }
    else
        prog_ok = DWT_ERROR;

    _dwt_nvmsetmrregs(0); //setting NVM mode register for low RM read - resetting the device would be alternative

    return prog_ok;
}
// upload always on array configuration
void _dwt_aonconfigupload(void)
{
    uint8 buf[1];

    // Upload array
    buf[0] = 0x04;
    dwt_writetodevice(AON_ID,0x02,1,buf);
    buf[0] = 0x00;
    dwt_writetodevice(AON_ID,0x02,1,buf);
}

// upload always on array configuration and go to sleep
void _dwt_aonarrayupload(void)
{
    uint8 buf[1];

    // Upload array
    buf[0] = 0x00;
    dwt_writetodevice(AON_ID,0x02,1,buf);
    buf[0] = 0x02;
    dwt_writetodevice(AON_ID,0x02,1,buf);
}


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_entersleep()
 *
 *  Description: This function puts the device into deep sleep or sleep.  dwt_configuredeepsleep should be called first
 *  to configure the sleep and on-wake/wakeup parameters
 *
 *  input parameters
 *
 *  output parameters
 *
 * no return value
 */
void dwt_entersleep(void)
{
    //copy config to AON - upload the new configuration
    _dwt_aonarrayupload();
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_configuresleepcnt()
 *
 * Description: sets the sleep counter to new value, this function programs the high 16-bits of the 28-bit counter
 *
 * NOTE: this function needs to be run before dwt_configuresleep, also the SPI freq has to be < 3MHz
 *
 * input parameters
 * @param sleepcnt - this it value of the sleep counter to program
 *
 * output parameters
 *
 * no return value
 */
void dwt_configuresleepcnt(uint16 sleepcnt)
{
    uint8 buf[2];

    buf[0] = 0x01;
    dwt_writetodevice(PMSC_ID,0x00,1,buf);

    buf[0] = 0;
    dwt_writetodevice(AON_ID, 0x6, 1, buf); //to make sure we don't accidentaly go to sleep

    buf[0] = 0;
    dwt_writetodevice(AON_ID, 0xA, 1, buf);
    //disable the sleep counter
    _dwt_aonconfigupload();

    //set new value
    buf[0] = sleepcnt & 0xFF;
    buf[1] = (sleepcnt >> 8) & 0xFF;
    dwt_writetodevice(AON_ID, 0x8, 2, buf);
    _dwt_aonconfigupload();

    //enable the sleep counter
    buf[0] = 1;
    dwt_writetodevice(AON_ID, 0xA, 1, buf);
    _dwt_aonconfigupload();

    buf[0] = 0x00;
    dwt_writetodevice(PMSC_ID,0x00,1,buf);
}


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_calibratesleepcnt()
 *
 * Description: calibrates the local oscillator as its frequency can vary between 7 and 13kHz depending on temp and voltage
 *
 * NOTE: this function needs to be run before dwt_configuresleepcnt, so that we know what the counter units are
 * returns the number of XTAL/2 cycles per low-power oscillator cycle
 *
 * To get LP OSC freq = (XTAL/2)/result = 19.2MHz/result
 *
 * input parameters
 *
 * output parameters
 *
 * no return value
 */
uint16 dwt_calibratesleepcnt(void)
{
    uint8 buf[2];
    uint16 result;

    //enable cal of the sleep counter
    buf[0] = 4;
    dwt_writetodevice(AON_ID, 0xA, 1, buf);
    _dwt_aonconfigupload();

    //disables the cal of the sleep counter
    buf[0] = 0;
    dwt_writetodevice(AON_ID, 0xA, 1, buf);
    _dwt_aonconfigupload();

    buf[0] = 0x01;
    dwt_writetodevice(PMSC_ID,0x00,1,buf);
    Delay_ms(1);

    //read the number of XTAL/2 cycles one lposc cycle took.
    // Set up address
    buf[0] = 118;
    dwt_writetodevice(AON_ID,0x04,1,buf);

     // Enable manual override
    buf[0] = 0x80; // OVR EN
    dwt_writetodevice(AON_ID,0x2,1,buf);

    // Read confirm data that was written
    buf[0] = 0x88; // OVR EN, OVR_RD
    dwt_writetodevice(AON_ID,0x2,1,buf);

    // Read back byte from AON
    dwt_readfromdevice(AON_ID,3,1,buf);
    result = buf[0];
    result = result << 8;

    // Set up address
    buf[0] = 117;
    dwt_writetodevice(AON_ID,0x04,1,buf);

     // Enable manual override
    buf[0] = 0x80; // OVR EN
    dwt_writetodevice(AON_ID,0x2,1,buf);

    // Read confirm data that was written
    buf[0] = 0x88; // OVR EN, OVR_RD
    dwt_writetodevice(AON_ID,0x2,1,buf);

    // Read back byte from AON
    dwt_readfromdevice(AON_ID,3,1,buf);
    result |= buf[0];

    buf[0] = 0x00; // Disable OVR EN
    dwt_writetodevice(AON_ID,0x2,1,buf);

    buf[0] = 0x00;
    dwt_writetodevice(PMSC_ID,0x00,1,buf);

    //returns the number of XTAL/2 cycles per one LP OSC cycle
    //this can be converted into LP OSC freq by 19.2MHz/result
    return result;
}


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_configuresleep()
 *
 *  Description: configures the device for both DEEP_SLEEP and SLEEP modes, and on-wake mode
 *  I.e. before entering the sleep, the device should be programmed for TX or RX, then upon "waking up" the TX/RX settings
 *  will be preserved and the device can immediately perform the desired action TX/RX
 *
 * NOTE: e.g. MP :- Tag operation - after deep sleep, the device needs to just load the TX buffer and send the frame
 *
 *
 *      mode: the array and LDE code (NV RAM) and LDO tune, and set sleep persist
 *      DWT_LOADUCODE    0x800 - load ucode from NVMEM
 *      DWT_PRESRV_SLEEP 0x100 - preserve sleep
 *      DWT_LOADOPSET    0x080 - load operating parameter set on wakeup
 *      DWT_CONFIG       0x040 - download the AON array into the HIF (configuration download)
 *      DWT_LOADEUI      0x008
 *      DWT_GOTORX       0x002
 *      DWT_TANDV        0x001
 *
 *      wake: wake up parameters
 *		DWT_XTAL_EN		 0x10 - keep XTAL running during sleep
 *      DWT_WAKE_SLPCNT  0x8 - wake up after sleep count
 *      DWT_WAKE_CS      0x4 - wake up on chip select
 *      DWT_WAKE_WK      0x2 - wake up on WAKEUP PIN
 *      DWT_SLP_EN       0x1 - enable sleep/deep sleep functionality
 *
 * input parameters
 * @param mode - config on-wake parameters
 * @param wake - config wake up parameters
 *
 * output parameters
 *
 * no return value
 */
void dwt_configuresleep(uint16 mode, uint8 wake)
{
    uint8 buf[1];

    dwt_write16bitoffsetreg(AON_ID, 0x00, mode);

    buf[0] = wake;

    dwt_writetodevice(AON_ID, 0x06, 1, buf);

}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_entersleepaftertx(int enable)
 *
 *  Description: sets the auto TX to sleep bit. This means that after a frame
 *  transmission the device will enter deep sleep mode. The dwt_setdeepsleep() function
 *  needs to be called before this to configure the on-wake settings
 *
 * NOTE: the IRQ line has to be low/inactive (i.e. no pending events)
 *
 * input parameters
 * @param enable - 1 to configure the device to enter deep sleep after TX, 0 - disables the configuration
 *
 * output parameters
 *
 * no return value
 */
void dwt_entersleepaftertx(int enable)
{
    uint32 reg = dwt_read32bitoffsetreg(PMSC_ID, 0x4);
    //set the auto TX -> sleep bit
    if(enable)
        reg |= 0x800;
    else
        reg &= ~(0x800);

    dwt_write32bitoffsetreg(PMSC_ID, 0x4, reg);
}


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_spicswakeup()
 *
 * Description: wake up the device from sleep mode using the SPI read,
 * the device will wake up on chip slect line going low if the line is held low for at least 600us
 * NOTE: Alternatively the device can be woken up with WaKE_UP pin if configured for that operation
 *
 * input parameters
 * @param buff   - this is a pointer to the dummy buffer which will be used in the SPI read transaction used for the WAKE UP of the device
 * @param length - this is the length of the dummy buffer
 *
 * output parameters
 *
 * returns DWT_DECA_SUCCESS for success, or DWT_DECA_ERROR for error
 */
int dwt_spicswakeup(uint8 *buff, uint16 length)
{
    if(dwt_readdevid() != DWT_DEVICE_ID) //device was in deep sleep (the first read fails)
    {
        //need to keep chip select line low for at least 600us
        dwt_readfromdevice(0x0, 0x0, length, buff); //do a long read to wake up the chip (hold the chip select low) (@3M, 100 ~= 272us, 200 ~= 540us)

        //need 5ms for XTAL to start and stabilise (could wait for PLL lock IRQ status bit !!!)
        //NOTE: Polling of the STATUS register is not possible unless freq is < 3MHz
        Delay_ms(5);
    }
    else
        return DWT_SUCCESS;

    //DEBUG - check if still in sleep mode
    if(dwt_readdevid() != DWT_DEVICE_ID)
    {
        return DWT_ERROR;
    }

    return DWT_SUCCESS;
}


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: _dwt_configlde()
 *
 * Description: configure LDE algorithm parameters
 *
 * input parameters
 * @param prf   -   this is the PRF index (0 or 1) 0 correcpods to 16 and 1 to 64 PRF
 *
 * output parameters
 *
 * no return value
 */
void _dwt_configlde(int prfIndex)
{
    uint8 x = LDE_PARAM1;

    dwt_writetodevice( LDE_CFG_STS_ID, 0x806, 1, &x );

    if(prfIndex)
        dwt_write16bitoffsetreg( LDE_CFG_STS_ID, 0x1806, (uint16) LDE_PARAM3_64);
    else
        dwt_write16bitoffsetreg( LDE_CFG_STS_ID, 0x1806, (uint16) LDE_PARAM3_16);

}


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: _dwt_loaducodefromrom()
 *
 * Description:  load ucode from OTP MEMORY or ROM
 *
 * input parameters
 *
 * output parameters
 *
 * no return value
 */
int _dwt_loaducodefromrom(void)
{
    uint32 reg = dwt_read32bitoffsetreg(NVRAM_IF_ID, 0x6);
    uint8 wr_buf[2];

    //set up clocks
    wr_buf[1] = 0x23;
    wr_buf[0] = 0x11;
    dwt_writetodevice(PMSC_ID, 0x0, 2, wr_buf);
    //kick off the NV MEM load
    dwt_write32bitoffsetreg(NVRAM_IF_ID, 0x6, reg | LDE_LOAD_BIT); // set load LDE kick bit

    Delay_ms(1); // Allow time for code to upload (should take up to 120 us)

    //default clocks
    wr_buf[1] = 0x02;
    wr_buf[0] = 0x00;
    dwt_writetodevice(PMSC_ID, 0x0, 2, wr_buf);

    return DWT_SUCCESS ;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_loadopsettabfromotp()
 *
 *  Description: This is used to select which Operational Parameter Set table to load from OTP memory
 *
 * input parameters
 * @param - opset table selection
 *
 * output parameters
 *
 * no return value
 */
int dwt_loadopsettabfromotp(uint8 gtab_sel)
{
    uint8 wr_buf[2];
    uint16 reg = (((gtab_sel & 0x3) << 5) | 0x1);
    //set up clocks
    wr_buf[1] = 0x23;
    wr_buf[0] = 0x11;
    dwt_writetodevice(PMSC_ID, 0x0, 2, wr_buf);

    dwt_write16bitoffsetreg(NVRAM_IF_ID, 0x12, reg); // set load gtab kick bit (bit0) and gtab selection bit

    //default clocks
    wr_buf[1] = 0x02;
    wr_buf[0] = 0x00;
    dwt_writetodevice(PMSC_ID, 0x0, 2, wr_buf);

    return DWT_SUCCESS ;
}


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_setsmarttxpower()
 *
 * Description: This call enables or disables the smart TX power feature.
 *
 * input parameters
 * @param enable - this enables or disables the TX smart power (1 = enable, 0 = disable)
 *
 * output parameters
 *
 * returns
 */
void dwt_setsmarttxpower(int enable)
{
    //config system register
    dw1000local.sysCFGreg = dwt_read32bitreg(SYS_CFG_ID) ;            // read sysconfig register

    //disable smart power configuration
    if(enable)
        dw1000local.sysCFGreg &= ~SYS_CFG_DIS_SMART_TXP ;
    else
        dw1000local.sysCFGreg |= SYS_CFG_DIS_SMART_TXP ;

    dwt_write32bitreg(SYS_CFG_ID,dw1000local.sysCFGreg) ;
}


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_enableautoack()
 *
 *  Description: This call enables the auto-ACK feature. If the responseDelayTime (paramter) is 0, the ACK will be sent a.s.a.p.
 *  otherwise it will be sent with a programmed delay (in symbols), max is 255.
 *
 * input parameters
 * @param responseDelayTime - if non-zero the ACK is sent after this delay, max is 255.
 *
 * output parameters
 *
 * no return value
 */
void dwt_enableautoack(uint8 responseDelayTime)
{
    //enable auto ACK - needs to have frame filtering enabled as well
    dw1000local.sysCFGreg |= SYS_CFG_AUTACK;
    // auto ACK reply delay
    dwt_write16bitoffsetreg(ACK_RESP_ID, 0x2, (responseDelayTime << 8)) ; //in symbols

    dwt_write32bitreg(SYS_CFG_ID,dw1000local.sysCFGreg) ;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_setdblrxbuffmode()
 *
 *  Description: This call enables the double receive buffer mode
 *
 * input parameters
 * @param enable - 1 to enable, 0 to disable the double buffer mode
 *
 * output parameters
 *
 * no return value
 */
void dwt_setdblrxbuffmode(int enable)
{
    if(enable)
    {
        //enable double rx buffer mode
        dw1000local.sysCFGreg &= ~SYS_CFG_DIS_DRXB;
        dw1000local.dblbuffon = 1;
    }
    else
    {
        //disable double rx buffer mode
        dw1000local.sysCFGreg |= SYS_CFG_DIS_DRXB;
        dw1000local.dblbuffon = 0;
    }

    dwt_write32bitreg(SYS_CFG_ID,dw1000local.sysCFGreg) ;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_setautorxreenable()
 *
 *  Description: This call enables the auto rx re-enable feature
 *
 * input parameters
 * @param enable - 1 to enable, 0 to disable the feature
 *
 * output parameters
 *
 * no return value
 */
void dwt_setautorxreenable(int enable)
{
    if(enable)
    {
        //enable auto re-enable of the receiver
        dw1000local.sysCFGreg |= SYS_CFG_RXAUTR;
    }
    else
    {
        //disable auto re-enable of the receiver
        dw1000local.sysCFGreg &= ~SYS_CFG_RXAUTR;
    }

    dwt_write32bitreg(SYS_CFG_ID, dw1000local.sysCFGreg) ;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_setrxaftertxdelay()
 *
 *  Description: This sets the receiver turnon delay time after a transmission of a frame
 *
 * input parameters
 * @param rxDelayTime - (20 bits) - the delay is in micro seconds
 *
 * output parameters
 *
 * no return value
 */
void dwt_setrxaftertxdelay(uint32 rxDelayTime)
{
    uint32 val = dwt_read32bitreg(ACK_RESP_ID) ;          // read ACK_RESP_ID register

    val &= 0xFFF00000 ; //clear the timer (19:0)

    val |= rxDelayTime & 0xFFFFF; //in usec (turn the receiver 20us after TX)

    dwt_write32bitreg(ACK_RESP_ID, val) ;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_setcallbacks()
 *
 *  Description: This is the devices interrupt handler function, it will process/report status events
 *
 * input parameters
 * @param txcallback - the pointer to the TX callback function
 * @param rxcallback - the pointer to the RX callback function
 *
 * output parameters
 *
 * no return value
 */
void dwt_setcallbacks(void (*txcallback)(const dwt_callback_data_t *), void (*rxcallback)(const dwt_callback_data_t *))
{
    dw1000local.dwt_txcallback = txcallback;

    dw1000local.dwt_rxcallback = rxcallback;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_isr()
 *
 * Description: This is the devices interrupt handler function, it will process/report status events
 * Notes:  In PC based system using (Cheetah or ARM) USB to SPI converter there can be no interrupts, however we still need something
 *         to take the place of it and operate in a polled way.
 *         In an embedded system this function should be configured to launch on an interrupt, then it will process the interrupt trigger event and
 *         call a tx or rx callback function depending on whether the event is a TX or RX event.
 *         the tx call-back will be called when a frame has been sent and the rx call-back when a frame has been received.
 *
 * input parameters
 *
 * output parameters
 *
 * no return value
 */
void dwt_isr(void) // assume interrupt can supply context
{
    uint32  status = 0;
    //uint32 status1 = 0;
    //uint32 states = 0;
    //uint32 states1 = 0;
    uint32  clear = 0; // will clear any events seen
    //uint8 buffer[2];

    dw1000local.cdata.event = 0;

    status = dwt_read32bitreg(SY_STAT_ID) ;            // read status register

    //NOTES:
    //1. TX Event - if DWT_INT_TFRS is enabled, then when the frame has completed transmission the interrupt will be triggered.
    //   The status register will have the TXFRS bit set. This function will clear the tx event and call the dwt_txcallback function.
    //
    //2. RX Event - if DWT_INT_RFCG is enabled, then when a frame with good CRC has been received the interrupt will be triggered.
    //   The status register will have the RXFCG bit set. This function will clear the rx event and call the dwt_rxcallback function.
    //
    //2.a. RX Event - This is same as 2. above except this time the received frame has ACK request bit set in the header (AAT bit will be set).
    //     This function will clear the rx event and call the dwt_rxcallback function, notifying the application that ACK req is set.
    //     If using auto-ACK, the AAT indicates that ACK frame is in progress. Once the ACK has been sent the TXFRS bit will be set and Tx event triggered.
    //     If the auto-ACK is not enabled, the application can format and start transmission of own ACK frame.
    //
    //


    //DEBUG ----- DEBUG ----- DEBUG -----
    //status1 = dwt_read32bitoffsetreg(SY_STAT_ID, 1) ;            // read status register
    //dwt_readfromdevice(RX_BUFFER_ID,0,2,buffer) ;
    //printf("status %02X%08X, byte 0 %02X%02X\n", status1>>24, status, buffer[0], buffer[1]);
    //states = dwt_read32bitreg(0x19) ;            // read status register
    //states1 = dwt_read32bitoffsetreg(0x19, 1) ;            // read status register
    //printf("states %02X%08X\n", states1>>24, states);


    //
    // 1st check for RX frame received or RX timeout and if so call the rx callback function
    //
    if (status & SY_STAT_RFCG)  // Receiver FCS Good
    {
        // read frame info
        uint16 len = dwt_read32bitreg(RX_FINFO_ID) & 0x3FF;

        if (dw1000local.longFrames==0)
        {
            len &= 0x7F ;
        }

        // STD frame length up to 127, extended frame length up to 1023 bytes
        dw1000local.cdata.datalength = len ;

        //clear all receive status bits (as we are finished with this receive event)
        clear |= status & CLEAR_ALLRXGOOD_EVENTS  ;
        dwt_write32bitreg(SY_STAT_ID,clear) ;         // write status register to clear event bits we have seen
        //NOTE: clear the event which caused interrupt means once the rx is enabled or tx is started
        //new events can trigger and give rise to new interrupts

        dw1000local.cdata.event = DWT_SIG_RX_OKAY ;
        dw1000local.cdata.aatset = (status & SY_STAT_TFAA) ; //ACK request is set

        //call the RX call-back function to process the RX event
        if(dw1000local.dwt_rxcallback != NULL)
            dw1000local.dwt_rxcallback(&dw1000local.cdata);

        if(dw1000local.dblbuffon) //double RX buffer is enabled - toggle the host side pointer
        {
            uint8 hsrb = 0x1 ;
            //toggle the host side Receive Buffer Pointer by writing one to the register
            dwt_writetodevice(SYS_CTRL_ID, 3, 1, &hsrb) ;       // we need to swap rx buffer status reg (write one to toggle internally)
        }

    } // end if CRC is good
    else
    //
    // Check for TX frame sent event and signal to upper layer.
    //
    if (status & SY_STAT_TFRS)  // Transmit Frame Sent
    {
        clear |= CLEAR_ALLTX_EVENTS; //clear TX event bits
        dwt_write32bitreg(SY_STAT_ID,clear) ;         // write status register to clear event bits we have seen
        //NOTE: clear the event which caused interrupt means once the rx is enabled or tx is started
        //new events can trigger and give rise to new interrupts
        if(dw1000local.cdata.aatset)
        {
            dw1000local.cdata.aatset = 0; //the ACK has been sent
        }

        dw1000local.cdata.event = DWT_SIG_TX_DONE ;  // signal TX completed

        //call the TX call-back function to process the TX event
        if(dw1000local.dwt_txcallback != NULL)
            dw1000local.dwt_txcallback(&dw1000local.cdata);

    }
    else if (status & SY_STAT_RFTO)                                              // Receiver Frame Wait timeout:
    {
        clear |= status & SY_STAT_RFTO ;
        dwt_write32bitreg(SY_STAT_ID,clear) ;         // write status register to clear event bits we have seen
        dw1000local.cdata.event = DWT_SIG_RX_TIMEOUT  ;
        if(dw1000local.dwt_rxcallback != NULL)
            dw1000local.dwt_rxcallback(&dw1000local.cdata);

    }
    else if(status & CLEAR_ALLRXERROR_EVENTS)
    {
        clear |= status & CLEAR_ALLRXERROR_EVENTS;
        dwt_write32bitreg(SY_STAT_ID,clear) ;         // write status register to clear event bits we have seen
        //NOTE: clear the event which caused interrupt means once the rx is enabled or tx is started
        //new events can trigger and give rise to new interrupts
        if(status & SY_STAT_RPHE)
        {
            dw1000local.cdata.event = DWT_SIG_RX_PHR_ERROR  ;
        }
        else if(status & SY_STAT_RFCE)
        {
            dw1000local.cdata.event = DWT_SIG_RX_ERROR  ;
        }
        else if(status & SY_STAT_RFSL)
        {
            dw1000local.cdata.event = DWT_SIG_RX_SYNCLOSS  ;
        }
        else if(status & SY_STAT_SFDT)
        {
            dw1000local.cdata.event = DWT_SIG_RX_SFDTIMEOUT  ;
        }
        else if(status & SY_STAT_RXPTO)
        {
            dw1000local.cdata.event = DWT_SIG_RX_PTOTIMEOUT  ;
        }
        else
            dw1000local.cdata.event = DWT_SIG_RX_ERROR  ;

        if(dw1000local.dwt_rxcallback != NULL)
            dw1000local.dwt_rxcallback(&dw1000local.cdata);

    }

}  // end dwt_isr()

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_setleds()
 *
 *  Description: This is used to set up Tx/Rx GPIOs which could be used to control LEDs
 *  Note: not completely IC dependent, also needs board with LEDS fitted on right I/O lines
 *        this function enables GPIOs 2 and 3 which are connected to LED3 and LED4 on EVB1000
 *
 * input parameters
 * @param test - if 1 the LEDs will be enabled, if 0 the LED control is disabled.
 *             - if value is 2 the LEDs will flash once after enable.
 *
 * output parameters
 *
 * no return value
 */
void dwt_setleds(uint8 test)
{
    uint8 buf[2];

	if(test>0)
	{
    // Set up MFIO for LED output
    dwt_readfromdevice(GPIO_CTRL_ID,0x00,2,buf);
		//buf[1] |= 0x15; // Set GPIO3,2,1
		//buf[0] |= 0x40; // Set GPIO0
		//dwt_writetodevice(GPIO_CTRL_ID,0x00,2,buf);
		buf[0] |= 0x14;
		dwt_writetodevice(GPIO_CTRL_ID,0x01,1,buf);

    // Enable LP Oscillator to run from counter, turn on debounce clock
    dwt_readfromdevice(PMSC_ID,0x02,1,buf);
    buf[0] |= 0x84; //
    dwt_writetodevice(PMSC_ID,0x02,1,buf);

    // Enable LEDs to blink
    buf[0] = 0x10; // Blink period.
    buf[1] = 0x01; // Enable blink counter
    dwt_writetodevice(PMSC_ID,0x28,2,buf);

    // Test LEDs
		if(test==2)
    {
        buf[0] = 0x0f; // Fire a LED blink trigger
        dwt_writetodevice(PMSC_ID,0x2a,1,buf);
        buf[0] = 0x00; // Clear forced trigger bits
        dwt_writetodevice(PMSC_ID,0x2a,1,buf);
    }
	}
	else
	{
		// Clear the GPIO bits that are used for LED control
		dwt_readfromdevice(GPIO_CTRL_ID,0x00,2,buf);
		buf[1] &= ~(0x15); // Set GPIO3, 2, 1
		buf[0] &= ~(0x40); // Set GPIO0
		dwt_writetodevice(GPIO_CTRL_ID,0x00,2,buf);
	}

} // end _dwt_enableleds()

// -------------------------------------------------------------------------------------------------------------------

#define D_PLL_LOCK (0x1)
#define D_PLL_LOCK_WARN (0x6)

//NOTE:
//configure PLL1/CLK PLL block - it will auto cal - this may take up to 150us (5us per cal step)
//while the PLL is calibrating any SPI access longer than a byte might be corrupted
//best is to wait for an interrupt to let us know when it is finished.

int _dwt_checkclkplllock(void)
{
    int lock = 0;
    int k = 50 ;
    uint8 rd_buf = 0;

    while((lock == 0) && (k-->0))
    {
        dwt_readfromdevice(RF_CFG_STS_ID,0x2c,1,&rd_buf);
        if(((rd_buf & D_PLL_LOCK_WARN) == 0) //PLL needs re-cal - if hi/lo bits set - no lock
                && (rd_buf & D_PLL_LOCK)) //PLL locked
        {
            lock = 1;
            break;
        }
    }
    return lock;
}

// -------------------------------------------------------------------------------------------------------------------
// function to enable/disable clocks to particular digital blocks/system
//

void _dwt_enableclocks(int clocks)
{
    uint8 reg[2];

    dwt_readfromdevice(PMSC_ID, 0x0, 2, reg);
    switch(clocks)
    {
        case ENABLE_ALL_SEQ:
        {
            reg[0] = 0x00 ;
            reg[1] = reg[1] & 0xfe;
        }
        break;
        case FORCE_SYS_XTI:
        {
            //system and rx
            reg[0] = 0x01 | (reg[0] & 0xfc);
        }
        break;
        case FORCE_SYS_PLL:
        {
            //system
            reg[0] = 0x02 | (reg[0] & 0xfc);
        }
        break;
        case READ_LDE_ON:
        {
            //LDE
            reg[0] = 0x01 | (reg[0] & 0xfc);
            reg[1] = 0x01 | reg[1];
        }
        break;
        case READ_LDE_OFF:
        {
            //LDE
            reg[0] = reg[0] & 0xfc;
            reg[1] = reg[1] & 0xfe;
        }
        break;
        case READ_ACC_ON:
        {
            reg[0] = 0x48 | (reg[0] & 0xb3);
            reg[1] = 0x80 | reg[1];
        }
        break;
        case READ_ACC_OFF:
        {
            reg[0] = reg[0] & 0xb3;
            reg[1] = 0x7f & reg[1];

        }
        break;
        case FORCE_NVM_ON:
        {
            reg[1] = 0x02 | reg[1];
        }
        break;
        case FORCE_NVM_OFF:
        {
            reg[1] = reg[1] & 0xfd;
        }
        break;
        case FORCE_LDE_ON:
        {
            //LDE
            reg[1] = 0x01 | reg[1];
        }
        break;
        case FORCE_LDE_OFF:
        {
            //LDE
            reg[1] = 0xFE & reg[1];
        }
        break;
        case FORCE_TX_PLL:
        {
            reg[0] = 0x20| (reg[0] & 0xcf);
        }
        default:
        break;
    }


    // Need to write lower byte separately before setting the higher byte(s)
    dwt_writetodevice(PMSC_ID, 0x0, 1, &reg[0]);
    dwt_writetodevice(PMSC_ID, 0x1, 1, &reg[1]);

} // end _dwt_enableclocks()


void _dwt_disablesequencing(void) //disable sequencing and go to state "INIT"
{
    _dwt_enableclocks(FORCE_SYS_XTI); //set system clock to XTI

    dwt_write16bitoffsetreg(PMSC_ID, 0x4, 0x0); //disable PMSC ctrl of RF and RX clk blocks
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_setdelayedtrxtime()
 *
 * Description:  This API function configures the delayed transmit time or the delayed rx on time
 *
 * input parameters
 * @param starttime - the tx/rx start time (the 32 bits should be the high 32 bits of the system time at which to send the message,
 * or at which to turn on the receiver)
 *
 * output parameters none
 *
 * no return value
 */
void dwt_setdelayedtrxtime(uint32 starttime)
{
    dwt_write32bitoffsetreg(DX_TIME_ID, 1, starttime) ;

} // end dwt_setdelayedtrxtime()

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_starttx()
 *
 *  Description: This call initiates the transmission, input parameter indicates which TX mode is used see below
 *
 * input parameters:
 * @param mode - if 0 immediate TX (no response expected)
 *               if 1 delayed TX (no response expected)
 *               if 2 immediate TX (response expected - so the receiver sill be automatically turned on after TX is done)
 *               if 3 delayed TX (response expected - so the receiver sill be automatically turned on after TX is done)
 *
 * output parameters
 *
 * returns DWT_DECA_SUCCESS for success, or DWT_DECA_ERROR for error (e.g. a delayed transmission will fail if the delayed time has passed)
 */
int dwt_starttx(uint8 mode)
{
    int retval = DWT_SUCCESS ;
    uint8 temp  = 0x0;

    if(mode & DWT_RESPONSE_EXPECTED)
    {
        temp = 0x80 ; //set wait4response bit
    }

    if (mode & DWT_START_TX_DELAYED)
    {
        uint32 status ;

        // both SYS_CTRL_TXSTRT and SYS_CTRL_TXDLYS to correctly enable TX
        temp |= SYS_CTRL_TXDLYS | SYS_CTRL_TXSTRT ;
        dwt_writetodevice(SYS_CTRL_ID,0,1,&temp) ;

        status = dwt_read32bitreg(SY_STAT_ID) ;          // read status register
        if ((status & (SY_STAT_DSHP /*| SY_STAT_TXPRERR*/)) == 0)                               // Transmit Delayed Send set over Half a Period away.
        {
            //printf("tx delayed \n");
            retval = DWT_SUCCESS ;                                            // All okay
        }
        else
        {
            // I am taking DSHP set to Indicate that the TXDLYS was set too late for the specified DX_TIME.
            // Remedial Action - (a) cancel delayed send
            temp = SYS_CTRL_TRXOFF ;                                   // this assumes the bit is in the lowest byte
            dwt_writetodevice(SYS_CTRL_ID,0,1,&temp) ;
            // note event Delayed TX Time too Late
            // could fall through to start a normal send (below) just sending late.....
            // ... instead return and assume return value of 1 will be used to detect and recover from the issue.

            // clear the "auto TX to sleep" bit
            dwt_entersleepaftertx(0);

            retval = DWT_ERROR ;                                            // Failed !

        }
    }
    else
    {
        temp |= SYS_CTRL_TXSTRT ;
        dwt_writetodevice(SYS_CTRL_ID,0,1,&temp) ;
    }

    return retval;

} // end dwt_starttx()


// -------------------------------------------------------------------------------------------------------------------
//
// Force Transceiver OFF
//
void dwt_forcetrxoff(void)
{
    uint8 temp ;

    temp = SYS_CTRL_TRXOFF ;                       // this assumes the bit is in the lowest byte
    dwt_writetodevice(SYS_CTRL_ID,0,1,&temp) ;

    // Forcing Transceiver off - so we do not want to see any new events that may have happened
    dwt_write32bitreg(SY_STAT_ID,(CLEAR_ALLTX_EVENTS | CLEAR_ALLRXERROR_EVENTS | CLEAR_ALLRXGOOD_EVENTS)) ;

    dwt_syncrxbufptrs();

    //test code
    temp = SYS_CTRL_TRXOFF ;                       // this assumes the bit is in the lowest byte
    dwt_writetodevice(SYS_CTRL_ID,0,1,&temp) ;

} // end deviceforcetrxoff()

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_syncrxbufptrs()
 *
 *  Description: this function synchronizes rx buffer pointers
 *  need to make sure that the host/IC buffer pointers are aligned before starting RX
 *
 * input parameters:
 *
 * output parameters
 *
 * no return value
 */
void dwt_syncrxbufptrs(void)
{
	uint8  buff ;
	//need to make sure that the host/IC buffer pointers are aligned before starting RX
	dwt_readfromdevice(SY_STAT_ID, 3, 1, &buff);

	if((buff & 0x80) !=  //IC side Receive Buffer Pointer
			((buff & 0x40) << 1) ) //Host side Receive Buffer Pointer
	{
		uint8 hsrb = 1;
		dwt_writetodevice(SYS_CTRL_ID, 3, 1, &hsrb) ;       // we need to swap rx buffer status reg (write one to toggle internally)
	}
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_setrxmode()
 *
 *  Description: enable different rx modes, e.g.:
 *   a) "snooze" mode, the receiver only listens periodically for preamble
 *   b) the rx PPDM "sniff" mode - receiver cycles through ON/OFF periods
 *
 * input parameters:
 * @param mode      - RX_NORMAL = 0x0
 *                    RX_SNIFF  = 0x1      enable the rx PPDM "sniff" mode
 * @param rxON      - SNIFF mode ON period in PACs
 * @param rxOFF     - SNIFF mode OFF period in us (actually in 1.0256 micro second intervals)
 * output parameters
 *
 * no return value
 */
void dwt_setrxmode(int mode, uint8 rxON, uint8 rxOFF)
{
    uint16 reg16 =  ((rxOFF << 8) | rxON);

    if(mode & DWT_RX_SNIFF)
    {
        // PPM_OFF 15:8, PPM_ON 3:0
        dwt_write16bitoffsetreg(PPDM_ID, 0x00, reg16) ; //enable
    }
    else
    {
        dwt_write16bitoffsetreg(PPDM_ID, 0x00, 0x0000) ; //disable
    }

}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_rxenable()
 *
 *  Description: This call turns on the receiver, can be immediate or delayed.
 *  The receiver will stay turned on, listening to any messages until
 *  it either receives a good frame, an error (CRC, PHY header, Reed Solomon) or  it times out (SFD, Preamble or Frame).
 *
 * input parameters
 * @param delayed - TRUE the receiver is turned on after some delay (as programmed with dwt_setdelayedtime())
 *
 * output parameters
 *
 * returns DWT_DECA_SUCCESS for success, or DWT_DECA_ERROR for error (e.g. a delayed receive enable will be too far in the future if delayed time has passed (if delayed time is > 8s from now))
 */
int dwt_rxenable(int delayed)
{
    uint16 temp ;

    dwt_syncrxbufptrs();

    temp = SYS_CTRL_RXENAB ;

    if (delayed)
    {
        temp |= SYS_CTRL_RXDLYE ;
    }

    dwt_write16bitoffsetreg(SYS_CTRL_ID,0,temp) ;

    if (delayed) //check for errors
    {
        uint32 status1 = dwt_read32bitreg(SY_STAT_ID) ;          // read status register

        if (status1 & (SY_STAT_DSHP)) //if delay has not passed do delayed else immediate RX on
        {
            dwt_forcetrxoff(); //turn the delayed receive off, and do immediate receive, return warning indication
            temp = SYS_CTRL_RXENAB ; //clear the delay bit
            dwt_write16bitoffsetreg(SYS_CTRL_ID,0,temp) ;
            return DWT_ERROR;
        }
    }

    return DWT_SUCCESS;
} // end dwt_rxenable()

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_setrxtimeout()
 *
 *  Description: This call enables RX timeout (SY_STAT_RFTO event)
 *
 * input parameters
 * @param time - how long the receiver remains on from the RX enable command
 *               The time parameter used here is in 1.012 us (128/499.2MHz) units
 *               If set to 0 the timeout is disabled.
 *
 * output parameters
 *
 * no return value
 */
void dwt_setrxtimeout(uint16 time)
{
    uint8 temp ;

    dwt_readfromdevice(SYS_CFG_ID,3,1,&temp) ;           // read register

    if(time > 0)
    {
        dwt_write16bitoffsetreg(RX_FWTO_ID, 0x0, time) ;

        temp |= SYS_CFG_RXWTOE_SB;
        // OR in 32bit value (1 bit set), I know this is in high byte.
        dw1000local.sysCFGreg |= (SYS_CFG_RXWTOE_SB << 24);

        dwt_writetodevice(SYS_CFG_ID,3,1,&temp) ;
    }
    else
    {
        temp &= (~SYS_CFG_RXWTOE_SB);
        // AND in inverted 32bit value (1 bit clear), I know this is in high byte.
        dw1000local.sysCFGreg &= ~(SYS_CFG_RXWTOE_SB << 24);

        dwt_writetodevice(SYS_CFG_ID,3,1,&temp) ;

        //dwt_write16bitoffsetreg(RX_FWTO_ID,0,0) ;          // clearing the time is not needed
    }

} // end dwt_setrxtimeout()


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_setpreambledetecttimeout()
 *
 *  Description: This call enables preamble timeout (SY_STAT_RXPTO event)
 *
 * input parameters
 * @param  timeout - in PACs
 *
 * output parameters
 *
 * no return value
 */
void dwt_setpreambledetecttimeout(uint16 timeout)
{
    dwt_write16bitoffsetreg(DRX_CFG_STS_ID, RX_DTUNE4_SUB_ADDR, timeout);
}


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: decamutexon()
 *
 * Description: This function should disable interrupts. This is called at the start of a critical section
 * It returns the irq state before disable, this value is used to re-enable in decamutexoff call
 *
 * Note: The body of this function is defined in deca_mutex.c and is platform specific
 *
 * input parameters:	
 *
 * output parameters
 *
 * returns the state of the DW1000 interrupt
 */
decaIrqStatus_t decamutexon(void)           
{
	decaIrqStatus_t s = EXTI_GetITEnStatus(DECAIRQ_EXTI_IRQn);

	if(s) {
		//port_DisableEXT_IRQ(); //disable the external interrupt line
		NVIC_DisableIRQ(DECAIRQ_EXTI_IRQn);
	}
	return s ;   // return state before disable, value is used to re-enable in decamutexoff call
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: decamutexoff()
 *
 * Description: This function should re-enable interrupts, or at least restore their state as returned(&saved) by decamutexon 
 * This is called at the end of a critical section
 *
 * Note: The body of this function is defined in deca_mutex.c and is platform specific
 *
 * input parameters:	
 * @param s - the state of the DW1000 interrupt as returned by decamutexon
 *
 * output parameters
 *
 * returns the state of the DW1000 interrupt
 */
void decamutexoff(decaIrqStatus_t s)        // put a function here that re-enables the interrupt at the end of the critical section
{
	if(s) { //need to check the port state as we can't use level sensitive interrupt on the STM ARM
		//port_EnableEXT_IRQ();
		NVIC_EnableIRQ(DECAIRQ_EXTI_IRQn);
	}
}



/*! ------------------------------------------------------------------------------------------------------------------
 * Function: void dwt_setinterrupt( uint32 bitmask, uint8 enable)
 *
 * Description: This function enables the specified events to trigger an interrupt.
 * The following events can be enabled:
 * DWT_INT_TFRS         0x00000080          // frame sent
 * DWT_INT_RFCG         0x00004000          // frame received with good CRC
 * DWT_INT_RPHE         0x00001000          // receiver PHY header error
 * DWT_INT_RFCE         0x00008000          // receiver CRC error
 * DWT_INT_RFSL         0x00010000          // receiver sync loss error
 * DWT_INT_RFTO         0x00020000          // frame wait timeout
 * DWT_INT_RXPTO        0x00200000          // preamble detect timeout
 * DWT_INT_SFDT         0x04000000          // SFD timeout
 * DWT_INT_ARFE         0x20000000          // frame rejected (due to frame filtering configuration)
 *
 *
 * input parameters:
 * @param bitmask - sets the events which will generate interrupt
 * @param enable - if set the interrupts are enabled else they are cleared
 *
 * output parameters
 *
 * no return value
 */
void dwt_setinterrupt(uint32 bitmask, uint8 enable)
{
    decaIrqStatus_t stat ;
    uint32 mask ;

    // need to beware of interrupts occurring in the middle of following read modify write cycle
    stat = decamutexon() ;

    mask = dwt_read32bitreg(SY_MASK_ID) ;           // read register

    if(enable)
        mask |= bitmask ;
    else
        mask &= ~bitmask ;  // clear the bit

#ifdef _MSC_VER
    //if using PC/Cheetah - Interrupts are not supported...
#else
    dwt_write32bitreg(SY_MASK_ID,mask) ;            // new value
#endif

    decamutexoff(stat) ;
}


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_configeventcounters()
 *
 *  Description: This is used to enable/disable the event counter in the IC
 *
 * input parameters
 * @param - enable - 1 enables (and reset), 0 disables the event counters
 * output parameters
 *
 * no return value
 */
void dwt_configeventcounters(int enable)
{
    uint8 temp = 0x0;  //disable
    //need to clear and disable, can't just clear
    temp = 0x2; //clear and disable
    dwt_writetodevice(EVENT_CTRL_ID, 0x0, 1, &temp) ;

    if(enable)
    {
        temp = 0x1; //enable
        dwt_writetodevice(EVENT_CTRL_ID, 0x0, 1, &temp) ;
    }
}


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_readeventcounters()
 *
 *  Description: This is used to read the event counters in the IC
 *
 * input parameters
 * @param counters - pointer to the dwt_deviceentcnts_t structure which will hold the read data
 *
 * output parameters
 *
 * no return value
 */
void dwt_readeventcounters(dwt_deviceentcnts_t *counters)
{
    uint32 temp;

    temp= dwt_read32bitoffsetreg(EVENT_CTRL_ID, EVENT_COUNT0); //read sync loss (31-16), PHE (15-0)
    counters->PHE = temp & 0xFFF;
    counters->RSL = (temp >> 16) & 0xFFF;

    temp = dwt_read32bitoffsetreg(EVENT_CTRL_ID, EVENT_COUNT1); //read CRC bad (31-16), CRC good (15-0)
    counters->CRCG = temp & 0xFFF;
    counters->CRCB = (temp >> 16) & 0xFFF;

    temp = dwt_read32bitoffsetreg(EVENT_CTRL_ID, EVENT_COUNT2); //overruns (31-16), address errors (15-0)
    counters->ARFE = temp & 0xFFF;
    counters->OVER = (temp >> 16) & 0xFFF;

    temp = dwt_read32bitoffsetreg(EVENT_CTRL_ID, EVENT_COUNT3); //read PTO (31-16), SFDTO (15-0)
    counters->PTO = (temp >> 16) & 0xFFF;
    counters->SFDTO = temp & 0xFFF;

    temp = dwt_read32bitoffsetreg(EVENT_CTRL_ID, EVENT_COUNT4); //read RX TO (31-16), TXFRAME (15-0)
    counters->TXF = (temp >> 16) & 0xFFF;
    counters->RTO = temp & 0xFFF;

    temp = dwt_read32bitoffsetreg(EVENT_CTRL_ID, EVENT_COUNT5); //read half period warning events
    counters->HPW = temp & 0xFFF;
    counters->TXW = (temp >> 16) & 0xFFF;                       //power up warning events
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn dwt_rxreset()
 *
 *  @brief this function resets the receiver of the DW1000
 *
 * input parameters:
 *
 * output parameters
 *
 * no return value
 */
void dwt_rxreset(void)
{
	uint8 resetrx = 0xe0;
	//set rx reset (writing 
	dwt_writetodevice(PMSC_ID, 0x3, 1, &resetrx);

	resetrx = 0xf0; //clear RX reset
	dwt_writetodevice(PMSC_ID, 0x3, 1, &resetrx);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_softreset()
 *
 *  Description: this function resets the DW1000
 *
 * input parameters:
 *
 * output parameters
 *
 * no return value
 */
void dwt_softreset(void)
{
    uint8 temp[1] = {0};

    _dwt_disablesequencing();
    //_dwt_enableclocks(FORCE_SYS_XTI); //set system clock to XTI

    //clear any AON auto download bits (as reset will trigger AON download)
    dwt_write16bitoffsetreg(AON_ID,0x00,0x0);
    //clear the wakeup configuration
    dwt_writetodevice(AON_ID,0x06,1,temp);
    //upload the new configuration
    _dwt_aonarrayupload();

    //reset HIF, TX, RX and PMSC
    dwt_readfromdevice(PMSC_ID, 0x3, 1, temp) ;

    temp[0] &= 0x0F;
    dwt_writetodevice(PMSC_ID, 0x3, 1, &temp[0]) ;

    //DW1000 needs a 10us sleep to let clk PLL lock after reset - the PLL will automatically lock after the reset
    Delay_ms(1);
    //Can also poll the PLL lock flag, but then the SPI needs to be < 3MHz !!

    temp[0] |= 0xF0;
    dwt_writetodevice(PMSC_ID, 0x3, 1, &temp[0]) ;

}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_xtaltrim()
 *
 *  Description: This is used adjust the crystal frequency
 *
 * input parameters:
 * @param   value - crystal trim value (in range 0x0 to 0x1F) 31 steps (~1.5ppm per step)
 *
 * output parameters
 *
 * no return value
 */
void dwt_xtaltrim(uint8 value)
{
    uint8 write_buf;

    dwt_readfromdevice(SYNTH_CAL_ID,0xE,1,&write_buf);

    write_buf &= 0xE0 ;

    write_buf |= value ;

    dwt_writetodevice(SYNTH_CAL_ID,0xE,1,&write_buf);
}


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_configcwmode()
 *
 *  Description: this function sets the DW1000 to transmit cw signal at specific channel frequency
 *
 * input parameters:
 * @param chan - specifies the operating channel (e.g. 1, 2, 3, 4, 5, 6 or 7)
 *
 * output parameters
 *
 * returns DWT_DECA_SUCCESS for success, or DWT_DECA_ERROR for error
 */
int dwt_configcwmode(uint8 chan)
{
    uint8 write_buf[1];
#ifdef DWT_API_ERROR_CHECK
    if ((chan < 1) || (chan > 7) || (6 == chan)) return DWT_ERROR ; // validate channel number parameter
#endif

    write_buf[0] = 0x13;

    //configure CW mode
    dwt_writetodevice(TX_CAL_ID, 0x0c, 1, write_buf);

    //
    //  disable TX/RX RF block sequencing (needed for cw frame mode)
    //
    _dwt_disablesequencing();

    //config RF pll (for a given channel)
    //configure PLL2/RF PLL block CFG
    dwt_writetodevice(SYNTH_CAL_ID, 0x07, 5, &pll2_config[chan_idx[chan]][0]);
    //configure PLL2/RF PLL block CAL
    dwt_writetodevice(SYNTH_CAL_ID, 0x0e, 1, &pll2calcfg);
    //PLL wont be enabled until a TX/RX enable is issued later on
    // Configure RF TX blocks (for specified channel and prf)
    //Config RF TX control
    dwt_write32bitoffsetreg(RF_CFG_STS_ID, RF_CTRLTX_SUB_ADDR, tx_config[chan_idx[chan]]);


    //
    // enable RF PLL
    //
    dwt_write32bitreg(RF_CFG_STS_ID, 0x01FE000);
    dwt_write32bitreg(RF_CFG_STS_ID, 0x05FFF00);

    //
    //  configure TX clocks
    //
    write_buf[0] = 0x22;
    dwt_writetodevice(PMSC_ID,0x0,1,write_buf);
    write_buf[0] = 0x07;
    dwt_writetodevice(PMSC_ID,0x1,1,write_buf);

    //disable fine grain TX seq
    dwt_write16bitoffsetreg(PMSC_ID,0x26,0);

    return DWT_SUCCESS ;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_configcontinuousframemode()
 *
 *  Description: this function sets the DW1000 to continuous tx frame mode
 *
 * input parameters:
 * @param framerepetitionrate - specifies the
 *
 * output parameters
 *
 * no return value
 */
void dwt_configcontinuousframemode(uint32 framerepetitionrate)
{
    uint8 write_buf[4];
    //
    //  disable TX/RX RF block sequencing (needed for continuous frame mode)
    //

    _dwt_disablesequencing();

    // enable RF PLL and TX blocks
    //
    dwt_write32bitreg(RF_CFG_STS_ID, 0x01FE000);
    dwt_write32bitreg(RF_CFG_STS_ID, 0x05FFF00);

    //
    //  configure TX clocks
    //
    _dwt_enableclocks(FORCE_SYS_PLL);
    _dwt_enableclocks(FORCE_TX_PLL);

    //set the frame repetition rate
    if(framerepetitionrate < 4)
        framerepetitionrate = 4;

    dwt_write32bitoffsetreg(DX_TIME_ID, 0, framerepetitionrate) ;
    //
    //  configure continuous frame TX
    //
    write_buf[0] = 0x10;
    dwt_writetodevice(EVENT_CTRL_ID,0x24,1,write_buf); //turn the tx power spectrum test mode - continuous sending of frames
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_readtempvbat()
 *
 *  Description: this function reads the battery voltage and temperature of the MP
 *  The values read here will be the current values sampled by DW1000 AtoD converters.
 *  Note: SPI must be up to 3MHz
 *  Note on Temperature: the temperature value needs to be converted to give the real temperature
 *  the formula is: 1.13 * reading - 113.0
 *  Note on Voltage: the voltage value needs to be converted to give the real voltage
 *  the formula is: 0.0057 * reading + 2.3
 *
 * input parameters:
 *
 * output parameters
 *
 * returns  (temp_raw<<8)|(vbat_raw)
 */
uint16 dwt_readtempvbat(void)
{
    uint8 wr_buf[1];
    uint8 vbat_raw;
    uint8 temp_raw;

    //these writes should be single writes and in sequence
    wr_buf[0] = 0x80; // Enable TLD Bias
    dwt_writetodevice(RF_CFG_STS_ID,0x11,1,wr_buf);

    wr_buf[0] = 0x0A; // Enable TLD Bias and ADC Bias
    dwt_writetodevice(RF_CFG_STS_ID,0x12,1,wr_buf);

    wr_buf[0] = 0x0f; // Enable Outputs (only after Biases are up and running)
    dwt_writetodevice(RF_CFG_STS_ID,0x12,1,wr_buf);


    // Reading All SAR inputs
    wr_buf[0] = 0x00; // Clear TX LVL read
    dwt_writetodevice(TX_CAL_ID,0x01,1,wr_buf);
    wr_buf[0] = 0x00; //
    dwt_writetodevice(TX_CAL_ID,0x00,1,wr_buf);
    wr_buf[0] = 0x01; // Set SAR enable
    dwt_writetodevice(TX_CAL_ID,0x00,1,wr_buf);

    // Bug in Register read in TXCAL block. Do one byte at a time
    dwt_readfromdevice(TX_CAL_ID,0x03,1,&vbat_raw);
    dwt_readfromdevice(TX_CAL_ID,0x04,1,&temp_raw);

    wr_buf[0] = 0x00; // Clear SAR enable
    dwt_writetodevice(TX_CAL_ID,0x00,1,wr_buf);

    return ((temp_raw<<8)|(vbat_raw));
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_readwakeuptemp()
 *
 *  Description: this function reads the temperature of the DW1000 that was sampled
 *  on waking from Sleep/Deepsleep. They are not current values, but read on last
 *  wakeup if DWT_TANDV bit is set in mode parameter of dwt_configuresleep
 *
 * input parameters:
 *
 * output parameters:
 *
 * returns: 8-bit raw temperature sensor value
 */

uint8 dwt_readwakeuptemp(void)
{
    uint8 temp_raw;
    dwt_readfromdevice(TX_CAL_ID,0x04,1,&temp_raw);
    return (temp_raw);
}


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: dwt_readwakeupvbat()
 *
 *  Description: this function reads the battery voltage of the DW1000 that was sampled
 *  on waking from Sleep/Deepsleep. They are not current values, but read on last
 *  wakeup if DWT_TANDV bit is set in mode parameter of dwt_configuresleep
 *
 * input parameters:
 *
 * output parameters:
 *
 * returns: 8-bit raw battery voltage sensor value
 */

uint8 dwt_readwakeupvbat(void)
{
    uint8 vbat_raw;
    dwt_readfromdevice(TX_CAL_ID,0x03,1,&vbat_raw);
    return (vbat_raw);
}


/*! ------------------------------------------------------------------------------------------------------------------
*/

#if (REG_DUMP == 1)
void dwt_dumpregisters(char *str, size_t strSize)
{
    uint32 reg = 0;
    uint8 buff[5];
    int i;
    int cnt ;

    //first print all single registers
    for(i=0; i<0x3F; i++)
    {
        dwt_readfromdevice(i, 0, 5, buff) ;
        str += cnt = sprintf_s(str,strSize,"reg[%02X]=%02X%02X%02X%02X%02X",i,buff[4], buff[3], buff[2], buff[1], buff[0] ) ;
        strSize -= cnt ;
        str += cnt = sprintf_s(str,strSize,"\n") ;
        strSize -= cnt ;
    }

    //reg 0x20
    for(i=0; i<=32; i+=4)
    {
        reg = dwt_read32bitoffsetreg(0x20,i) ;
        str += cnt = sprintf_s(str,strSize,"reg[%02X:%02X]=%08X",0x20,i,reg) ;
        strSize -= cnt ;
        str += cnt = sprintf_s(str,strSize,"\n") ;
        strSize -= cnt ;
    }

    //reg 0x21
    for(i=0; i<=44; i+=4)
    {
        reg = dwt_read32bitoffsetreg(0x21,i) ;
        str += cnt = sprintf_s(str,strSize,"reg[%02X:%02X]=%08X",0x21,i,reg) ;
        strSize -= cnt ;
        str += cnt = sprintf_s(str,strSize,"\n") ;
        strSize -= cnt ;
    }

    //reg 0x23
    for(i=0; i<=0x20; i+=4)
    {
        reg = dwt_read32bitoffsetreg(0x23,i) ;
        str += cnt = sprintf_s(str,strSize,"reg[%02X:%02X]=%08X",0x23,i,reg) ;
        strSize -= cnt ;
        str += cnt = sprintf_s(str,strSize,"\n") ;
        strSize -= cnt ;
    }

    //reg 0x24
    for(i=0; i<=12; i+=4)
    {
        reg = dwt_read32bitoffsetreg(0x24,i) ;
        str += cnt = sprintf_s(str,strSize,"reg[%02X:%02X]=%08X",0x24,i,reg) ;
        strSize -= cnt ;
        str += cnt = sprintf_s(str,strSize,"\n") ;
        strSize -= cnt ;
    }

    //reg 0x27
    for(i=0; i<=44; i+=4)
    {
        reg = dwt_read32bitoffsetreg(0x27,i) ;
        str += cnt = sprintf_s(str,strSize,"reg[%02X:%02X]=%08X",0x27,i,reg) ;
        strSize -= cnt ;
        str += cnt = sprintf_s(str,strSize,"\n") ;
        strSize -= cnt ;
    }

    //reg 0x28
    for(i=0; i<=64; i+=4)
    {
        reg = dwt_read32bitoffsetreg(0x28,i) ;
        str += cnt = sprintf_s(str,strSize,"reg[%02X:%02X]=%08X",0x28,i,reg) ;
        strSize -= cnt ;
        str += cnt = sprintf_s(str,strSize,"\n") ;
        strSize -= cnt ;
    }

    //reg 0x2A
    for(i=0; i<20; i+=4)
    {
        reg = dwt_read32bitoffsetreg(0x2A,i) ;
        str += cnt = sprintf_s(str,strSize,"reg[%02X:%02X]=%08X",0x2A,i,reg) ;
        strSize -= cnt ;
        str += cnt = sprintf_s(str,strSize,"\n") ;
        strSize -= cnt ;
    }

    //reg 0x2B
    for(i=0; i<24; i+=4)
    {
        reg = dwt_read32bitoffsetreg(0x2B,i) ;
        str += cnt = sprintf_s(str,strSize,"reg[%02X:%02X]=%08X",0x2B,i,reg) ;
        strSize -= cnt ;
        str += cnt = sprintf_s(str,strSize,"\n") ;
        strSize -= cnt ;
    }

    //reg 0x2f
    for(i=0; i<40; i+=4)
    {
        reg = dwt_read32bitoffsetreg(0x2f,i) ;
        str += cnt = sprintf_s(str,strSize,"reg[%02X:%02X]=%08X",0x2f,i,reg) ;
        strSize -= cnt ;
        str += cnt = sprintf_s(str,strSize,"\n") ;
        strSize -= cnt ;
    }

    //reg 0x31
    for(i=0; i<84; i+=4)
    {
        reg = dwt_read32bitoffsetreg(0x31,i) ;
        str += cnt = sprintf_s(str,strSize,"reg[%02X:%02X]=%08X",0x31,i,reg) ;
        strSize -= cnt ;
        str += cnt = sprintf_s(str,strSize,"\n") ;
        strSize -= cnt ;
    }

    //reg 0x36 = PMSC_ID
    for(i=0; i<=48; i+=4)
    {
        reg = dwt_read32bitoffsetreg(PMSC_ID,i) ;
        str += cnt = sprintf_s(str,strSize,"reg[%02X:%02X]=%08X",PMSC_ID,i,reg) ;
        strSize -= cnt ;
        str += cnt = sprintf_s(str,strSize,"\n") ;
        strSize -= cnt ;
    }
}
#endif

/* ===============================================================================================
   List of expected (known) device ID handled by this software
   ===============================================================================================

    0xDECA0130                               // DW1000 - MP

   ===============================================================================================
*/

