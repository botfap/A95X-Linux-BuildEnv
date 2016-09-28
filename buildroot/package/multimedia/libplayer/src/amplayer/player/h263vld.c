#include "h263vld.h"

static unsigned int msk[33] = {
    0x00000000, 0x00000001, 0x00000003, 0x00000007,
    0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
    0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
    0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
    0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
    0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
    0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
    0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
    0xffffffff
};

static VLCtab MCBPCtabintra[] = {
    { -1, 0},
    {20, 6}, {36, 6}, {52, 6}, { 4, 4}, { 4, 4}, { 4, 4},
    { 4, 4}, {19, 3}, {19, 3}, {19, 3}, {19, 3}, {19, 3},
    {19, 3}, {19, 3}, {19, 3}, {35, 3}, {35, 3}, {35, 3},
    {35, 3}, {35, 3}, {35, 3}, {35, 3}, {35, 3}, {51, 3},
    {51, 3}, {51, 3}, {51, 3}, {51, 3}, {51, 3}, {51, 3},
    {51, 3},
};

static VLCtab MCBPCtab0[] = {
    { -1, 0},
    {255, 9}, {52, 9}, {36, 9}, {20, 9}, {49, 9}, {35, 8}, {35, 8}, {19, 8}, {19, 8},
    { 50, 8}, {50, 8}, {51, 7}, {51, 7}, {51, 7}, {51, 7}, {34, 7}, {34, 7}, {34, 7},
    { 34, 7}, {18, 7}, {18, 7}, {18, 7}, {18, 7}, {33, 7}, {33, 7}, {33, 7}, {33, 7},
    { 17, 7}, {17, 7}, {17, 7}, {17, 7}, { 4, 6}, { 4, 6}, { 4, 6}, { 4, 6}, { 4, 6},
    {  4, 6}, { 4, 6}, { 4, 6}, {48, 6}, {48, 6}, {48, 6}, {48, 6}, {48, 6}, {48, 6},
    { 48, 6}, {48, 6}, { 3, 5}, { 3, 5}, { 3, 5}, { 3, 5}, { 3, 5}, { 3, 5}, { 3, 5},
    {  3, 5}, { 3, 5}, { 3, 5}, { 3, 5}, { 3, 5}, { 3, 5}, { 3, 5}, { 3, 5}, { 3, 5},
    { 32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4},
    { 32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4},
    { 32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4},
    { 32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4},
    { 16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4},
    { 16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4},
    { 16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4},
    { 16, 4}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3},
    {  2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3},
    {  2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3},
    {  2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3},
    {  2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3},
    {  2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3},
    {  2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3}, { 2, 3},
    {  2, 3}, { 2, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3},
    {  1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3},
    {  1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3},
    {  1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3},
    {  1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3},
    {  1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3},
    {  1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3}, { 1, 3},
    {  1, 3}, { 1, 3}, { 1, 3},
};

static VLCtab MCBPCtab1[] = {
    {5, 11}, {5, 11},  {5, 11}, {5, 11}, {21, 13}, {21, 13}, {37, 13}, {53, 13},
};

static VLCtab CBPYtab[48] = {
    { -1, 0}, { -1, 0}, { 9, 6}, { 6, 6}, { 7, 5}, { 7, 5}, {11, 5}, {11, 5},
    {13, 5}, {13, 5}, {14, 5}, {14, 5}, {15, 4}, {15, 4}, {15, 4}, {15, 4},
    { 3, 4}, { 3, 4}, { 3, 4}, { 3, 4}, { 5, 4}, { 5, 4}, { 5, 4}, { 5, 4},
    { 1, 4}, { 1, 4}, { 1, 4}, { 1, 4}, {10, 4}, {10, 4}, {10, 4}, {10, 4},
    { 2, 4}, { 2, 4}, { 2, 4}, { 2, 4}, {12, 4}, {12, 4}, {12, 4}, {12, 4},
    { 4, 4}, { 4, 4}, { 4, 4}, { 4, 4}, { 8, 4}, { 8, 4}, { 8, 4}, { 8, 4},
};

int DQ_tab[4] = { -1, -2, 1, 2};

static VLCtab TMNMVtab0[] = {
    { 3, 4}, {61, 4}, { 2, 3}, { 2, 3}, {62, 3}, {62, 3},
    { 1, 2}, { 1, 2}, { 1, 2}, { 1, 2}, {63, 2}, {63, 2}, {63, 2}, {63, 2}
};

