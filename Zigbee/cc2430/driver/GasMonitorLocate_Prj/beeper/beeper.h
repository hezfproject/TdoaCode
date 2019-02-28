#ifndef _BEEPER_H_
#define _BEEPER_H_

#define MOTOR_OPEN()	(P1_2=1)
#define MOTOR_CLOSE() 	(P1_2=0)

extern void motor_init(void);
extern void beeper_init(void);
extern void beep_begin(void);
extern void beep_stop(void);

#endif
