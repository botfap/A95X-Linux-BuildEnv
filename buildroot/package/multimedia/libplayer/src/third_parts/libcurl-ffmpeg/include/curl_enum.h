#ifndef CURL_ENUM_H_
#define CURL_ENUM_H_

#include <stdlib.h>

typedef enum {
    C_MAX_REDIRECTS     = 0x0001,
    C_MAX_TIMEOUT           = 0x0002,
    C_MAX_CONNECTTIMEOUT    = 0x0003,
    C_BUFFERSIZE            = 0x0004,
    C_USER_AGENT            = 0x0005,
    C_COOKIES               = 0x0006,
    C_HEADERS               = 0x0007,
    C_HTTPHEADER            = 0x0008,
} curl_para;

typedef enum {
    C_INFO_EFFECTIVE_URL            = 0x0001,
    C_INFO_RESPONSE_CODE            = 0x0002,
    C_INFO_TOTAL_TIME               = 0x0003,
    C_INFO_CONNECT_TIME         = 0x0004,
    C_INFO_SIZE_DOWNLOAD            = 0x0005,
    C_INFO_SPEED_DOWNLOAD       = 0x0006,
    C_INFO_HEADER_SIZE          = 0x0007,
    C_INFO_REQUEST_SIZE         = 0x0008,
    C_INFO_CONTENT_LENGTH_DOWNLOAD  = 0x0009,
    C_INFO_CONTENT_TYPE         = 0x000A,
    C_INFO_REDIRECT_TIME            = 0x000B,
    C_INFO_REDIRECT_COUNT       = 0x000C,
    C_INFO_HTTP_CONNECTCODE     = 0x000D,
    C_INFO_REDIRECT_URL         = 0x000E,
    C_INFO_NUM_CONNECTS         = 0x000F,
    C_INFO_HTTPAUTH_AVAIL       = 0x0010,
    C_INFO_PROXYAUTH_AVAIL      = 0x0011,
    C_INFO_SSL_ENGINES          = 0x0012,
    C_INFO_NAMELOOKUP_TIME      = 0x0013,
    C_INFO_APPCONNECT_TIME      = 0x0014,
} curl_info;

typedef enum {
    C_PROT_HTTP         = 0x001,
    C_PROT_HTTPS            = 0x002,
    C_PROT_FTP              = 0x003,
    C_PROT_FILE         = 0x004,
    C_PROT_TELNET           = 0x005,
} curl_prot_type;

typedef enum {
    C_ERROR_OK                      = 0,
    C_ERROR_UNKNOW              = -1,
    C_ERROR_EAGAIN              = -11, // consider for ffmpeg compatibility
    C_ERROR_PERFORM_BASE_ERROR    = 1000,
} curl_error_code;

#endif