static VLCtab TMNMVtab1[] = {
    {12, 10}, {52, 10}, {11, 10}, {53, 10}, {10, 9}, {10, 9},
    {54, 9}, {54, 9}, { 9, 9}, { 9, 9}, {55, 9}, {55, 9},
    { 8, 9}, { 8, 9}, {56, 9}, {56, 9}, { 7, 7}, { 7, 7},
    { 7, 7}, { 7, 7}, { 7, 7}, { 7, 7}, { 7, 7}, { 7, 7},
    {57, 7}, {57, 7}, {57, 7}, {57, 7}, {57, 7}, {57, 7},
    {57, 7}, {57, 7}, { 6, 7}, { 6, 7}, { 6, 7}, { 6, 7},
    { 6, 7}, { 6, 7}, { 6, 7}, { 6, 7}, {58, 7}, {58, 7},
    {58, 7}, {58, 7}, {58, 7}, {58, 7}, {58, 7}, {58, 7},
    { 5, 7}, { 5, 7}, { 5, 7}, { 5, 7}, { 5, 7}, { 5, 7},
    { 5, 7}, { 5, 7}, {59, 7}, {59, 7}, {59, 7}, {59, 7},
    {59, 7}, {59, 7}, {59, 7}, {59, 7}, { 4, 6}, { 4, 6},
    { 4, 6}, { 4, 6}, { 4, 6}, { 4, 6}, { 4, 6}, { 4, 6},
    { 4, 6}, { 4, 6}, { 4, 6}, { 4, 6}, { 4, 6}, { 4, 6},
    { 4, 6}, { 4, 6}, {60, 6}, {60, 6}, {60, 6}, {60, 6},
    {60, 6}, {60, 6}, {60, 6}, {60, 6}, {60, 6}, {60, 6},
    {60, 6}, {60, 6}, {60, 6}, {60, 6}, {60, 6}, {60, 6}
};

static VLCtab TMNMVtab2[] = {
    {32, 12}, {31, 12}, {33, 12}, {30, 11}, {30, 11}, {34, 11},
    {34, 11}, {29, 11}, {29, 11}, {35, 11}, {35, 11}, {28, 11},
    {28, 11}, {36, 11}, {36, 11}, {27, 11}, {27, 11}, {37, 11},
    {37, 11}, {26, 11}, {26, 11}, {38, 11}, {38, 11}, {25, 11},
    {25, 11}, {39, 11}, {39, 11}, {24, 10}, {24, 10}, {24, 10},
    {24, 10}, {40, 10}, {40, 10}, {40, 10}, {40, 10}, {23, 10},
    {23, 10}, {23, 10}, {23, 10}, {41, 10}, {41, 10}, {41, 10},
    {41, 10}, {22, 10}, {22, 10}, {22, 10}, {22, 10}, {42, 10},
    {42, 10}, {42, 10}, {42, 10}, {21, 10}, {21, 10}, {21, 10},
    {21, 10}, {43, 10}, {43, 10}, {43, 10}, {43, 10}, {20, 10},
    {20, 10}, {20, 10}, {20, 10}, {44, 10}, {44, 10}, {44, 10},
    {44, 10}, {19, 10}, {19, 10}, {19, 10}, {19, 10}, {45, 10},
    {45, 10}, {45, 10}, {45, 10}, {18, 10}, {18, 10}, {18, 10},
    {18, 10}, {46, 10}, {46, 10}, {46, 10}, {46, 10}, {17, 10},
    {17, 10}, {17, 10}, {17, 10}, {47, 10}, {47, 10}, {47, 10},
    {47, 10}, {16, 10}, {16, 10}, {16, 10}, {16, 10}, {48, 10},
    {48, 10}, {48, 10}, {48, 10}, {15, 10}, {15, 10}, {15, 10},
    {15, 10}, {49, 10}, {49, 10}, {49, 10}, {49, 10}, {14, 10},
    {14, 10}, {14, 10}, {14, 10}, {50, 10}, {50, 10}, {50, 10},
    {50, 10}, {13, 10}, {13, 10}, {13, 10}, {13, 10}, {51, 10},
    {51, 10}, {51, 10}, {51, 10}
};

