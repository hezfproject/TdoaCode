#ifndef _TIMER_H_
#define _TIMER_H_

void start_Timer1(void);
void start_Timer0(void);
void init_Timer0(void);
void stop_Timer0(void);

#define  BV(n)   (0x01<<(n))
#define  RECEIVE_FRE     50000  //don,t modify it,if you think not more modify or the source not change!
#define  TIMER0FOOD   (255-(1000000/(4*RECEIVE_FRE)))

#endif//_TIMER_H_
