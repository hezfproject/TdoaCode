// -------------------------------------------------------------------------------------------------------------------
//
//  File: deca_regs.h - Decawave Device Register Definitions
//
//  Copyright 2008 (c) DecaWave Ltd, Dublin, Ireland.
//
//  All rights reserved.
//
//  Author: Billy Verso, November 2008
//
// -------------------------------------------------------------------------------------------------------------------


#ifndef _DECA_REGS_H_
#define _DECA_REGS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define DEV_ID_ID               0x00            // DEV_ID - Device ID register, includes revision info (0xDECA0130)
#define EUI_64_ID               0x01            // EUI_64 - IEEE Extended Unique Identifier (63:0)
#define PANADR_ID               0x03            // PANADR - PAN ID (31:16) and Short Address (15:0)

#define SYS_CFG_ID              0x04            // SYS_CFG - System Configuration (31:0)

#define SYS_TIME_L_ID           0x06            // SYS_TIME_L - 40-bit System Timer (63.8976 GHz) - 40 bits
#define SYS_TIME_LEN            5               // 40-bits ~= 5 bytes

#define TX_FCTRL_ID             0x08            // TX_FCTRL - TX Frame Control
#define TX_BUFFER_ID            0x09            // TX_BUFFER - Transmit Data Buffer

#define DX_TIME_ID              0x0A            // DX_TIME - 40-bit Delayed Send or Receive Time - 40 bits

#define RX_FWTO_ID              0x0C            // RX_FWTO - Receive Frame Wait Timeout Period
#define SYS_CTRL_ID             0x0D            // SYS_CTRL - System Control Register

#define SY_MASK_ID              0x0E            // SY_MASK - System event Mask_Register
#define SY_STAT_ID              0x0F            // SY_STAT - System event Status Register

#define RX_FINFO_ID             0x10            // RX_FINFO - RX Frame Information
#define RX_BUFFER_ID            0x11            // RX_BUFFER - Receive Data Buffer
#define RX_FQUAL_ID             0x12            // RX_FQUAL - RX status information indicating Frame Quality

#define RX_TIME_ID              0x15            // RX_TIME - 40-bit Time of Receiving
#define RX_TIME_LEN             5               // read only 5 bytes (the adjusted timestamp (40:0))

#define TX_TIME_ID              0x17            // TX_TIME - 40-bit Time of Sending
#define TX_TIME_LEN             5               // 40-bits = 5 bytes

#define TX_ANTD                 0x18            // 32bit delay from antenna to transmitter

#define SY_STATES_ID            0x19            // DBG - SYSTEM states

#define ACK_RESP_ID             0x1A            // Acknowledge (31:24 preamble symbol delay before auto ACK is sent) and respose (19:0 - unit 1us) timer
#define PPDM_ID                 0x1D            // Preamble Pulse Detection Mode

#define TX_POWER_ID             0x1E            // TX_POWER - TX Power Control

#define CHAN_CTRL_ID            0x1F            // CHAN_CTRL - Channel Control

#define USR_SFD_ID              0x21            // User-specified short/long TX/RX SFD sequences

#define AGC_CFG_STS_ID          0x23            // AGC configuration and status
#define EXT_SYNC_ID             0x24            // External clock synchronisation

#define RX_ACCD_ID              0x25            // RX_ACCD - Receive Accumulator for Debug

#define GPIO_CTRL_ID            0x26            // GPIO configuration and status
#define DRX_CFG_STS_ID          0x27            // Digital configuration and status registers
#define RF_CFG_STS_ID           0x28            // RF  configuration and status
#define TX_CAL_ID               0x2A            // TX RF calibration and status
#define SYNTH_CAL_ID            0x2B            // SYNTH RF calibration and status
#define AON_ID                  0x2C            // Always-On register configuration
#define NVRAM_IF_ID             0x2D            // Non-volatile memory