VLCtab DCT3Dtab0[] = {
    {4225, 7}, {4209, 7}, {4193, 7}, {4177, 7}, { 193, 7}, { 177, 7},
    { 161, 7}, {   4, 7}, {4161, 6}, {4161, 6}, {4145, 6}, {4145, 6},
    {4129, 6}, {4129, 6}, {4113, 6}, {4113, 6}, { 145, 6}, { 145, 6},
    { 129, 6}, { 129, 6}, { 113, 6}, { 113, 6}, {  97, 6}, {  97, 6},
    {  18, 6}, {  18, 6}, {   3, 6}, {   3, 6}, {  81, 5}, {  81, 5},
    {  81, 5}, {  81, 5}, {  65, 5}, {  65, 5}, {  65, 5}, {  65, 5},
    {  49, 5}, {  49, 5}, {  49, 5}, {  49, 5}, {4097, 4}, {4097, 4},
    {4097, 4}, {4097, 4}, {4097, 4}, {4097, 4}, {4097, 4}, {4097, 4},
    {   1, 2}, {   1, 2}, {   1, 2}, {   1, 2}, {   1, 2}, {   1, 2},
    {   1, 2}, {   1, 2}, {   1, 2}, {   1, 2}, {   1, 2}, {   1, 2},
    {   1, 2}, {   1, 2}, {   1, 2}, {   1, 2}, {   1, 2}, {   1, 2},
    {   1, 2}, {   1, 2}, {   1, 2}, {   1, 2}, {   1, 2}, {   1, 2},
    {   1, 2}, {   1, 2}, {   1, 2}, {   1, 2}, {   1, 2}, {   1, 2},
    {   1, 2}, {   1, 2}, {  17, 3}, {  17, 3}, {  17, 3}, {  17, 3},
    {  17, 3}, {  17, 3}, {  17, 3}, {  17, 3}, {  17, 3}, {  17, 3},
    {  17, 3}, {  17, 3}, {  17, 3}, {  17, 3}, {  17, 3}, {  17, 3},
    {  33, 4}, {  33, 4}, {  33, 4}, {  33, 4}, {  33, 4}, {  33, 4},
    {  33, 4}, {  33, 4}, {   2, 4}, {   2, 4}, {   2, 4}, {   2, 4},
    {   2, 4}, {   2, 4}, {   2, 4}, {   2, 4},
};


VLCtab DCT3Dtab1[] = {
    {   9, 10}, {   8, 10}, {4481, 9}, {4481, 9}, {4465, 9}, {4465, 9},
    {4449, 9}, {4449, 9}, {4433, 9}, {4433, 9}, {4417, 9}, {4417, 9},
    {4401, 9}, {4401, 9}, {4385, 9}, {4385, 9}, {4369, 9}, {4369, 9},
    {4098, 9}, {4098, 9}, { 353, 9}, { 353, 9}, { 337, 9}, { 337, 9},
    { 321, 9}, { 321, 9}, { 305, 9}, { 305, 9}, { 289, 9}, { 289, 9},
    { 273, 9}, { 273, 9}, { 257, 9}, { 257, 9}, { 241, 9}, { 241, 9},
    {  66, 9}, {  66, 9}, {  50, 9}, {  50, 9}, {   7, 9}, {   7, 9},
    {   6, 9}, {   6, 9}, {4353, 8}, {4353, 8}, {4353, 8}, {4353, 8},
    {4337, 8}, {4337, 8}, {4337, 8}, {4337, 8}, {4321, 8}, {4321, 8},
    {4321, 8}, {4321, 8}, {4305, 8}, {4305, 8}, {4305, 8}, {4305, 8},
    {4289, 8}, {4289, 8}, {4289, 8}, {4289, 8}, {4273, 8}, {4273, 8},
    {4273, 8}, {4273, 8}, {4257, 8}, {4257, 8}, {4257, 8}, {4257, 8},
    {4241, 8}, {4241, 8}, {4241, 8}, {4241, 8}, { 225, 8}, { 225, 8},
    { 225, 8}, { 225, 8}, { 209, 8}, { 209, 8}, { 209, 8}, { 209, 8},
    {  34, 8}, {  34, 8}, {  34, 8}, {  34, 8}, {  19, 8}, {  19, 8},
    {  19, 8}, {  19, 8}, {   5, 8}, {   5, 8}, {   5, 8}, {   5, 8},
};


VLCtab DCT3Dtab2[] = {
    {4114, 11}, {4114, 11}, {4099, 11}, {4099, 11}, {  11, 11}, {  11, 11},
    {  10, 11}, {  10, 11}, {4545, 10}, {4545, 10}, {4545, 10}, {4545, 10},
    {4529, 10}, {4529, 10}, {4529, 10}, {4529, 10}, {4513, 10}, {4513, 10},
    {4513, 10}, {4513, 10}, {4497, 10}, {4497, 10}, {4497, 10}, {4497, 10},
    { 146, 10}, { 146, 10}, { 146, 10}, { 146, 10}, { 130, 10}, { 130, 10},
    { 130, 10}, { 130, 10}, { 114, 10}, { 114, 10}, { 114, 10}, { 114, 10},
    {  98, 10}, {  98, 10}, {  98, 10}, {  98, 10}, {  82, 10}, {  82, 10},
    {  82, 10}, {  82, 10}, {  51, 10}, {  51, 10}, {  51, 10}, {  51, 10},
    {  35, 10}, {  35, 10}, {  35, 10}, {  35, 10}, {  20, 10}, {  20, 10},
    {  20, 10}, {  20, 10}, {  12, 11}, {  12, 11}, {  21, 11}, {  21, 11},
    { 369, 11}, { 369, 11}, { 385, 11}, { 385, 11}, {4561, 11}, {4561, 11},
    {4577, 11}, {4577, 11}, {4593, 11}, {4593, 11}, {4609, 11}, {4609, 11},
    {  22, 12}, {  36, 12}, {  67, 12}, {  83, 12}, {  99, 12}, { 162, 12},
    { 401, 12}, { 417, 12}, {4625, 12}, {4641, 12}, {4657, 12}, {4673, 12},
    {4689, 12}, {4705, 12}, {4721, 12}, {4737, 12}, {7167, 7}, {7167, 7},
    {7167, 7}, {7167, 7}, {7167, 7}, {7167, 7}, {7167, 7}, {7167, 7},
    {7167, 7}, {7167, 7}, {7167, 7}, {7167, 7}, {7167, 7}, {7167, 7},
    {7167, 7}, {7167, 7}, {7167, 7}, {7167, 7}, {7167, 7}, {7167, 7},
    {7167, 7}, {7167, 7}, {7167, 7}, {7167, 7}, {7167, 7}, {7167, 7},
    {7167, 7}, {7167, 7}, {7167, 7}, {7167, 7}, {7167, 7}, {7167, 7},
};

