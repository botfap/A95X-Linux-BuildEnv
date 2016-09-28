/*
 * Copyright (c) 2003 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * ID3v2 header parser
 *
 * Specifications available at:
 * http://id3.org/Developer_Information
 */

#include "id3v2.h"
#include "id3v1.h"
#include "libavutil/avstring.h"
#include "libavutil/intreadwrite.h"
#include "libavutil/dict.h"
#include "avio_internal.h"

int ff_id3v2_match(const uint8_t *buf, const char * magic)
{
    return  buf[0]         == magic[0] &&
            buf[1]         == magic[1] &&
            buf[2]         == magic[2] &&
            buf[3]         != 0xff &&
            buf[4]         != 0xff &&
           (buf[6] & 0x80) ==    0 &&
           (buf[7] & 0x80) ==    0 &&
           (buf[8] & 0x80) ==    0 &&
           (buf[9] & 0x80) ==    0;
}

int ff_id3v2_tag_len(const uint8_t * buf)
{
    int len = ((buf[6] & 0x7f) << 21) +
              ((buf[7] & 0x7f) << 14) +
              ((buf[8] & 0x7f) << 7) +
               (buf[9] & 0x7f) +
              ID3v2_HEADER_SIZE;
    if (buf[5] & 0x10)
        len += ID3v2_HEADER_SIZE;
    return len;
}

static unsigned int get_size(AVIOContext *s, int len)
{
    int v = 0;
    while (len--)
        v = (v << 7) + (avio_r8(s) & 0x7F);
    return v;
}

static void read_ttag(AVFormatContext *s, AVIOContext *pb, int taglen, const char *key)
{
    char *q, dst[512];
    const char *val = NULL;
    int len, dstlen = sizeof(dst) - 1;
    unsigned genre;
    unsigned int (*get)(AVIOContext*) = avio_rb16;

    dst[0] = 0;
    if (taglen < 1)
        return;

    taglen--; /* account for encoding type byte */

    switch (avio_r8(pb)) { /* encoding type */

    case ID3v2_ENCODING_ISO8859:
        q = dst;
        while (taglen-- && q - dst < dstlen - 7) {
            uint8_t tmp;
            PUT_UTF8(avio_r8(pb), tmp, *q++ = tmp;)
        }
        *q = 0;
        break;

    case ID3v2_ENCODING_UTF16BOM:
        taglen -= 2;
        switch (avio_rb16(pb)) {
        case 0xfffe:
            get = avio_rl16;
        case 0xfeff:
            break;
        default:
            av_log(s, AV_LOG_ERROR, "Incorrect BOM value in tag %s.\n", key);
            return;
        }
        // fall-through

    case ID3v2_ENCODING_UTF16BE:
        q = dst;
        while (taglen > 1 && q - dst < dstlen - 7) {
            uint32_t ch;
            uint8_t tmp;

            GET_UTF16(ch, ((taglen -= 2) >= 0 ? get(pb) : 0), break;)
            PUT_UTF8(ch, tmp, *q++ = tmp;)
        }
        *q = 0;
        break;

    case ID3v2_ENCODING_UTF8:
        len = FFMIN(taglen, dstlen);
        avio_read(pb, dst, len);
        dst[len] = 0;
        break;
    default:
        av_log(s, AV_LOG_WARNING, "Unknown encoding in tag %s.\n", key);
    }

    if (!(strcmp(key, "TCON") && strcmp(key, "TCO"))
        && (sscanf(dst, "(%d)", &genre) == 1 || sscanf(dst, "%d", &genre) == 1)
        && genre <= ID3v1_GENRE_MAX)
        val = ff_id3v1_genre_str[genre];
    else if (!(strcmp(key, "TXXX") && strcmp(key, "TXX"))) {
        /* dst now contains two 0-terminated strings */
        dst[dstlen] = 0;
        len = strlen(dst);
        key = dst;
        val = dst + FFMIN(len + 1, dstlen);
    }
    else if (*dst)
        val = dst;

    if (val)
        av_dict_set(&s->metadata, key, val, AV_DICT_DONT_OVERWRITE);
}

static int is_number(const char *str)
{
    while (*str >= '0' && *str <= '9') str++;
    return !*str;
}

static AVDictionaryEntry* get_date_tag(AVDictionary *m, const char *tag)
{
    AVDictionaryEntry *t;
    if ((t = av_dict_get(m, tag, NULL, AV_DICT_MATCH_CASE)) &&
        strlen(t->value) == 4 && is_number(t->value))
        return t;
    return NULL;
}

