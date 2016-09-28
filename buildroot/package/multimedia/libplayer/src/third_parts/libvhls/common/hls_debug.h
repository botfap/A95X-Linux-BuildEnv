#ifndef HLS_DEBUG_H_
#define HLS_DEBUG_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HLS_LOG_BASE,
    HLS_SHOW_URL,
} HLS_LOG_LEVEL;

#ifndef LOGV
#define LOGV(...)	fprintf(stderr,__VA_ARGS__)
#endif

#ifndef LOGI
#define LOGI(...)	fprintf(stderr,__VA_ARGS__)
#endif

#ifndef LOGW
#define LOGW(...)	fprintf(stderr,__VA_ARGS__)
#endif

#ifndef LOGE
#define LOGE(...)   fprintf(stderr,__VA_ARGS__)
#endif


#define TRACE()  printf("TARCE:%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);


#define LITERAL_TO_STRING_INTERNAL(x)    #x
#define LITERAL_TO_STRING(x) LITERAL_TO_STRING_INTERNAL(x)

#define CHECK(condition)                                \
    LOGV(                                \
            !(condition),                               \
            "%s",                                       \
            __FILE__ ":" LITERAL_TO_STRING(__LINE__)    \
            " CHECK(" #condition ") failed.")




#ifdef __cplusplus
}
#endif

#endif

