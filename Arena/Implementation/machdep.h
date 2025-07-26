/*
 * Machine dependent definitions
 *
 * added alpha			(wm 24.Mar.95)
 *
 */

#ifndef _MACHDEP_
#define _MACHDEP_

#if defined (__alpha) && defined (__osf__)
#define POINTER_IS_64BIT	1
#define POINTERSIZE     	8 /* byte */
#elif defined(__APPLE__) && defined(__MACH__)
#define POINTER_IS_64BIT 1
#define POINTERSIZE      8 /* byte */
#else
#define POINTERSIZE     	4 /* byte */
#endif /* __alpha && __osf__ || __APPLE__ && __MACH__ */

#if defined (__alpha) && defined (__osf__)
#define PRINTF_HAS_PFORMAT	1 /* printf family supports the %p format */
#elif defined(__APPLE__) && defined(__MACH__)
#define PRINTF_HAS_PFORMAT 1
#endif /* __alpha && __osf__ || __APPLE__ && __MACH__ */

#endif /* _MACHDEP_ */
