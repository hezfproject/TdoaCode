#ifndef _COMMON_TYPES_H_
#define _COMMON_TYPES_H_
/*********************************************************************
 * TYPEDEFS
 */
#if (defined __ARM32__) || (defined __CC_ARM) || (defined JENNIC_CHIP && (JENNIC_CHIP == JN5148 || JENNIC_CHIP == JN5168))
#define __PACKED __attribute__((__packed__))
#else
#define __PACKED
#endif

#if (defined __ARM32__) || (defined __CC_ARM) || (defined JENNIC_CHIP && (JENNIC_CHIP == JN5148 || JENNIC_CHIP == JN5168))
#define __CODE
#else
#define __CODE __code
#endif

typedef unsigned char 	uint_8;
typedef signed char 	int_8;
typedef unsigned short 	uint_16;
typedef short			int_16;
typedef unsigned long   uint_32;
typedef long			int_32;
typedef unsigned long long uint_64;
typedef long long int_64;

#endif
