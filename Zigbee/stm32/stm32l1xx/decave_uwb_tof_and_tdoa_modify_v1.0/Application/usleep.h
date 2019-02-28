/*
 * sleep.h
 *
 *  Created on: May 1, 2011
 *      Author: Ekawahyu Susilo
 */

#ifndef USLEEP_H_
#define USLEEP_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __useconds_t_required_
typedef unsigned long useconds_t;
#endif

#ifdef __tms_struct_required_
struct tms {
	clock_t	tms_utime;		/* user time */
	clock_t	tms_stime;		/* system time */
	clock_t	tms_cutime;		/* user time, children */
	clock_t	tms_cstime;		/* system time, children */
};
#endif

extern unsigned __weak sleep(unsigned seconds);
extern int __weak usleep(useconds_t useconds);

#ifdef __cplusplus
}
#endif

#endif /* SLEEP_H_ */