int roundtab[16] = {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2};

int top_mv[90][2], left_mv[2][2], mv[4][2];

unsigned int showbits(int n, int byte_index, int bit_index, unsigned char *buf)
{
    unsigned int data;

    data = (buf[byte_index] << 24) | (buf[byte_index + 1] << 16) | (buf[byte_index + 2] << 8) | buf[byte_index + 3];
    return (data >> (25 + bit_index - n)) & msk[n];
}

void flushbits(int n, int *byte_index, int *bit_index)
{
    *byte_index += n / 8;
    *bit_index -= n % 8;

    if (*bit_index < 0) {
        *bit_index += 8;
        (*byte_index) ++;
    }
}

unsigned int getbits(int n, int *byte_index, int *bit_index, unsigned char *buf)
{
    unsigned int l;

    l = showbits(n, *byte_index, *bit_index, buf);
    flushbits(n, byte_index, bit_index);
    return l;
}

int startcode(int *byte_index, int *bit_index, unsigned char *buf, int len)
{
#if 0
    int i;
    unsigned int start_code;

    if (*bit_index != 7) {
        *bit_index = 7;
        (*byte_index) ++;
    }

    start_code = buf[(*byte_index)];
    start_code = (start_code << 8) | buf[(*byte_index) + 1];

    for (i = (*byte_index) ; i < len - 2 ; i++) {
        start_code = (start_code << 8) | buf[i + 2];

        if ((start_code & 0xffff80) == 0x000080) {
            break;
        }
    }

    if (i == len - 2) {
        return -1;
    }

    *byte_index = i;
#else
    while (showbits(17, *byte_index, *bit_index, buf) != 1) {
        flushbits(1, byte_index, bit_index);

        if ((*byte_index) > len || ((*byte_index) == len && (*bit_index) < 7)) {
            return -1;
        }
    }
#endif

    return 0;
}

int get_pred_mv(int x, int k, int comp)
{
    int mv1, mv2, mv3;

    if (k == 0) {
        mv1 = left_mv[0][comp];
        mv2 = top_mv[x * 2][comp];
        mv3 = top_mv[(x + 1) * 2][comp];
    } else if (k == 1) {
        mv1 = mv[0][comp];
        mv2 = top_mv[x * 2 + 1][comp];
        mv3 = top_mv[(x + 1) * 2][comp];
    } else if (k == 2) {
        mv1 = left_mv[1][comp];
        mv2 = mv[0][comp];
        mv3 = mv[1][comp];
    } else {
        mv1 = mv[2][comp];
        mv2 = mv[0][comp];
        mv3 = mv[1][comp];
    }

    if (mv2 == NO_VEC) {
        mv2 = mv3 = mv1;
    }

    return mv1 + mv2 + mv3 - mmax(mv1, mmax(mv2, mv3)) - mmin(mv1, mmin(mv2, mv3));
}

int motion_decode(int vec, int pmv)
{
    if (vec > 31) {
        vec -= 64;
    }

    vec += pmv;

    if (vec > 31) {
        vec -= 64;
    }

    if (vec < -32) {
        vec += 64;
    }

    return vec;
}

