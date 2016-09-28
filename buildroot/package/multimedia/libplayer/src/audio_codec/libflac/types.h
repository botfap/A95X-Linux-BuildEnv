
#ifndef _LIBFLAC_TYPES_H
#define	_LIBFLAC_TYPES_H
/* bsd */
typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned int		u_int;
typedef unsigned long		u_long;

/* sysv */
typedef unsigned char		unchar;
typedef unsigned short		ushort;
typedef unsigned int		uint;
typedef unsigned long		ulong;

typedef unsigned char		__u8;
typedef unsigned short	__u16;
typedef unsigned int		__u32;
typedef unsigned long long		__u64;


typedef  signed char		__s8;
typedef  short	__s16;
typedef  int		__s32;
typedef  long long		__s64;

typedef		__u8		u_int8_t;
typedef		__s8		int8_t;
typedef		__u16		u_int16_t;
typedef		__s16		int16_t;
typedef		__u32		u_int32_t;
typedef		__s32		int32_t;


typedef		__u8		uint8_t;
typedef		__u16		uint16_t;
typedef		__u32		uint32_t;

typedef		__u64		uint64_t;
typedef		__u64		u_int64_t;
typedef		__s64		int64_t;
typedef		float  		float32_t;


#ifndef NULL
#define NULL ((void *)0)
#endif

#endif