static void merge_date(AVDictionary **m)
{
    AVDictionaryEntry *t;
    char date[17] = {0};      // YYYY-MM-DD hh:mm

    if (!(t = get_date_tag(*m, "TYER")) &&
        !(t = get_date_tag(*m, "TYE")))
        return;
    av_strlcpy(date, t->value, 5);
    av_dict_set(m, "TYER", NULL, 0);
    av_dict_set(m, "TYE",  NULL, 0);

    if (!(t = get_date_tag(*m, "TDAT")) &&
        !(t = get_date_tag(*m, "TDA")))
        goto finish;
    snprintf(date + 4, sizeof(date) - 4, "-%.2s-%.2s", t->value + 2, t->value);
    av_dict_set(m, "TDAT", NULL, 0);
    av_dict_set(m, "TDA",  NULL, 0);

    if (!(t = get_date_tag(*m, "TIME")) &&
        !(t = get_date_tag(*m, "TIM")))
        goto finish;
    snprintf(date + 10, sizeof(date) - 10, " %.2s:%.2s", t->value, t->value + 2);
    av_dict_set(m, "TIME", NULL, 0);
    av_dict_set(m, "TIM",  NULL, 0);

finish:
    if (date[0])
        av_dict_set(m, "date", date, 0);
}

static int parse_apic_tag(AVFormatContext *s, AVIOContext *pb, int taglen, const char *key, int flag)
{
    int cover_len = taglen;
    char mime[32];
    char dscrp[512];
    int ret;
	
    if(s->cover_data){
        av_log(s, AV_LOG_INFO, "Has parsed APIC tag!\n");
        return 0;
    }

    if(flag) {
        avio_r8(pb);  // Text encoding
        cover_len --;
        ret = avio_get_str(pb, cover_len, mime, sizeof(mime));  // MIME Type
        cover_len -= ret;
        avio_r8(pb);  // Picture Type
        cover_len --;
        ret = avio_get_str(pb, cover_len, dscrp, sizeof(dscrp));  // Description
        cover_len -= ret;
    } else {
        avio_r8(pb);  // Text encoding
        cover_len --;
        ret = avio_get_str(pb, 3, mime, sizeof(mime));  // MIME Type
        cover_len -= 3;
        avio_r8(pb);  // Picture Type
        cover_len --;
        ret = avio_get_str(pb, cover_len, dscrp, sizeof(dscrp));  // Description
        cover_len -= ret;
    }
	
    s->cover_data = av_malloc(cover_len);
    if(!s->cover_data){
        av_log(s, AV_LOG_INFO, "no memery, av_alloc failed!\n");
	 return -1;
    }
    if(!strcmp(mime,"image/jpeg")) {
            av_log(NULL, AV_LOG_INFO, "cover is image/jpeg, first byte must be 0xff!\n");
            do{
                ret = avio_r8(pb);
                cover_len --;           
            }while(ret!=0xff& cover_len > 0);
            avio_seek(pb, -1, SEEK_CUR);
            cover_len ++;
            av_log(NULL, AV_LOG_INFO, "[%s:%d]cover data offset=%llx ret=%x\n",  __FUNCTION__, __LINE__, avio_tell(pb));
    }
    s->cover_data_len = cover_len;
    avio_read(pb, s->cover_data, cover_len);

    av_dict_set(&s->metadata, "cover_pic", mime, 0);

    return 0;
}
typedef struct ID3v2EMFunc {
    const char *tag3;
    const char *tag4;
    void (*read)(AVFormatContext *, AVIOContext *, int, char *,
                 ID3v2ExtraMeta **);
    void (*free)(void *obj);
} ID3v2EMFunc;

static const ID3v2EMFunc id3v2_extra_meta_funcs[] = {
    { "GEO", "GEOB", NULL, NULL },
    { "PIC", "APIC", NULL,    NULL    },
    { "CHAP","CHAP", NULL, NULL         },
    { NULL }
};

/**
 * Get the corresponding ID3v2EMFunc struct for a tag.
 * @param isv34 Determines if v2.2 or v2.3/4 strings are used
 * @return A pointer to the ID3v2EMFunc struct if found, NULL otherwise.
 */
/**
 * Get the corresponding ID3v2EMFunc struct for a tag.
 * @param isv34 Determines if v2.2 or v2.3/4 strings are used
 * @return A pointer to the ID3v2EMFunc struct if found, NULL otherwise.
 */
