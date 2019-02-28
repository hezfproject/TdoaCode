#ifndef HAL_AIR_H
#define HAL_AIR_H

#define REPORT_POWERON               1
#define REPORT_WATCHDOG_RESTART   2
#define REPORT_EXTERNAL_RESTART      3
#define REPORT_SPI_RESTART           4
#define REPORT_STARTNWK_FAILED_RESTART           5
#define REPORT_BLAST_ERR_RESTART         6
#define REPORT_MEMORY_ERR_RESTART        7
#define REPORT_PARTBLAST_ERR_RESTART     8
#define REPORT_OAD_RESTART     9
#define REPORT_NO_HEAP     10

void Hal_AirInit (uint16 srcPanId, uint16 srcAddr );
bool_t Hal_SendDataToAir ( const uint8 *p, uint8 len, uint16 dstPan, uint16 dstaddr, bool retrans );
void Hal_TimePoll( void );
void  Hal_AirPoll ( void );
uint8  hal_ProcessDataCnf (MAC_McpsCfmData_s *pCnf );
uint32  Hal_GetTimeMs( void );

#endif
