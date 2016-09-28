/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_NDEBUG 1
#define LOG_TAG "Utils"
#include <utils/Log.h>

#include "ESDS.h"

#include <arpa/inet.h>

#include <foundation/ABuffer.h>
#include <foundation/ADebug.h>
#include <foundation/AMessage.h>
#include <stagefright/Utils.h>

namespace android {

uint16_t U16_AT(const uint8_t *ptr) {
    return ptr[0] << 8 | ptr[1];
}

uint32_t U32_AT(const uint8_t *ptr) {
    return ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3];
}

uint64_t U64_AT(const uint8_t *ptr) {
    return ((uint64_t)U32_AT(ptr)) << 32 | U32_AT(ptr + 4);
}

uint16_t U16LE_AT(const uint8_t *ptr) {
    return ptr[0] | (ptr[1] << 8);
}

uint32_t U32LE_AT(const uint8_t *ptr) {
    return ptr[3] << 24 | ptr[2] << 16 | ptr[1] << 8 | ptr[0];
}

uint64_t U64LE_AT(const uint8_t *ptr) {
    return ((uint64_t)U32LE_AT(ptr + 4)) << 32 | U32LE_AT(ptr);
}

// XXX warning: these won't work on big-endian host.
uint64_t ntoh64(uint64_t x) {
    return ((uint64_t)ntohl(x & 0xffffffff) << 32) | ntohl(x >> 32);
}

uint64_t hton64(uint64_t x) {
    return ((uint64_t)htonl(x & 0xffffffff) << 32) | htonl(x >> 32);
}

static size_t reassembleAVCC(const sp<ABuffer> &csd0, const sp<ABuffer> csd1, char *avcc) {

    avcc[0] = 1;        // version
    avcc[1] = 0x64;     // profile
    avcc[2] = 0;        // unused (?)
    avcc[3] = 0xd;      // level
    avcc[4] = 0xff;     // reserved+size

    size_t i = 0;
    int numparams = 0;
    int lastparamoffset = 0;
    int avccidx = 6;
    do {
        if (i >= csd0->size() - 4 ||
                memcmp(csd0->data() + i, "\x00\x00\x00\x01", 4) == 0) {
            if (i >= csd0->size() - 4) {
                // there can't be another param here, so use all the rest
                i = csd0->size();
            }
            //ALOGV("block at %d, last was %d", i, lastparamoffset);
            if (lastparamoffset > 0) {
                int size = i - lastparamoffset;
                avcc[avccidx++] = size >> 8;
                avcc[avccidx++] = size & 0xff;
                memcpy(avcc+avccidx, csd0->data() + lastparamoffset, size);
                avccidx += size;
                numparams++;
            }
            i += 4;
            lastparamoffset = i;
        } else {
            i++;
        }
    } while(i < csd0->size());
    //ALOGV("csd0 contains %d params", numparams);

    avcc[5] = 0xe0 | numparams;
    //and now csd-1
    i = 0;
    numparams = 0;
    lastparamoffset = 0;
    int numpicparamsoffset = avccidx;
    avccidx++;
    do {
        if (i >= csd1->size() - 4 ||
                memcmp(csd1->data() + i, "\x00\x00\x00\x01", 4) == 0) {
            if (i >= csd1->size() - 4) {
                // there can't be another param here, so use all the rest
                i = csd1->size();
            }
            //ALOGV("block at %d, last was %d", i, lastparamoffset);
            if (lastparamoffset > 0) {
                int size = i - lastparamoffset;
                avcc[avccidx++] = size >> 8;
                avcc[avccidx++] = size & 0xff;
                memcpy(avcc+avccidx, csd1->data() + lastparamoffset, size);
                avccidx += size;
                numparams++;
            }
            i += 4;
            lastparamoffset = i;
        } else {
            i++;
        }
    } while(i < csd1->size());
    avcc[numpicparamsoffset] = numparams;
    return avccidx;
}

static void reassembleESDS(const sp<ABuffer> &csd0, char *esds) {
    int csd0size = csd0->size();
    esds[0] = 3; // kTag_ESDescriptor;
    int esdescriptorsize = 26 + csd0size;
    //CHECK(esdescriptorsize < 268435456); // 7 bits per byte, so max is 2^28-1
    esds[1] = 0x80 | (esdescriptorsize >> 21);
    esds[2] = 0x80 | ((esdescriptorsize >> 14) & 0x7f);
    esds[3] = 0x80 | ((esdescriptorsize >> 7) & 0x7f);
    esds[4] = (esdescriptorsize & 0x7f);
    esds[5] = esds[6] = 0; // es id
    esds[7] = 0; // flags
    esds[8] = 4; // kTag_DecoderConfigDescriptor
    int configdescriptorsize = 18 + csd0size;
    esds[9] = 0x80 | (configdescriptorsize >> 21);
    esds[10] = 0x80 | ((configdescriptorsize >> 14) & 0x7f);
    esds[11] = 0x80 | ((configdescriptorsize >> 7) & 0x7f);
    esds[12] = (configdescriptorsize & 0x7f);
    esds[13] = 0x40; // objectTypeIndication
    esds[14] = 0x15; // not sure what 14-25 mean, they are ignored by ESDS.cpp,
    esds[15] = 0x00; // but the actual values here were taken from a real file.
    esds[16] = 0x18;
    esds[17] = 0x00;
    esds[18] = 0x00;
    esds[19] = 0x00;
    esds[20] = 0xfa;
    esds[21] = 0x00;
    esds[22] = 0x00;
    esds[23] = 0x00;
    esds[24] = 0xfa;
    esds[25] = 0x00;
    esds[26] = 5; // kTag_DecoderSpecificInfo;
    esds[27] = 0x80 | (csd0size >> 21);
    esds[28] = 0x80 | ((csd0size >> 14) & 0x7f);
    esds[29] = 0x80 | ((csd0size >> 7) & 0x7f);
    esds[30] = (csd0size & 0x7f);
    memcpy((void*)&esds[31], csd0->data(), csd0size);
    // data following this is ignored, so don't bother appending it

}

}  // namespace android