static int/*const ID3v2EMFunc **/get_extra_meta_func(const char *tag, int isv34)
{
    int i = 0;
    while (id3v2_extra_meta_funcs[i].tag3) {
        if (tag && !memcmp(tag,
                    (isv34 ? id3v2_extra_meta_funcs[i].tag4 :
                             id3v2_extra_meta_funcs[i].tag3),
                    (isv34 ? 4 : 3)))
            return 1/*&id3v2_extra_meta_funcs[i]*/;
        i++;
    }
    return 0/*NULL*/;
}
static void ff_id3v2_parse(AVFormatContext *s, int len, uint8_t version, uint8_t flags)
{
    int isv34, unsync;
    unsigned tlen;
    char tag[5];
    int64_t next, end = avio_tell(s->pb) + len;
    int taghdrlen;
    const char *reason = NULL;
    AVIOContext pb;
    AVIOContext *pbx;
    unsigned char *buffer = NULL;
    int buffer_size       = 0;
    const ID3v2EMFunc *extra_func = NULL;

    av_log(s, AV_LOG_DEBUG, "id3v2 ver:%d flags:%02X len:%d\n", version, flags, len);

    switch (version) {
    case 2:
        if (flags & 0x40) {
            reason = "compression";
            goto error;
        }
        isv34     = 0;
        taghdrlen = 6;
        break;

    case 3:
    case 4:
        isv34     = 1;
        taghdrlen = 10;
        break;

    default:
        reason = "version";
        goto error;
    }

    unsync = flags & 0x80;

    if (isv34 && flags & 0x40) { /* Extended header present, just skip over it */
        int extlen = get_size(s->pb, 4);
        if (version == 4)
            /* In v2.4 the length includes the length field we just read. */
            extlen -= 4;

        if (extlen < 0) {
            reason = "invalid extended header length";
            goto error;
        }
        avio_skip(s->pb, extlen);
        len -= extlen + 4;
        if (len < 0) {
            reason = "extended header too long.";
            goto error;
        }
    }

    while (len >= taghdrlen) {
        unsigned int tflags = 0;
        int tunsync         = 0;
        int tcomp           = 0;
        int tencr           = 0;
        unsigned long dlen;

        if (isv34) {
            avio_read(s->pb, tag, 4);
            tag[4] = 0;
            if (version == 3) {
                tlen = avio_rb32(s->pb);
            } else
                tlen = get_size(s->pb, 4);
            tflags  = avio_rb16(s->pb);
            tunsync = tflags & ID3v2_FLAG_UNSYNCH;
        } else {
            avio_read(s->pb, tag, 3);
            tag[3] = 0;
            tlen   = avio_rb24(s->pb);
        }
        if (tlen > (1<<28))
            break;
        len -= taghdrlen + tlen;

        if (len < 0)
            break;

        next = avio_tell(s->pb) + tlen;

        if (!tlen) {
            if (tag[0])
                av_log(s, AV_LOG_DEBUG, "Invalid empty frame %s, skipping.\n",
                       tag);
            continue;
        }

        if (tflags & ID3v2_FLAG_DATALEN) {
            if (tlen < 4)
                break;
            dlen = avio_rb32(s->pb);
            tlen -= 4;
        } else
            dlen = tlen;

        tcomp = tflags & ID3v2_FLAG_COMPRESSION;
        tencr = tflags & ID3v2_FLAG_ENCRYPTION;

        /* skip encrypted tags and, if no zlib, compressed tags */
        if (tencr || (!CONFIG_ZLIB && tcomp)) {
            const char *type;
            if (!tcomp)
                type = "encrypted";
            else if (!tencr)
                type = "compressed";
            else
                type = "encrypted and compressed";

            av_log(s, AV_LOG_WARNING, "Skipping %s ID3v2 frame %s.\n", type, tag);
            avio_skip(s->pb, tlen);
        /* check for text tag or supported special meta tag */
        } else if (tag[0] == 'T' ||
                   (/*extra_meta &&*/
                    (/*extra_func = */get_extra_meta_func(tag, isv34)))) {
            pbx = s->pb;

            if (unsync || tunsync || tcomp) {
                av_fast_malloc(&buffer, &buffer_size, tlen);
                if (!buffer) {
                    av_log(s, AV_LOG_ERROR, "Failed to alloc %d bytes\n", tlen);
                    goto seek;
                }
            }
            if (unsync || tunsync) {
                int64_t end = avio_tell(s->pb) + tlen;
                uint8_t *b;

                b = buffer;
                while (avio_tell(s->pb) < end && b - buffer < tlen && !s->pb->eof_reached) {
                    *b++ = avio_r8(s->pb);
                    if (*(b - 1) == 0xff && avio_tell(s->pb) < end - 1 &&
                        b - buffer < tlen &&
                        !s->pb->eof_reached ) {
                        uint8_t val = avio_r8(s->pb);
                        *b++ = val ? val : avio_r8(s->pb);
                    }
                }
                ffio_init_context(&pb, buffer, b - buffer, 0, NULL, NULL, NULL,
                                  NULL);
                tlen = b - buffer;
                pbx  = &pb; // read from sync buffer
            }

            if (tag[0] == 'T')
                /* parse text tag */
                read_ttag(s, pbx, tlen, tag);
        	else if(tag[0] == 'A' && tag[1] == 'P' && tag[2] == 'I' && tag[3] == 'C') {
	     		parse_apic_tag(s, pbx, tlen, tag, isv34);
	 	}
	 	else if(tag[0] == 'P' && tag[1] == 'I' && tag[2] == 'C') {
	     		parse_apic_tag(s, pbx, tlen, tag, isv34);
	 	}			
			
        } else if (!tag[0]) {
            if (tag[1])
                av_log(s, AV_LOG_WARNING, "invalid frame id, assuming padding\n");
            avio_skip(s->pb, tlen);
            break;
        }
        /* Skip to end of tag */
seek:
        avio_seek(s->pb, next, SEEK_SET);
    }

    /* Footer preset, always 10 bytes, skip over it */
    if (version == 4 && flags & 0x10)
        end += 10;

error:
    if (reason)
        av_log(s, AV_LOG_INFO, "ID3v2.%d tag skipped, cannot handle %s\n",
               version, reason);
    avio_seek(s->pb, end, SEEK_SET);
    av_free(buffer);
    return;
}

