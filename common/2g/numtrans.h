#ifndef NUM_TRANS_HPP
#define NUM_TRANS_HPP



#if (defined __ARM32__) || (defined JENNIC_CHIP && JENNIC_CHIP == JN5148)||(defined __STM32__)||(defined JENNIC_CHIP && JENNIC_CHIP == JN5168)
#else
#include "hal_types.h"
#include "Comdef.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "app_protocol.h"

/* change termNbr to string */
/* return the string size */
extern unsigned int num_term2str(char* s, const app_termNbr_t *p);

/* change termNbr to string */
/* return the string size */
extern unsigned int  num_str2term(app_termNbr_t *pterm, char* s);

extern unsigned int num_term_getlen(const app_termNbr_t *p);

extern unsigned char num_isequal(const app_termNbr_t *p1, const app_termNbr_t *p2);

#ifdef __cplusplus
}
#endif

#endif


