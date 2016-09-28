#ifndef _H263_VLD_
#define _H263_VLD_

#define H263_I_PICTURE  0
#define H263_P_PICTURE  1

#define MODE_INTER      0
#define MODE_INTER_Q    1
#define MODE_INTER4V    2
#define MODE_INTRA      3
#define MODE_INTRA_Q    4
#define MODE_INTER4V_Q  5

#define NO_VEC          999
#define ESCAPE          7167

#define mmax(a, b)      ((a) > (b) ? (a) : (b))
#define mmin(a, b)      ((a) < (b) ? (a) : (b))
#define msign(a)         ((a) < 0 ? -1 : 1)
#define mabs(a)         ((a) < 0 ? -(a) : (a))

typedef struct {
    int val, len;
} VLCtab;

unsigned int showbits(int n, int byte_index, int bit_index, unsigned char *buf);
void flushbits(int n, int *byte_index, int *bit_index);
unsigned int getbits(int n, int *byte_index, int *bit_index, unsigned char *buf);
int h263vld(unsigned char *inbuf, unsigned char *outbuf, int inbuf_len, int s263);
int decodeble_h263(unsigned char *buf);


#endif