#define LDE_CFG_STS_ID          0x2E            // Leading edge configuration and status
#define EVENT_CTRL_ID           0x2F            // Event counters

#define PMSC_ID                 0x36            // Power-management clocks sequencing and control registers

#define LDE_LOAD_BIT            ((0x1 << 15))   // force LDE code image to be copied from NVM to LDE memory
#define LDE_CRC_GEN_EN_BIT      ((0x1 << 13))   // enable CRC gen on the LDE code image
#define LDE_READ_BIT            ((0x1 << 14))   // read the CRC ...

//Digital configuration and status registers - subaddresses

#define RX_DTUNE0_SUB_ADDR2     0x02  // Tuning for Rx digital system
#define RX_DTUNE1_SUB_ADDR      0x04  // Tuning for Rx digital system
#define RX_DTUNE1_SUB_ADDR6     0x06  // Tuning for Rx digital system
#define RX_DTUNE2_SUB_ADDR      0x08  // Tuning for Rx digital system
#define RX_DTUNE3_SUB_ADDR      0x20  // Tuning for Rx digital system
#define RX_DTUNE4_SUB_ADDR      0x24  // Tuning for Rx digital system




// Bit definitions for register 0x04 - SYS_CFG - System Configuration
#define SYS_CFG_DIS_FCE         0x00000800      // Ignore FCS
#define SYS_CFG_DIS_DRXB        0x00001000      // Disable Double RX Buffer; 0 - Double rx buffer is enabled, 1 - Double rx buffer is disabled
#define SYS_CFG_F110K           0x00400000      // Receiver: Force into 110K mode, disabling short SFD detection.  (Test mode).
#define SYS_CFG_HSRXBS          0x08000000      // Selects which set (half) of double set of swinging RX buffer + status info, is currently to being presented / accessed in the SY_STAT register
#define SYS_CFG_RXWTOE          0x10000000      // Receive Wait Timeout Enable. Use RX_FWTO downcount as timeout on RX.
#define SYS_CFG_RXWTOE_SB       0x10            // Single Byte - Receive Wait Timeout Enable. Use RX_FWTO downcount as timeout on RX.
#define SYS_CFG_RXAUTR          0x20000000      // Receiver Auto Re-Enable (only rx off and timeout disable the rx - else it stays on)
#define SYS_CFG_AUTACK          0x40000000      // Auto ACK enable
#define SYS_CFG_FF              0x00000001      // Configure frame filtering
#define SYS_CFG_DIS_SMART_TXP   0x40000         // Disable TX power smart gating gain
#define SYS_CFG_LONG_FRAMES     0x00030000      // Set PHR mode bits to 0x11 for long frames