void ff_id3v2_read(AVFormatContext *s, const char *magic)
{
    int len, ret;
    uint8_t buf[ID3v2_HEADER_SIZE];
    int     found_header;
    int64_t off;

    do {
        /* save the current offset in case there's nothing to read/skip */
        off = avio_tell(s->pb);
        ret = avio_read(s->pb, buf, ID3v2_HEADER_SIZE);
        if (ret != ID3v2_HEADER_SIZE)
            break;
            found_header = ff_id3v2_match(buf, magic);
            if (found_header) {
            /* parse ID3v2 header */
            len = ((buf[6] & 0x7f) << 21) |
                  ((buf[7] & 0x7f) << 14) |
                  ((buf[8] & 0x7f) << 7) |
                   (buf[9] & 0x7f);
            ff_id3v2_parse(s, len, buf[3], buf[5]);
        } else {
            avio_seek(s->pb, off, SEEK_SET);
        }
    } while (found_header);
    ff_metadata_conv(&s->metadata, NULL, ff_id3v2_34_metadata_conv);
    ff_metadata_conv(&s->metadata, NULL, ff_id3v2_2_metadata_conv);
    ff_metadata_conv(&s->metadata, NULL, ff_id3v2_4_metadata_conv);
    merge_date(&s->metadata);
}

const AVMetadataConv ff_id3v2_34_metadata_conv[] = {
    { "TALB", "album"},
    { "TCOM", "composer"},
    { "TCON", "genre"},
    { "TCOP", "copyright"},
    { "TENC", "encoded_by"},
    { "TIT2", "title"},
    { "TLAN", "language"},
    { "TPE1", "artist"},
    { "TPE2", "album_artist"},
    { "TPE3", "performer"},
    { "TPOS", "disc"},
    { "TPUB", "publisher"},
    { "TRCK", "track"},
    { "TSSE", "encoder"},
    { 0 }
};

const AVMetadataConv ff_id3v2_4_metadata_conv[] = {
    { "TDRL", "date"},
    { "TDRC", "date"},
    { "TDEN", "creation_time"},
    { "TSOA", "album-sort"},
    { "TSOP", "artist-sort"},
    { "TSOT", "title-sort"},
    { 0 }
};

const AVMetadataConv ff_id3v2_2_metadata_conv[] = {
    { "TAL",  "album"},
    { "TCO",  "genre"},
    { "TT2",  "title"},
    { "TEN",  "encoded_by"},
    { "TP1",  "artist"},
    { "TP2",  "album_artist"},
    { "TP3",  "performer"},
    { "TRK",  "track"},
    { 0 }
};


const char ff_id3v2_tags[][4] = {
   "TALB", "TBPM", "TCOM", "TCON", "TCOP", "TDLY", "TENC", "TEXT",
   "TFLT", "TIT1", "TIT2", "TIT3", "TKEY", "TLAN", "TLEN", "TMED",
   "TOAL", "TOFN", "TOLY", "TOPE", "TOWN", "TPE1", "TPE2", "TPE3",
   "TPE4", "TPOS", "TPUB", "TRCK", "TRSN", "TRSO", "TSRC", "TSSE",
   { 0 },
};

const char ff_id3v2_4_tags[][4] = {
   "TDEN", "TDOR", "TDRC", "TDRL", "TDTG", "TIPL", "TMCL", "TMOO",
   "TPRO", "TSOA", "TSOP", "TSOT", "TSST",
   { 0 },
};

const char ff_id3v2_3_tags[][4] = {
   "TDAT", "TIME", "TORY", "TRDA", "TSIZ", "TYER",
   { 0 },
};
