#ifndef AMSOURCE_HH_HEADER_HH
#define  AMSOURCE_HH_HEADER_HH

#include <stdlib.h>
struct source;
struct  source_options {
    int64_t filesize;
    int      is_streaming;
    int64_t  duration_ms;
    int      support_seek_by_time;
};


typedef struct sourceprot {
    const char *name;
    int (*open)(struct source *s, const char *url, const char *header, int flags);
    int (*read)(struct source *s, unsigned char *buf, int size);
    int (*write)(struct source *s,  unsigned char *buf, int size);
    int64_t (*seek)(struct source *s, int64_t pos, int whence);
    int (*supporturl)(struct source *s, const char *url, const char *header, int flags);
    int (*close)(struct source *s);
} sourceprot_t;
#define MAX_URL_SIZE 4096
typedef struct source {
    const char * url;
    char location[MAX_URL_SIZE];
    const char * header;
    int flags;
    int64_t s_off;/*don't changed it on lowlevel */
    sourceprot_t *prot;
    int firstread;
    struct  source_options options;
    unsigned long priv[64];
} source_t;


#define SOURCE_ERROR_BASE   (-111000)
#define SOURCE_ERROR_OPENFAILED (SOURCE_ERROR_BASE-0)
#define SOURCE_ERROR_EOF            (SOURCE_ERROR_BASE-1)
#define SOURCE_ERROR_NOMEN      (SOURCE_ERROR_BASE-2)
#define SOURCE_ERROR_IO         (SOURCE_ERROR_BASE-3)
#define SOURCE_ERROR_DISCONNECT     (SOURCE_ERROR_BASE-4)
#define SOURCE_ERROR_SEEK_FAILED        (SOURCE_ERROR_BASE-5)
#define SOURCE_ERROR_NOT_OPENED     (SOURCE_ERROR_BASE-6)
#define SOURCE_ERROR_SIZE_NOT_VALIED     (SOURCE_ERROR_BASE-7)


#define SOURCE_SEEK_BASE   (11000)
#define SOURCE_SEEK_SIZE   (11000+1)
#define SOURCE_SEEK_BY_TIME   (11000+2)

source_t *new_source(const char * url, const char *header, int flags);
int source_open(source_t *as);
int source_close(source_t *as);
int source_read(source_t *as, char * buf, int size);
int64_t source_seek(source_t *as, int64_t off, int whence);
int release_source(source_t *as);
int register_source(sourceprot_t *source);
int source_init_all(void);
int64_t source_size(source_t *as);
int source_getoptions(source_t *as, struct source_options *op);
#endif