/*masks */
#define SYS_STATUS_MASK_32		0xFFF7FFFFUL	/* System event Status Register access mask (all unused fields should always be writen as zero) */
/*offset 0 */
#define SYS_STATUS_IRQS			0x00000001UL	/* Interrupt Request Status READ ONLY */
#define SYS_STATUS_CPLOCK		0x00000002UL	/* Clock PLL Lock */
#define SYS_STATUS_ESYNCR		0x00000004UL	/* External Sync Clock Reset */
#define SYS_STATUS_AAT			0x00000008UL	/* Automatic Acknowledge Trigger */
#define SYS_STATUS_TXFRB		0x00000010UL	/* Transmit Frame Begins */
#define SYS_STATUS_TXPRS		0x00000020UL	/* Transmit Preamble Sent */
#define SYS_STATUS_TXPHS		0x00000040UL	/* Transmit PHY Header Sent */
#define SYS_STATUS_TXFRS		0x00000080UL	/* Transmit Frame Sent: This is set when the transmitter has completed the sending of a frame */
/*offset 8 */
#define SYS_STATUS_RXPRD		0x00000100UL	/* Receiver Preamble Detected status */
#define SYS_STATUS_RXSFDD		0x00000200UL	/* Receiver Start Frame Delimiter Detected. */
#define SYS_STATUS_LDEDONE		0x00000400UL	/* LDE processing done */
#define SYS_STATUS_RXPHD		0x00000800UL	/* Receiver PHY Header Detect */
#define SYS_STATUS_RXPHE		0x00001000UL	/* Receiver PHY Header Error */
#define SYS_STATUS_RXDFR		0x00002000UL	/* Receiver Data Frame Ready */
#define SYS_STATUS_RXFCG		0x00004000UL	/* Receiver FCS Good */
#define SYS_STATUS_RXFCE		0x00008000UL	/* Receiver FCS Error */
/*offset 16 */
#define SYS_STATUS_RXRFSL		0x00010000UL	/* Receiver Reed Solomon Frame Sync Loss */
#define SYS_STATUS_RXRFTO		0x00020000UL	/* Receive Frame Wait Timeout */
#define SYS_STATUS_LDEERR		0x00040000UL	/* Leading edge detection processing error */
#define SYS_STATUS_reserved		0x00080000UL	/* bit19 reserved */
#define SYS_STATUS_RXOVRR		0x00100000UL	/* Receiver Overrun */
#define SYS_STATUS_RXPTO		0x00200000UL	/* Preamble detection timeout */
#define SYS_STATUS_GPIOIRQ		0x00400000UL	/* GPIO interrupt */
#define SYS_STATUS_SLP2INIT		0x00800000UL	/* SLEEP to INIT */
/*offset 24 */
#define SYS_STATUS_RFPLL_LL		0x01000000UL	/* RF PLL Losing Lock */
#define SYS_STATUS_CLKPLL_LL	0x02000000UL	/* Clock PLL Losing Lock */
#define SYS_STATUS_RXSFDTO		0x04000000UL	/* Receive SFD timeout */
#define SYS_STATUS_HPDWARN		0x08000000UL	/* Half Period Delay Warning */
#define SYS_STATUS_TXBERR		0x10000000UL	/* Transmit Buffer Error */
#define SYS_STATUS_AFFREJ		0x20000000UL	/* Automatic Frame Filtering rejection */
#define SYS_STATUS_HSRBP		0x40000000UL	/* Host Side Receive Buffer Pointer */
#define SYS_STATUS_ICRBP		0x80000000UL	/* IC side Receive Buffer Pointer READ ONLY */
/*offset 32 */
#define SYS_STATUS_RXRSCS		0x0100000000ULL	/* Receiver Reed-Solomon Correction Status */
#define SYS_STATUS_RXPREJ		0x0200000000ULL	/* Receiver Preamble Rejection */
#define SYS_STATUS_TXPUTE		0x0400000000ULL	/* Transmit power up time error */


//frame filtering configuration options
#define FF_ALL_EN               0x1FE           // all frames allowed

// Bit definitions for register 0x08 - TX_FCTRL - TX Frame Control
#define TX_FCTRL_FLEN_SHFT      0               // shift to access Frame Length field
#define TX_FCTRL_FLEN_MASK      0x000003FF      // bit mask to access Frame Length field
#define TX_FCTRL_RATE_SHFT      13              // shift to access Data Rate field
#define TX_FCTRL_RANG_SHFT      15              // shift to access Ranging bit
#define TX_FCTRL_RANG_BIT       (0x1 <<  TX_FCTRL_RANG_SHFT)   // set/clear ranging bit
#define TX_FCTRL_PRFQ_SHFT      16              // shift to access Pulse Repetition Frequency field
#define TX_FCTRL_PSRP_SHFT      18              // shift to access preamble symbol repetitions field
#define TX_FCTRL_PEXT_SHFT      20              // shift to access preamble length Extension to allow specification of non-standard values

