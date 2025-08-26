/* if your compiler does not include stdint.h */

/* see read/stdint.ref for details */

#if defined(__xlc__) 	/* AIX PowerPC */
#include <stdint.h>

#elif defined(__hpux) && defined(__ia64)
typedef int			int32_t;	/* 32bits */

#elif defined(__hpux) || defined(_HPUX_SOURCE)
#include <stdlib.h>

#elif (_AIX51 || _WIN32 || SUN || __SUNPRO_C)	/* standard 32 bit compilers */

typedef short			int16_t;	/* 16bits */
typedef unsigned short		uint16_t;	/* 16bits unsigned */

typedef int			int32_t;	/* 32bits */
typedef unsigned int		uint32_t;	/* 32bits unsigned */

typedef long long int		int64_t;	/* 64bits */
typedef unsigned long long int	uint64_t;	/* 64bits unsigned */

#elif ( 0 ) 			/* standard 64 bit compilers */

typedef short			int16_t;	/* 16bits */
typedef unsigned short		uint16_t;	/* 16bits unsigned */

typedef int			int32_t;	/* 32bits */
typedef unsigned int		uint32_t;	/* 32bits unsigned */

typedef long int		int64_t;	/* 64bits */
typedef unsigned long int	uint64_t;	/* 64bits unsigned */

#else				/* the offical C99 way */

#include <stdint.h>

#endif
