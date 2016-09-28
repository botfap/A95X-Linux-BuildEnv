/**
* @file codec_msg.c
* @brief  Codec message covertion functions
* @author Zhang Chen <chen.zhang@amlogic.com>
* @version 1.0.0
* @date 2011-02-24
*/
/* Copyright (C) 2007-2011, Amlogic Inc.
* All right reserved
* 
*/
#include <stdio.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <codec_error.h>
#include <codec.h>
#include "codec_h_ctrl.h"


/* --------------------------------------------------------------------------*/
/**
* @brief  system_error_to_codec_error  Convert system error to codec error types
*
* @param[in]  error  System error to be converted
*
* @return     Codec error type
*/
/* --------------------------------------------------------------------------*/
int system_error_to_codec_error(int error)
{
    switch (error) {
    case 0:
        return CODEC_ERROR_NONE;
    case EBUSY:
        return -CODEC_ERROR_BUSY;
    case ENOMEM:
        return -CODEC_ERROR_NOMEM;
    case ENODEV:
        return -CODEC_ERROR_IO;
    default:
        return -(C_PAE|error);
    }
}

typedef struct 
{
	int error_no;
	char buf[256];
}codec_errors_t;

const codec_errors_t codec_errno[] =
{
	//codec error
	{CODEC_ERROR_NONE, "codec no errors"},
	{-CODEC_ERROR_INVAL, "invalid handle or parameter"}, 
	{-CODEC_ERROR_BUSY, "codec is busy"},
	{-CODEC_ERROR_NOMEM, "no enough memory for codec"},
	{-CODEC_ERROR_IO, "codec io error"},
	{-CODEC_ERROR_PARAMETER, "Parameters error"},
	{-CODEC_ERROR_AUDIO_TYPE_UNKNOW, "Audio Type error"},
	{-CODEC_ERROR_VIDEO_TYPE_UNKNOW, "Video Type error"},
	{-CODEC_ERROR_STREAM_TYPE_UNKNOW, "Stream Type error"},
	{-CODEC_ERROR_INIT_FAILED, "Codec init failed"},
	{-CODEC_ERROR_SET_BUFSIZE_FAILED, "Codec change buffer size failed"},

	//errno, definition in error.h
	{EPERM,"Operation not permitted"},			// 1
	{ENOENT,"No such file or directory"},		// 2
	{ESRCH, "No such process"},					// 3
	{EINTR, "Interrupted system call"},			// 4
	{EIO, "I/O error"},							// 5
	{ENXIO, "No such device or address"},		// 6
	{E2BIG, "Arg list too long"},				// 7
	{ENOEXEC, "Exec format error"},				// 8
	{EBADF, "Bad file number"},					// 9
	{ECHILD, "No child processes"},				// 10
	{EAGAIN, "Try again"},						// 11
	{ENOMEM, "Out of memory"},					// 12
	{EACCES, "Permission denied"},				// 13
	{EFAULT, "Bad address"},					// 14
	{ENOTBLK, "Block device required"},			// 15
	{EBUSY, "Device or resource busy"},			// 16
	{EEXIST, "File exists"},					// 17
	{EXDEV, "Cross-device link"},				// 18
	{ENODEV, "No such device"},					// 19
	{ENOTDIR, "Not a directory"},				// 20
	{EISDIR, "Is a directory"},					// 21
	{EINVAL, "Invalid argument"},				// 22
	{ENFILE, "File table overflow"},			// 23
	{EMFILE, "Too many open files"},			// 24
	{ENOTTY, "Not a typewriter"},				// 25
	{ETXTBSY, "Text file busy"},				// 26
	{EFBIG, "File too large"},					// 27
	{ENOSPC, "No space left on device"},		// 28
	{ESPIPE, "Illegal seek"},					// 29
	{EROFS, "Read-only file system"},			// 30
	{EMLINK, "Too many links"},					// 31
	{EPIPE, "Broken pipe"},						// 32
	{EDOM, "Math argument out of domain of func"},	// 33
	{ERANGE, "Math result not representable"},	// 34
	{EDEADLK, "Resource deadlock would occur"},	// 35
	{ENAMETOOLONG, "File name too long"},		// 36
	{ENOLCK, "No record locks available"},		// 37
	{ENOSYS, "Function not implemented"},		// 38
	{ENOTEMPTY, "Directory not empty"},			// 39
	{ELOOP, "Too many symbolic links encountered"},	// 40
	{EWOULDBLOCK, "Operation would block"},	// 41
	{ENOMSG, "No message of desired type"},			// 42
	{EIDRM, "Identifier removed"},				// 43
	{ECHRNG, "Channel number out of range"},	// 44
	{EL2NSYNC, "Level 2 not synchronized"},		// 45
	{EL3HLT, "Level 3 halted"},					// 46
	{EL3RST, "Level 3 reset"},					// 47
	{ELNRNG, "Link number out of range"},		// 48
	{EUNATCH, "Protocol driver not attached"},	// 49
	{ENOCSI, "No CSI structure available"},		// 50
	{EL2HLT, "Level 2 halted"},					// 51
	{EBADE, "Invalid exchange"},				// 52
	{EBADR, "Invalid request descriptor"},		// 53
	{EXFULL, "Exchange full"},					// 54
	{ENOANO, "No anode"},						// 55
	{EBADRQC, "Invalid request code"},			// 56
	{EBADSLT, "Invalid slot"},					// 57
	{EDEADLOCK,"dead lock/link"},				// 58
	{EBFONT, "Bad font file format"},			// 59 
	{ENOSTR, "Device not a stream"},			// 60
	{ENODATA, "No data available"},				// 61
	{ETIME, "Timer expired"},					// 62
	{ENOSR, "Out of streams resources"},		// 63
	{ENONET, "Machine is not on the network"},	// 64
	{ENOPKG, "Package not installed"},						// 65
	{EREMOTE, "Object is remote"},							// 66
	{ENOLINK, "Link has been severed "},					// 67
	{EADV, "Advertise error"},								// 68
	{ESRMNT, "Srmount error"},								// 69
	{ECOMM, "Communication error on send"},					// 70
	{EPROTO,"Protocol error"},								// 71
	{EMULTIHOP, "Multihop attempted"},						// 72
	{EDOTDOT, "RFS specific error"},						// 73
	{EBADMSG, "Not a data message"},						// 74
	{EOVERFLOW, "Value too large for defined data type"},	// 75
	{ENOTUNIQ, "Name not unique on network "},				// 76
	{EBADFD, "File descriptor in bad state"},				// 77
	{EREMCHG, "Remote address changed "},					// 78
	{ELIBACC, "Can not access a needed shared library"},	//79
	{ELIBBAD, "Accessing a corrupted shared library"},		// 80
	{ELIBSCN, ".lib section in a.out corrupted"},			// 81
	{ELIBMAX, "Attempting to link in too many shared libraries"},	// 82
	{ELIBEXEC, "Cannot exec a shared library directly"}, 	// 83	
	{EILSEQ, "Illegal byte sequence"},						// 84
	{ERESTART, "Interrupted system call should be restarted"},	// 85
	{ESTRPIPE, "Streams pipe error "},						// 86
	{EUSERS, "Too many users"},								// 87
	{ENOTSOCK, "Socket operation on non-socket"},			// 88
	{EDESTADDRREQ, "Destination address required"},			// 89
	{EMSGSIZE, "Message too long"},							// 90
	{EPROTOTYPE, "Protocol wrong type for socket"},			// 91
	{ENOPROTOOPT, "Protocol not available"},				// 92
	//{EPROTONOSUPPORT, "Protocol not supported"},			// 93
	{ESOCKTNOSUPPORT, "Socket type not supported"}, 		// 94
	{EOPNOTSUPP, "Operation not supported on transport endpoint"}, // 95
	{EPFNOSUPPORT, "Protocol family not supported"}, 		// 96
	{EAFNOSUPPORT, "Address family not supported by protocol"},		// 97
	{EADDRINUSE, "Address already in use"},					// 98
	{EADDRNOTAVAIL, "Cannot assign requested address"},		// 99
	{ENETDOWN, "Network is down"},							// 100	
    {ENETUNREACH, "Network is unreachable"},				// 101
	{ENETRESET, "Network dropped connection because of reset"},		// 102
	{ECONNABORTED, "Software caused connection abort"},		// 103
	{ECONNRESET, "Connection reset by peer"},				// 104
	{ENOBUFS, "No buffer space available"},					// 105
	{EISCONN, "Transport endpoint is already connected"},	// 106
	{ENOTCONN, "Transport endpoint is not connected"},		// 107
	{ESHUTDOWN, "Cannot send after transport endpoint shutdown"},	// 108
	{ETOOMANYREFS, "Too many references: cannot splice"},	// 109
	//{ETIMEDOUT, "Connection timed out"},					// 110
	//{ECONNREFUSED, "Connection refused"},					// 111
	{EHOSTDOWN, "Host is down"},							// 112
	{EHOSTUNREACH, "No route to host"},						// 113
	{EALREADY, "Operation already in progress"},			// 114
	//{EINPROGRESS, "Operation now in progress"},				// 115
	{ESTALE, "Stale NFS file handle"},						// 116
	{EUCLEAN, "Structure needs cleaning"},					// 117
	{ENOTNAM, "Not a XENIX named type file"},				// 118
	{ENAVAIL, "No XENIX semaphores available"},				// 119
	{EISNAM, "Is a named type file"},						// 120
	{EREMOTEIO, "Remote I/O error"},						// 121
	{EDQUOT, "Quota exceeded"},								// 122
	{ENOMEDIUM, "No medium found"},							// 123
	{EMEDIUMTYPE, "Wrong medium type"}						// 124
};

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_error_msg  Convert codec type to error message
*
* @param[in]  error  Codec error type
*
* @return     Error message string
*/
/* --------------------------------------------------------------------------*/
const char * codec_error_msg(int error)
{
	unsigned int i;
	for(i = 0; i < (sizeof(codec_errno)/sizeof(codec_errors_t)); i ++)
	{
		if (error == codec_errno[i].error_no)
			return codec_errno[i].buf;		
	} 
    return "invalid operate";   
}

/* --------------------------------------------------------------------------*/
/**
* @brief  print_error_msg  Print error message in uniform format
*
* @param[in]  error  Codec error type
* @param[in]  errno  Codec error number, define in error.h
* @param[in]  func   Function where error happens
* @param[in]  line   Line where error happens
*/
/* --------------------------------------------------------------------------*/
void print_error_msg(int error, int syserr, char *func, int line)
{
    CODEC_PRINT("Error=%x errno=%d : %s,func=%s,line=%d\n", error, syserr, codec_error_msg(syserr),func, line);
}