// Bit definitions for register 0x0D - SYS_CTRL - System Control Register
#define SYS_CTRL_SFCST          0x00000001      // Suppress Auto-FCS Transmission (on this frame)
#define SYS_CTRL_TXSTRT         0x00000002      // Start Transmitting Now
#define SYS_CTRL_TXDLYS         0x00000004      // Transmitter Delayed Sending (initiates sending when SY_TIME == TXD_TIME
#define SYS_CTRL_NOSFCST        0x00000008      // Cancel CRC suppression
#define SYS_CTRL_TRXOFF         0x00000040      // Force Transciever OFF abort TX or RX immediately
#define SYS_CTRL_RXENAB         0x00000100      // Enable Receiver Now
#define SYS_CTRL_RXDLYE         0x00000200      // Receiver Delayed Enable (Enables Receiver when SY_TIME[0x??] == RXD_TIME[0x??]
#define SYS_CTRL_HSRBTOGGLE     0x01000000      // Host side receiver buffer pointer toggle - toggles 0/1 host side data set pointer

// Bit definitions for register 0x0F - SY_STAT - System event Status Register
#define SY_STAT_TFAA            0x00000008      // Auto ACK requested
#define SY_STAT_TFRS            0x00000080      // Transmit Frame Sent: This is set when the transmitter has completed the sending of a frame
#define SY_STAT_PRED            0x00000100      // Receiver Preamble
#define SY_STAT_SFDD            0x00000200      // Receiver Start Frame Delimiter Detected.
#define SY_STAT_LDED            0x00000400      // LDE done (ucode finished executing)
#define SY_STAT_RHED            0x00000800      // Receiver PHY Header Detect
#define SY_STAT_RPHE            0x00001000      // Receiver PHY Header Error:  Reception completed, Frame Error
#define SY_STAT_RDFR            0x00002000      // Receiver Data Frame Ready:  Reception completed, Frame and Related status info may be read
#define SY_STAT_RFCG            0x00004000      // Receiver FCS Good:  The CRC check has matched the transmitted CRC, frame should be good.
#define SY_STAT_RFCE            0x00008000      // Receiver FCS Error:  The CRC check has not matched the transmitted CRC, frame has some error.
#define SY_STAT_RFSL            0x00010000      // Receiver Frame Sync Loss: The RX lost signal before frame was received, indicates excessive Reed Solomon decoder errors.
#define SY_STAT_RFTO            0x00020000      // Receiver Frame Wait Timeout:  The RX_FWTO time period expired without a Frame RX.
#define SY_STAT_LDEE            0x00040000      // LDE Error
#define SY_STAT_RES             0x00080000      // Reserved
#define SY_STAT_OVRR            0x00100000      // Receiver overrun status
#define SY_STAT_RXPTO           0x00200000      // Receiver Preamble Timeout
#define SY_STAT_SFDT            0x04000000      // SFD Timeout
#define SY_STAT_DSHP            0x08000000      // Transmit Delayed Send set over Half a Period away.  This Indicate that the TXDLYS may be set too late for the specified DX_TIME.
#define SY_STAT_TXBERR          0x10000000      // Transmit Buffer Error
#define SY_STAT_ARFE            0x20000000      // ARFE - frame rejection status
#define SY_STAT_HRBP            0x40000000      // Host Side Receive Buffer Pointer status (selects which data set to read)
#define SY_STAT_RXBP            0x80000000      // Receive Buffer Pointer status - Read ONLY, cannot write 1 to clear

#define CLEAR_ALLTX_EVENTS          (0xF8)          // Clear all TX event flags
#define CLEAR_ALLRXGOOD_EVENTS      (SY_STAT_RDFR | SY_STAT_RFCG | SY_STAT_PRED | SY_STAT_SFDD | SY_STAT_RHED)          // Clear all RX event flags after a packet reception
#define CLEAR_ALLRXERROR_EVENTS     (SY_STAT_RPHE | SY_STAT_RFCE | SY_STAT_RFSL | SY_STAT_SFDT | SY_STAT_RFTO | SY_STAT_RXPTO | SY_STAT_ARFE)           // Clear all RX event flags after an rx error

