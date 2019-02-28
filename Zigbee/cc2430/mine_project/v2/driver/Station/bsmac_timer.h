#ifndef __BSMAC_TIMER_H__
#define __BSMAC_TIMER_H__

/*******************************************************************************
** Macro define
*******************************************************************************/
#define BSMAC_TIMER_INIT_SUCCESS        (0)
#define BSMAC_TIMER_INIT_CB_INVALID     (1)

#define BSMAC_TIMER_CH0_INT             (0)
#define BSMAC_TIMER_CH1_INT             (1)

/*******************************************************************************
** Description:
    timer interrupt call back
*******************************************************************************/
typedef void (*bsmac_timer_cb)(unsigned char int_ch);

/*******************************************************************************
** Description:
    initial mac timer(here use time 1)
*******************************************************************************/
unsigned char bsmac_timer_init(bsmac_timer_cb cb);

/*******************************************************************************
** Description:
    restart mac timer
*******************************************************************************/
void bsmac_timer_restart(void);

/*******************************************************************************
** Description:
    start listen rx dma to detect whether it's hanging
*******************************************************************************/
void bsmac_timer_start_listen_rxdma(void);

/*******************************************************************************
** Description:
    stop listen rx dma
*******************************************************************************/
void bsmac_timer_stop_listen_rxdma(void);

/*******************************************************************************
** Description:
    reset live timer
*******************************************************************************/
void bsmac_timer_reset_live(void);


#endif  // __BSMAC_TIMER_H__