int h263vld(unsigned char *inbuf, unsigned char *outbuf, int inbuf_len, int s263)
{
    int outbuf_len = 0, byte_index = 0, bit_index = 7;
    int i, j, k, comp, temp, h263_version, pict_type, quant, mb_width, mb_height;
    int MCBPC, Mode, CBPY, CBP, DQUANT, pred, run, last, level, val, sign, format_bit, coeff_count;
    static int width = 0, height = 0, source_format = 0, optional_custom_PCF = 0;
    VLCtab *tab;

    outbuf[outbuf_len++] = 0;
    outbuf[outbuf_len++] = 0;
    outbuf[outbuf_len++] = 1;
    outbuf[outbuf_len++] = 0xb6;

    getbits(17, &byte_index, &bit_index, inbuf);
    h263_version = getbits(5, &byte_index, &bit_index, inbuf);
    outbuf[outbuf_len++] = getbits(8, &byte_index, &bit_index, inbuf);

    if (s263 == 0) {
        temp = getbits(5, &byte_index, &bit_index, inbuf);
        if (temp != 16) {
            return 0;
        }

        temp = getbits(3, &byte_index, &bit_index, inbuf);

        if (temp == 7) {
            int UFEP, rounding_type;

            UFEP = getbits(3, &byte_index, &bit_index, inbuf);
            if (UFEP >= 2) {
                return 0;
            }

            if (UFEP == 1) {
                source_format = getbits(3, &byte_index, &bit_index, inbuf);
                optional_custom_PCF = getbits(1, &byte_index, &bit_index, inbuf);
                temp = getbits(14, &byte_index, &bit_index, inbuf);
                if (temp != 8) {
                    return 0;
                }
            }

            pict_type = getbits(3, &byte_index, &bit_index, inbuf);
            if (pict_type >= 2) {
                return 0;
            }

            temp = getbits(2, &byte_index, &bit_index, inbuf);
            if (temp != 0) {
                return 0;
            }

            rounding_type = getbits(1, &byte_index, &bit_index, inbuf);

            temp = getbits(3, &byte_index, &bit_index, inbuf);
            if (temp != 1) {
                return 0;
            }

            temp = getbits(1, &byte_index, &bit_index, inbuf);
            if (temp != 0) {
                return 0;
            }

            if (UFEP == 1) {
                switch (source_format) {
                case 1:
                    width = 128;
                    height = 96;
                    break;
                case 2:
                    width = 176;
                    height = 144;
                    break;
                case 3:
                    width = 352;
                    height = 288;
                    break;
                case 4:
                    width = 704;
                    height = 576;
                    break;
                case 5:
                    width = 1408;
                    height = 1152;
                    break;
                case 6:
                    temp = getbits(4, &byte_index, &bit_index, inbuf);
                    width = getbits(9, &byte_index, &bit_index, inbuf);
                    width = (width + 1) * 4;
                    getbits(1, &byte_index, &bit_index, inbuf);
                    height = getbits(9, &byte_index, &bit_index, inbuf);
                    height = height * 4;

                    if (temp == 15) {
                        getbits(16, &byte_index, &bit_index, inbuf);
                    }

                    break;
                default:
                    return 0;
                }
            }

            outbuf[outbuf_len++] = width >> 8;
            outbuf[outbuf_len++] = width;
            outbuf[outbuf_len++] = height >> 8;
            outbuf[outbuf_len++] = height;
            outbuf[outbuf_len++] = pict_type;
            outbuf[outbuf_len++] = rounding_type;

            if (optional_custom_PCF) {
                getbits(10, &byte_index, &bit_index, inbuf);
            }

            quant = getbits(5, &byte_index, &bit_index, inbuf);
        } else {
            switch (temp) {
            case 1:
                width = 128;
                height = 96;
                break;
            case 2:
                width = 176;
                height = 144;
                break;
            case 3:
                width = 352;
                height = 288;
                break;
            case 4:
                width = 704;
                height = 576;
                break;
            case 5:
                width = 1408;
                height = 1152;
                break;
            default:
                return 0;
            }

            outbuf[outbuf_len++] = width >> 8;
            outbuf[outbuf_len++] = width;
            outbuf[outbuf_len++] = height >> 8;
            outbuf[outbuf_len++] = height;

            pict_type = getbits(1, &byte_index, &bit_index, inbuf);
            outbuf[outbuf_len++] = pict_type;
            outbuf[outbuf_len++] = 0;

            temp = getbits(4, &byte_index, &bit_index, inbuf);
            if (temp != 0) {
                return 0;
            }

            quant = getbits(5, &byte_index, &bit_index, inbuf);

            temp = getbits(1, &byte_index, &bit_index, inbuf);
            if (temp != 0) {
                return 0;
            }
        }
    } else {
        temp = getbits(3, &byte_index, &bit_index, inbuf);

        switch (temp) {
        case 0:
            width = getbits(8, &byte_index, &bit_index, inbuf);
            height = getbits(8, &byte_index, &bit_index, inbuf);
            break;
        case 1:
            width = getbits(16, &byte_index, &bit_index, inbuf);
            height = getbits(16, &byte_index, &bit_index, inbuf);
            break;
        case 2:
            width = 352;
            height = 288;
            break;
        case 3:
            width = 176;
            height = 144;
            break;
        case 4:
            width = 128;
            height = 96;
            break;
        case 5:
            width = 320;
            height = 240;
            break;
        case 6:
            width = 160;
            height = 120;
            break;
        default:
            return 0;
        }

        outbuf[outbuf_len++] = width >> 8;
        outbuf[outbuf_len++] = width;
        outbuf[outbuf_len++] = height >> 8;
        outbuf[outbuf_len++] = height;

        pict_type = getbits(2, &byte_index, &bit_index, inbuf);
        if (pict_type == 3) {
            return 0;
        }

        outbuf[outbuf_len++] = pict_type;
        outbuf[outbuf_len++] = 0;

        if (pict_type == 2) {
            pict_type = 1;
        }

        getbits(1, &byte_index, &bit_index, inbuf);
        quant = getbits(5, &byte_index, &bit_index, inbuf);
    }

    temp = getbits(1, &byte_index, &bit_index, inbuf);
    while (temp == 1) {
        getbits(8, &byte_index, &bit_index, inbuf);
        temp = getbits(1, &byte_index, &bit_index, inbuf);
    }

    mb_width = (width + 15) / 16;
    mb_height = (height + 15) / 16;

    for (k = 0 ; k < mb_width * 2 ; k++)
        for (comp = 0 ; comp < 2 ; comp++) {
            top_mv[k][comp] = NO_VEC;
        }

    for (k = mb_width * 2 ; k < mb_width * 2 + 2 ; k++)
        for (comp = 0 ; comp < 2 ; comp++) {
            top_mv[k][comp] = 0;
        }

    for (i = 0 ; i < mb_height ; i++) {
        for (k = 0 ; k < 2 ; k++)
            for (comp = 0 ; comp < 2 ; comp++) {
                left_mv[k][comp] = 0;
            }

        for (j = 0 ; j < mb_width ; j++) {
            if (showbits(16, byte_index, bit_index, inbuf) == 0) {
                if (startcode(&byte_index, &bit_index, inbuf, inbuf_len) < 0) {
                    return 0;
                }

                getbits(17, &byte_index, &bit_index, inbuf);
                getbits(5, &byte_index, &bit_index, inbuf);
                getbits(2, &byte_index, &bit_index, inbuf);
                quant = getbits(5, &byte_index, &bit_index, inbuf);

                for (k = 0 ; k < mb_width * 2 ; k++)
                    for (comp = 0 ; comp < 2 ; comp++) {
                        top_mv[k][comp] = NO_VEC;
                    }

                for (k = mb_width * 2 ; k < mb_width * 2 + 2 ; k++)
                    for (comp = 0 ; comp < 2 ; comp++) {
                        top_mv[k][comp] = 0;
                    }

                for (k = 0 ; k < 2 ; k++)
                    for (comp = 0 ; comp < 2 ; comp++) {
                        left_mv[k][comp] = 0;
                    }
            }

            if (byte_index > inbuf_len || (byte_index == inbuf_len && bit_index < 7)) {
                return 0;
            }

            if (pict_type == H263_P_PICTURE) {
                if (getbits(1, &byte_index, &bit_index, inbuf) == 1) {
                    outbuf[outbuf_len++] = 0;
                    top_mv[j * 2][0] = top_mv[j * 2][1] = top_mv[j * 2 + 1][0] = top_mv[j * 2 + 1][1] = 0;
                    left_mv[0][0] = left_mv[0][1] = left_mv[1][0] = left_mv[1][1] = 0;
                    continue;
                }
            }

            if (pict_type == H263_I_PICTURE) {
                temp = showbits(9, byte_index, bit_index, inbuf);

                if (temp == 1) {
                    flushbits(9, &byte_index, &bit_index);
                    MCBPC = 255;
                }

                if (temp < 8) {
                    return 0;
                }

                temp >>= 3;

                if (temp >= 32) {
                    flushbits(1, &byte_index, &bit_index);
                    MCBPC = 3;
                } else {
                    flushbits(MCBPCtabintra[temp].len, &byte_index, &bit_index);
                    MCBPC = MCBPCtabintra[temp].val;
                }
            } else {
                temp = showbits(13, byte_index, bit_index, inbuf);

                if (temp >> 4 == 1) {
                    flushbits(9, &byte_index, &bit_index);
                    MCBPC = 255;
                }

                if (temp == 0) {
                    return 0;
                }

                if (temp >= 4096) {
                    flushbits(1, &byte_index, &bit_index);
                    MCBPC = 0;
                } else if (temp >= 16) {
                    flushbits(MCBPCtab0[temp >> 4].len, &byte_index, &bit_index);
                    MCBPC = MCBPCtab0[temp >> 4].val;
                } else {
                    flushbits(MCBPCtab1[temp - 8].len, &byte_index, &bit_index);
                    MCBPC = MCBPCtab1[temp - 8].val;
                }
            }

            if (MCBPC == 255) {
                j--;
                continue;
            }

            Mode = MCBPC & 7;
            temp = showbits(6, byte_index, bit_index, inbuf);

            if (temp < 2) {
                return 0;
            }

            if (temp >= 48) {
                flushbits(2, &byte_index, &bit_index);
                CBPY = 0;
            } else {
                flushbits(CBPYtab[temp].len, &byte_index, &bit_index);
                CBPY = CBPYtab[temp].val;
            }

            if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q) {
                CBPY = CBPY ^ 15;
            }

            CBP = (CBPY << 2) | (MCBPC >> 4);

            if (Mode == MODE_INTER_Q || Mode == MODE_INTRA_Q || Mode == MODE_INTER4V_Q) {
                DQUANT = getbits(2, &byte_index, &bit_index, inbuf);
                quant += DQ_tab[DQUANT];
            }

            if (quant > 31 || quant < 1) {
                return 0;
            }

            if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q) {
                outbuf[outbuf_len++] = 1;
                top_mv[j * 2][0] = top_mv[j * 2][1] = top_mv[j * 2 + 1][0] = top_mv[j * 2 + 1][1] = 0;
                left_mv[0][0] = left_mv[0][1] = left_mv[1][0] = left_mv[1][1] = 0;
            } else if (Mode == MODE_INTER || Mode == MODE_INTER_Q) {
                outbuf[outbuf_len++] = 2;

                for (comp = 0 ; comp < 2 ; comp++) {
                    pred = get_pred_mv(j, 0, comp);

                    if (getbits(1, &byte_index, &bit_index, inbuf)) {
                        mv[0][comp] = 0;
                    } else {
                        temp = showbits(12, byte_index, bit_index, inbuf);

                        if (temp >= 512) {
                            temp = (temp >> 8) - 2;
                            flushbits(TMNMVtab0[temp].len, &byte_index, &bit_index);
                            mv[0][comp] = TMNMVtab0[temp].val;
                        } else if (temp >= 128) {
                            temp = (temp >> 2) - 32;
                            flushbits(TMNMVtab1[temp].len, &byte_index, &bit_index);
                            mv[0][comp] = TMNMVtab1[temp].val;
                        } else {
                            temp -= 5;

                            if (temp < 0) {
                                return 0;
                            }

                            flushbits(TMNMVtab2[temp].len, &byte_index, &bit_index);
                            mv[0][comp] = TMNMVtab2[temp].val;
                        }
                    }

                    mv[0][comp] = motion_decode(mv[0][comp], pred);

                    outbuf[outbuf_len++] = (mv[0][comp] >> 7) & 0x3f;
                    outbuf[outbuf_len++] = mv[0][comp] << 1;
                }

                top_mv[j * 2][0] = top_mv[j * 2 + 1][0] = mv[0][0];
                top_mv[j * 2][1] = top_mv[j * 2 + 1][1] = mv[0][1];
                left_mv[0][0] = left_mv[1][0] = mv[0][0];
                left_mv[0][1] = left_mv[1][1] = mv[0][1];
            } else {
                int sum, dx, dy;

                outbuf[outbuf_len++] = 3;

                for (k = 0 ; k < 4 ; k++) {
                    for (comp = 0 ; comp < 2 ; comp++) {
                        pred = get_pred_mv(j, k, comp);

                        if (getbits(1, &byte_index, &bit_index, inbuf)) {
                            mv[k][comp] = 0;
                        } else {
                            temp = showbits(12, byte_index, bit_index, inbuf);

                            if (temp >= 512) {
                                temp = (temp >> 8) - 2;
                                flushbits(TMNMVtab0[temp].len, &byte_index, &bit_index);
                                mv[k][comp] = TMNMVtab0[temp].val;
                            } else if (temp >= 128) {
                                temp = (temp >> 2) - 32;
                                flushbits(TMNMVtab1[temp].len, &byte_index, &bit_index);
                                mv[k][comp] = TMNMVtab1[temp].val;
                            } else {
                                temp -= 5;

                                if (temp < 0) {
                                    return 0;
                                }

                                flushbits(TMNMVtab2[temp].len, &byte_index, &bit_index);
                                mv[k][comp] = TMNMVtab2[temp].val;
                            }
                        }

                        mv[k][comp] = motion_decode(mv[k][comp], pred);

                        outbuf[outbuf_len++] = (mv[k][comp] >> 7) & 0x3f;
                        outbuf[outbuf_len++] = mv[k][comp] << 1;
                    }
                }

                sum = mv[0][0] + mv[1][0] + mv[2][0] + mv[3][0];
                dx = msign(sum) * (roundtab[mabs(sum) % 16] + (mabs(sum) / 16) * 2);
                sum = mv[0][1] + mv[1][1] + mv[2][1] + mv[3][1];
                dy = msign(sum) * (roundtab[mabs(sum) % 16] + (mabs(sum) / 16) * 2);
                outbuf[outbuf_len++] = (dx >> 7) & 0x3f;
                outbuf[outbuf_len++] = dx << 1;
                outbuf[outbuf_len++] = (dy >> 7) & 0x3f;
                outbuf[outbuf_len++] = dy << 1;

                top_mv[j * 2][0] = mv[2][0];
                top_mv[j * 2][1] = mv[2][1];
                top_mv[j * 2 + 1][0] = mv[3][0];
                top_mv[j * 2 + 1][1] = mv[3][1];
                left_mv[0][0] = mv[1][0];
                left_mv[0][1] = mv[1][1];
                left_mv[1][0] = mv[3][0];
                left_mv[1][1] = mv[3][1];
            }

            for (comp = 0 ; comp < 6 ; comp++) {
                coeff_count = 0;
                last = 0;

                if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q) {
                    coeff_count = 1;
                    temp = getbits(8, &byte_index, &bit_index, inbuf);

                    if (temp == 128) {
                        return 0;
                    }

                    if (temp == 255) {
                        temp = 128;
                    }

                    temp *= 8;

                    outbuf[outbuf_len++] = (temp >> 8) & 0xf;
                    outbuf[outbuf_len++] = temp;
                }

                if ((CBP & (1 << (5 - comp)))) {
                    while (last == 0) {
                        temp = showbits(12, byte_index, bit_index, inbuf);

                        if (temp >= 512) {
                            tab = &DCT3Dtab0[(temp >> 5) - 16];
                        } else if (temp >= 128) {
                            tab = &DCT3Dtab1[(temp >> 2) - 32];
                        } else if (temp >= 8) {
                            tab = &DCT3Dtab2[(temp >> 0) - 8];
                        } else {
                            return 0;
                        }

                        flushbits(tab->len, &byte_index, &bit_index);

                        if (tab->val == ESCAPE) {
                            if (s263 == 1 && h263_version == 1) {
                                format_bit = getbits(1, &byte_index, &bit_index, inbuf);
                                last = getbits(1, &byte_index, &bit_index, inbuf);
                                run = getbits(6, &byte_index, &bit_index, inbuf);

                                if (format_bit) {
                                    level = getbits(11, &byte_index, &bit_index, inbuf);
                                    sign = (level >= 1024);

                                    if (sign) {
                                        val = 2048 - level;
                                    } else {
                                        val = level;
                                    }
                                } else {
                                    level = getbits(7, &byte_index, &bit_index, inbuf);
                                    sign = (level >= 64);

                                    if (sign) {
                                        val = 128 - level;
                                    } else {
                                        val = level;
                                    }
                                }
                            } else {
                                last = getbits(1, &byte_index, &bit_index, inbuf);
                                run = getbits(6, &byte_index, &bit_index, inbuf);
                                level = getbits(8, &byte_index, &bit_index, inbuf);
                                sign = (level >= 128);

                                if (sign) {
                                    val = 256 - level;
                                } else {
                                    val = level;
                                }
                            }
                        } else {
                            run = (tab->val >> 4) & 255;
                            level = tab->val & 15;
                            last = (tab->val >> 12) & 1;

                            val = level;
                            sign = getbits(1, &byte_index, &bit_index, inbuf);
                        }

                        coeff_count += run + 1;
                        if (coeff_count > 64) {
                            return 0;
                        }

                        if ((quant % 2) == 1) {
                            temp = (sign ? -(quant * (2 * val + 1)) : quant * (2 * val + 1));
                        } else {
                            temp = (sign ? -(quant * (2 * val + 1) - 1) : quant * (2 * val + 1) - 1);
                        }

                        if (run < 15) {
                            outbuf[outbuf_len++] = (run << 4) | ((temp >> 8) & 0xf);
                            outbuf[outbuf_len++] = temp;
                        } else {
                            outbuf[outbuf_len++] = 0xf0;
                            outbuf[outbuf_len++] = run;
                            outbuf[outbuf_len++] = (temp >> 8) & 0xf;
                            outbuf[outbuf_len++] = temp;
                        }

                        if (last) {
                            outbuf[outbuf_len++] = 0xf0;
                            outbuf[outbuf_len++] = 0;
                        }
                    }
                } else {
                    outbuf[outbuf_len++] = 0xf0;
                    outbuf[outbuf_len++] = 0;
                }
            }
        }
    }

    return outbuf_len;
}

int decodeble_h263(unsigned char *buf)
{
    if ((buf[3] & 0x3) != 0x2 || (buf[4] & 0xe0) != 0) {
        return 0;
    }

    if ((buf[4] & 0x1c) == 0x1c) {
        if ((buf[4] & 0x3) != 0) {
            return 0;
        }

        if ((buf[5] & 0x87) != 0x80) {
            return 0;
        }

        if (buf[6] != 1 || (buf[7] & 0xe3) != 0) {
            return 0;
        }
    } else {
        if ((buf[4] & 0x1) != 0 || (buf[5] & 0xe0) != 0) {
            return 0;
        }
    }

    return 1;
}