// Bit definitions for register 0x10 - RX_FINFO - RX Frame Information
#define RX_FINFO_RXFLENLONG_MASK    0x0000003FF      // Receive Frame Length (0 to 1023)
#define RX_FINFO_RXFLEN_MASK    0x0000007F      // Receive Frame Length (0 to 127)
#define RX_FINFO_RXBR_MASK      0x00006000      // Receive Bit Rate: 00 = 110 Kbits/s, 01 =850 Kbits/s, 10 = 6.81 Kbits/s
#define RX_FINFO_RXBR_SHIFT     13
#define RX_FINFO_RR_MASK        0x00008000      // Receiver Ranging = Ranging bit from the received PHY header identifying the frame as a ranging packet.
#define RX_FINFO_RR_SHIFT       15
#define RX_FINFO_RXPRF_MASK     0x00030000      // Receiver Report RX Pulse Repetition Rate: 00= 16 MHz, 01= 64MHz, 10= 4MHz
#define RX_FINFO_RXPRF_SHIFT    16
#define RX_FINFO_RXPSR_MASK     0x000C0000      //  RX Preamble Repetitions, as reported in PHR of received frame.
#define RX_FINFO_RXPSR_SHIFT    18
#define RX_FINFO_RXP2SS_MASK    0xFFF00000      // Preamble to SFD Symbol Count.  symbols between preamble detect and SFD detect triggers.  (Range 0 - 4095)
#define RX_FINFO_RXP2SS_SHIFT   20


// Bit definitions for register 0x1F - CHAN_CTRL - Channel Control Register
#define CHAN_CTRL_TXCHAN_SHIFT  0               // Bits 0..3        TX channel number 0-15 selection
#define CHAN_CTRL_RXCHAN_SHIFT  4               // Bits 4..7        RX channel number 0-15 selection
#define CHAN_CTRL_TXBWDTH_SHIFT 8               // Bits 8..11       TX Bandwidth Selection, default 0000, means full bandwidth
#define CHAN_CTRL_RXBWDTH_SHIFT 12              // Bits 12..15      RX Bandwidth Selection, default 0000, means full bandwidth
#define CHAN_CTRL_DW_SFD_SHIFT  17              // Bit 17           Use DecaWave proprietary SFD code
#define CHAN_CTRL_RXFPRF_SHIFT  18              // Bits 18..19      Specify (Force) RX Pulse Repetition Rate: 00 = 4 MHz, 01 = 16 MHz, 10 = 64MHz.
#define CHAN_CTRL_TNS_SHIFT     20              // Bit 20           TX Non-standard SFD enable
#define CHAN_CTRL_RNS_SHIFT     21              // Bit 21           RX Non-standard SFD enable
#define CHAN_CTRL_TXPCOD_SHIFT  22              // Bits 22..26      TX Preamble Code selection, 1 to 24.
#define CHAN_CTRL_RXPCOD_SHIFT  27              // Bits 27..31      RX Preamble Code selection, 1 to 24.

// Event counter sub addresses
#define EVENT_COUNT0                (0x04)      //sync loss (31-16), PHE (15-0)
#define EVENT_COUNT1                (0x08)      //bad CRC (31-16), good CRC (15-0)
#define EVENT_COUNT2                (0x0C)      //address filter rej (31-16), rx buff overflow (15-0)
#define EVENT_COUNT3                (0x10)      //Preamble TO (31-16), SFD TO (15-0)
#define EVENT_COUNT4                (0x14)      //RTO (31-16), TXF (15-0)
#define EVENT_COUNT5                (0x18)      //half period away, transmit warnings
#define EVENT_COUNT6                (0x1C)      //Preamble rejections


// RF control and configuration subaddresses
#define RF_CTRL2_SUB_ADDR       0x0B            //
#define RF_CTRLTX_SUB_ADDR      0x0C            //

#ifdef __cplusplus
}
#endif

#endif


