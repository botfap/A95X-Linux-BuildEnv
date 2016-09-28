/*******************************************************************
 *
 *  Copyright C 2005 by Amlogic, Inc. All Rights Reserved.
 *
 *  Description:
 *
 *  Author: Amlogic Software
 *  Created: 12/22/2005 9:13PM
 *
 *******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#include "list.h"
#include "player_sub.h"
#include "player_priv.h"

#define DUMP_SUB

static float mpsub_position = 0;
static int sub_slacktime = 20000; //20 sec

static int sub_no_text_pp = 0; // 1 => do not apply text post-processing
// like {\...} elimination in SSA format.

/* read one line of string from data source */
static char *subfile_buffer = NULL;
static int buffer_size;
static unsigned subfile_read;

static char *internal_subf_gets(char *s, int fd)
{
    int offset = subfile_read;
    int copied;

    if (!subfile_buffer) {
        subfile_buffer = (char *)MALLOC(LINE_LEN);

        if (subfile_buffer) {
            subfile_read = 0;
            offset = 0;

            buffer_size = read(fd, subfile_buffer, LINE_LEN);

            if (buffer_size <= 0) {
                return NULL;
            }
        } else {
            log_error("no enough memory!\n");
            return NULL;
        }
    }

    while ((offset < buffer_size) && (subfile_buffer[offset] != '\n')) {
        offset++;
    }

    if (offset < buffer_size) {
        MEMCPY(s, subfile_buffer + subfile_read, offset - subfile_read);
        s[offset - subfile_read ] = '\0';

        subfile_read = offset + 1;
        if (subfile_read == LINE_LEN) {
            subfile_read = 0;

            buffer_size = read(fd, subfile_buffer, LINE_LEN);

        }
    } else {
        if (buffer_size < LINE_LEN) {
            /* end of file w/o terminator "\n" */
            return NULL;
        }

        copied = LINE_LEN - subfile_read;

        MEMCPY(s, subfile_buffer + subfile_read, copied);

        if (buffer_size == LINE_LEN) {
            /* refill */
            buffer_size = read(fd, subfile_buffer, LINE_LEN);

            if (buffer_size < 0) {
                return NULL;
            }
        }

        offset = 0;
        while ((offset < MIN(LINE_LEN - copied, buffer_size)) && (subfile_buffer[offset] != '\n')) {
            offset++;
        }

        if (subfile_buffer[offset] == '\n') {
            subfile_read = offset + 1;
            if (offset) {
                memcpy(s + copied, subfile_buffer, offset);
            }
            s[copied + offset + 1] = '\0';
        } else if (buffer_size < LINE_LEN) {
            /* end of file w/o terminator "\n" */
            return NULL;
        } else {
            s[LINE_LEN + 1] = '\0';
        }
    }

    return s;
}

/* internal string manipulation functions */
static int internal_eol(char p)
{
    return (p == '\r' || p == '\n' || p == '\0');
}

static void internal_trail_space(char *s)
{
    int i = 0;
    while (isspace(s[i])) {
        ++i;
    }
    if (i) {
        strcpy(s, s + i);
    }
    i = strlen(s) - 1;
    while (i > 0 && isspace(s[i])) {
        s[i--] = '\0';
    }
}

static char *internal_stristr(const char *haystack, const char *needle)
{
    int len = 0;
    const char *p = haystack;

    if (!(haystack && needle)) {
        return NULL;
    }

    len = strlen(needle);
    while (*p != '\0') {
        if (strncasecmp(p, needle, len) == 0) {
            return (char*)p;
        }
        p++;
    }
    return NULL;
}

static char *internal_sub_readtext(char *source, char **dest)
{
    int len = 0;
    char *p = source;

    while (!internal_eol(*p) && *p != '|') {
        p++, len++;
    }
    if (*dest == NULL) {
        *dest = (char *)MALLOC(len + 1);
    }
    if (*dest) {
        return ERR;
    }

    strncpy(*dest, source, len);

    (*dest)[len] = 0;

    while (*p == '\r' || *p == '\n' || *p == '|') {
        p++;
    }

    if (*p) {
        /* not-last text field */
        return p;
    } else {
        /* last text field */
        return NULL;
    }
}

static char *internal_sub_strdup(char *src)
{
    char *ret;
    int len;

    len = strlen(src);
    ret = (char *)MALLOC(len + 1);

    if (ret) {
        strcpy(ret, src);
    }

    return ret;
}

/* subtitle file read line functions */
static subtitle_t *internal_sub_read_line_sami(int fd, subtitle_t *current)
{
    static char line[LINE_LEN + 1];
    static char *s = NULL, *slacktime_s;
    char text[LINE_LEN + 1], *p = NULL, *q;
    int state;

    current->text.lines = current->start = current->end = 0;
    current->text.alignment = SUB_ALIGNMENT_BOTTOMCENTER;
    state = 0;

    /* read the first line */
    if (!s) {
        s = internal_subf_gets(line, fd);
        if (!s) {
            return 0;
        }
    }

    do {
        switch (state) {
        case 0: /* find "START=" or "Slacktime:" */
            slacktime_s = internal_stristr(s, "Slacktime:");
            if (slacktime_s) {
                sub_slacktime = strtol(slacktime_s + 10, NULL, 0) / 10;
            }

            s = internal_stristr(s, "Start=");
            if (s) {
                current->start = strtol(s + 6, &s, 0) / 10;
                /* eat '>' */
                for (; *s != '>' && *s != '\0'; s++) {
                    ;
                }
                s++;
                state = 1;
                continue;
            }
            break;

        case 1: /* find (optionnal) "<P", skip other TAGs */
            for (; *s == ' ' || *s == '\t'; s++) {
                ;    /* strip blanks, if any */
            }
            if (*s == '\0') {
                break;
            }
            if (*s != '<') {
                state = 3;
                p = text;
                continue;
            } /* not a TAG */
            s++;
            if (*s == 'P' || *s == 'p') {
                s++;
                state = 2;
                continue;
            } /* found '<P' */
            for (; *s != '>' && *s != '\0'; s++) {
                ;    /* skip remains of non-<P> TAG */
            }
            if (s == '\0') {
                break;
            }
            s++;
            continue;

        case 2: /* find ">" */
            s = strchr(s, '>');
            if (s) {
                s++;
                state = 3;
                p = text;
                continue;
            }
            break;

        case 3: /* get all text until '<' appears */
            if (*s == '\0') {
                break;
            } else if (!strncasecmp(s, "<br>", 4)) {
                *p = '\0';
                p = text;
                internal_trail_space(text);
                if (text[0] != '\0') {
                    current->text.text[current->text.lines++] = internal_sub_strdup(text);
                }
                s += 4;
            } else if ((*s == '{') && !sub_no_text_pp) {
                state = 5;
                ++s;
                continue;
            } else if (*s == '<') {
                state = 4;
            } else if (!strncasecmp(s, "&nbsp;", 6)) {
                *p++ = ' ';
                s += 6;
            } else if (*s == '\t') {
                *p++ = ' ';
                s++;
            } else if (*s == '\r' || *s == '\n') {
                s++;
            } else {
                *p++ = *s++;
            }

            /* skip duplicated space */
            if (p > text + 2) if (*(p - 1) == ' ' && *(p - 2) == ' ') {
                    p--;
                }

            continue;

        case 4: /* get current->end or skip <TAG> */
            q = internal_stristr(s, "Start=");
            if (q) {
                current->end = strtol(q + 6, &q, 0) / 10 - 1;
                *p = '\0';
                internal_trail_space(text);
                if (text[0] != '\0') {
                    current->text.text[current->text.lines++] = internal_sub_strdup(text);
                }
                if (current->text.lines > 0) {
                    state = 99;
                    break;
                }
                state = 0;
                continue;
            }
            s = strchr(s, '>');
            if (s) {
                s++;
                state = 3;
                continue;
            }
            break;

        case 5: /* get rid of {...} text, but read the alignment code */
            if ((*s == '\\') && (*(s + 1) == 'a') && !sub_no_text_pp) {
                if (internal_stristr(s, "\\a1") != NULL) {
                    current->text.alignment = SUB_ALIGNMENT_BOTTOMLEFT;
                    s = s + 3;
                }
                if (internal_stristr(s, "\\a2") != NULL) {
                    current->text.alignment = SUB_ALIGNMENT_BOTTOMCENTER;
                    s = s + 3;
                } else if (internal_stristr(s, "\\a3") != NULL) {
                    current->text.alignment = SUB_ALIGNMENT_BOTTOMRIGHT;
                    s = s + 3;
                } else if ((internal_stristr(s, "\\a4") != NULL) || (internal_stristr(s, "\\a5") != NULL) || (internal_stristr(s, "\\a8") != NULL)) {
                    current->text.alignment = SUB_ALIGNMENT_TOPLEFT;
                    s = s + 3;
                } else if (internal_stristr(s, "\\a6") != NULL) {
                    current->text.alignment = SUB_ALIGNMENT_TOPCENTER;
                    s = s + 3;
                } else if (internal_stristr(s, "\\a7") != NULL) {
                    current->text.alignment = SUB_ALIGNMENT_TOPRIGHT;
                    s = s + 3;
                } else if (internal_stristr(s, "\\a9") != NULL) {
                    current->text.alignment = SUB_ALIGNMENT_MIDDLELEFT;
                    s = s + 3;
                } else if (internal_stristr(s, "\\a10") != NULL) {
                    current->text.alignment = SUB_ALIGNMENT_MIDDLECENTER;
                    s = s + 4;
                } else if (internal_stristr(s, "\\a11") != NULL) {
                    current->text.alignment = SUB_ALIGNMENT_MIDDLERIGHT;
                    s = s + 4;
                }
            }
            if (*s == '}') {
                state = 3;
            }
            ++s;
            continue;
        }

        /* read next line */
        s = internal_subf_gets(line, fd);
        if (state != 99 && !s) {
            if (current->start > 0) {
                break; // if it is the last subtitle
            } else {
                return 0;
            }
        }

    } while (state != 99);

    /* For the last subtitle */
    if (current->end <= 0) {
        current->end = current->start + sub_slacktime;
        *p = '\0';
        internal_trail_space(text);
        if (text[0] != '\0') {
            current->text.text[current->text.lines++] = internal_sub_strdup(text);
        }
    }

    return current;
}

subtitle_t *internal_sub_read_line_microdvd(int fd, subtitle_t *current)
{
    char line[LINE_LEN + 1];
    char line2[LINE_LEN + 1];
    char *p, *next;
    int i;
    float ptsrate = (float)current->end / 960;
    current->end = 0;

    do {
        if (!internal_subf_gets(line, fd)) {
            return NULL;
        }
    } while ((sscanf(line, "{%ld}{}%[^\r\n]", &(current->start), line2) < 2) &&
             (sscanf(line, "{%ld}{%ld}%[^\r\n]", &(current->start), &(current->end), line2) < 3));

    p = line2;
    next = p, i = 0;
    while (1) {
        next = internal_sub_readtext(next, &(current->text.text[i]));
        if (!next) {
            break;
        }
        if (current->text.text[i] == ERR) {
            return ERR;
        }
        i++;
        if (i >= SUB_MAX_TEXT) {
            log_print(("Too many lines in a subtitle\n"));
            current->text.lines = i;
            return current;
        }
    }

    current->text.lines = ++i;
    current->start = current->start * ptsrate;
    current->end = current->end * ptsrate;

    return current;
}

subtitle_t *internal_sub_read_line_mpl2(int fd, subtitle_t *current)
{
    char line[LINE_LEN + 1];
    char line2[LINE_LEN + 1];
    char *p, *next;
    int i;

    do {
        if (!internal_subf_gets(line, fd)) {
            return NULL;
        }
    } while ((sscanf(line, "[%ld][%ld]%[^\r\n]", &(current->start), &(current->end), line2) < 3));

    current->start *= 10;
    current->end *= 10;

    p = line2;
    next = p, i = 0;
    while (1) {
        next = internal_sub_readtext(next, &(current->text.text[i]));
        if (!next) {
            break;
        }
        if (current->text.text[i] == ERR) {
            return ERR;
        }
        i++;
        if (i >= SUB_MAX_TEXT) {
            log_print(("Too many lines in a subtitle\n"));
            current->text.lines = i;
            return current;
        }
    }
    current->text.lines = ++i;

    return current;
}

subtitle_t *internal_sub_read_line_subrip(int fd, subtitle_t *current)
{
    char line[LINE_LEN + 1];
    int a1, a2, a3, a4, b1, b2, b3, b4;
    char *p = NULL, *q = NULL;
    int len;

    while (1) {
        if (!internal_subf_gets(line, fd)) {
            return NULL;
        }
        if (sscanf(line, "%d:%d:%d.%d,%d:%d:%d.%d",
                   &a1, &a2, &a3, &a4, &b1, &b2, &b3, &b4) < 8) {
            continue;
        }
        current->start = a1 * 360000 + a2 * 6000 + a3 * 100 + a4;
        current->end   = b1 * 360000 + b2 * 6000 + b3 * 100 + b4;

        if (!internal_subf_gets(line, fd)) {
            return NULL;
        }

        p = q = line;
        for (current->text.lines = 1; current->text.lines < SUB_MAX_TEXT; current->text.lines++) {
            for (q = p, len = 0; *p && *p != '\r' && *p != '\n' && *p != '|' && strncmp(p, "[br]", 4); p++, len++) {
                ;
            }

            current->text.text[current->text.lines - 1] = (char *)MALLOC(len + 1);
            if (!current->text.text[current->text.lines - 1]) {
                return ERR;
            }
            strncpy(current->text.text[current->text.lines - 1], q, len);
            current->text.text[current->text.lines - 1][len] = '\0';

            if (!*p || *p == '\r' || *p == '\n') {
                break;
            }
            if (*p == '|') {
                p++;
            } else {
                while (*p++ != ']') {
                    ;
                }
            }
        }
        break;
    }

    return current;
}

subtitle_t *internal_sub_read_line_subviewer(int fd, subtitle_t *current)
{
    char line[LINE_LEN + 1];
    int a1, a2, a3, a4, b1, b2, b3, b4;
    char *p = NULL;
    int i, len;

    while (!current->text.text[0]) {
        if (!internal_subf_gets(line, fd)) {
            return NULL;
        }
        if ((len = sscanf(line,
                          "%d:%d:%d%[,.:]%d --> %d:%d:%d%[,.:]%d",
                          &a1, &a2, &a3, (char *)&i, &a4, &b1, &b2, &b3,
                          (char *)&i, &b4)) < 10) {
            continue;
        }

        current->start = a1 * 360000 + a2 * 6000 + a3 * 100 + a4 / 10;
        current->end   = b1 * 360000 + b2 * 6000 + b3 * 100 + b4 / 10;

        for (i = 0; i < SUB_MAX_TEXT;) {
            if (!internal_subf_gets(line, fd)) {
                break;
            }

            len = 0;
            for (p = line; *p != '\n' && *p != '\r' && *p; p++, len++) {
                ;
            }

            if (len) {
                int j = 0, skip = 0;
                char *curptr = current->text.text[i] = (char *)MALLOC(len + 1);
                if (!current->text.text[i]) {
                    return ERR;
                }
                for (; j < len; j++) {
                    /* let's filter html tags ::atmos */
                    if (line[j] == '>') {
                        skip = 0;
                        continue;
                    }
                    if (line[j] == '<') {
                        skip = 1;
                        continue;
                    }
                    if (skip) {
                        continue;
                    }
                    *curptr = line[j];
                    curptr++;
                }
                *curptr = '\0';
                i++;
            } else {
                break;
            }
        }
        current->text.lines = i;
    }
    return current;
}

subtitle_t *internal_sub_read_line_subviewer2(int fd, subtitle_t *current)
{
    char line[LINE_LEN + 1];
    int a1, a2, a3, a4;
    char *p = NULL;
    int i, len;

    while (!current->text.text[0]) {
        if (!internal_subf_gets(line, fd)) {
            return NULL;
        }
        if (line[0] != '{') {
            continue;
        }
        if ((len = sscanf(line, "{T %d:%d:%d:%d", &a1, &a2, &a3, &a4)) < 4) {
            continue;
        }

        current->start = a1 * 360000 + a2 * 6000 + a3 * 100 + a4 / 10;

        for (i = 0; i < SUB_MAX_TEXT;) {
            if (!internal_subf_gets(line, fd)) {
                break;
            }
            if (line[0] == '}') {
                break;
            }
            len = 0;

            for (p = line; *p != '\n' && *p != '\r' && *p; ++p, ++len) {
                ;
            }

            if (len) {
                current->text.text[i] = (char *)MALLOC(len + 1);

                if (!current->text.text[i]) {
                    return ERR;
                }

                strncpy(current->text.text[i], line, len);

                current->text.text[i][len] = '\0';
                ++i;
            } else {
                break;
            }
        }
        current->text.lines = i;
    }
    return current;
}


subtitle_t *internal_sub_read_line_vplayer(int fd, subtitle_t *current)
{
    char line[LINE_LEN + 1];
    int a1, a2, a3;
    char *p = NULL, *next, separator;
    int i, len, plen;

    while (!current->text.text[0]) {
        if (!internal_subf_gets(line, fd)) {
            return NULL;
        }
        if ((len = sscanf(line, "%d:%d:%d%c%n", &a1, &a2, &a3, &separator, &plen)) < 4) {
            continue;
        }

        current->start = a1 * 360000 + a2 * 6000 + a3 * 100;
        if (!current->start) {
            continue;
        }

        p = &line[plen];
        //log_print("plen %d, p %s\n", plen, p);

        i = 0;
        if (*p != '|') {
            next = p;
            while (1) {
                next = internal_sub_readtext(next, &(current->text.text[i]));
                if (!next) {
                    break;
                }
                if (current->text.text[i] == ERR) {
                    return ERR;
                }
                i++;
                if (i >= SUB_MAX_TEXT) {
                    log_print(("Too many lines in a subtitle\n"));
                    current->text.lines = i;

                    while (1) {
                        if (!internal_subf_gets(line, fd)) {
                            return NULL;
                        }
                        if ((len = sscanf(line, "%d:%d:%d%c%n", &a1, &a2, &a3, &separator, &plen)) < 4) {
                            continue;
                        } else {
                            current->end = a1 * 360000 + a2 * 6000 + a3 * 100;
                            return current;
                        }
                    }

                    return current;
                }
            }
            current->text.lines = i;
        }
    }

    while (1) {
        if (!internal_subf_gets(line, fd)) {
            return NULL;
        }
        if ((len = sscanf(line, "%d:%d:%d%c%n", &a1, &a2, &a3, &separator, &plen)) < 4) {
            continue;
        } else {
            current->end = a1 * 360000 + a2 * 6000 + a3 * 100;
            return current;
        }
    }

    return current;
}

subtitle_t *internal_sub_read_line_rt(int fd, subtitle_t *current)
{
    //TODO: This format uses quite rich (sub/super)set of xhtml
    // I couldn't check it since DTD is not included.
    // WARNING: full XML parses can be required for proper parsing
    char line[LINE_LEN + 1];
    int a1, a2, a3, a4, b1, b2, b3, b4;
    char *p = NULL, *next = NULL;
    int i, len, plen;

    while (!current->text.text[0]) {
        if (!internal_subf_gets(line, fd)) {
            return NULL;
        }
        //TODO: it seems that format of time is not easily determined, it may be 1:12, 1:12.0 or 0:1:12.0
        //to describe the same moment in time. Maybe there are even more formats in use.
        //if ((len=sscanf (line, "<Time Begin=\"%d:%d:%d.%d\" End=\"%d:%d:%d.%d\"",&a1,&a2,&a3,&a4,&b1,&b2,&b3,&b4)) < 8)
        plen = a1 = a2 = a3 = a4 = b1 = b2 = b3 = b4 = 0;
        if (
            ((len = sscanf(line, "<%*[tT]ime %*[bB]egin=\"%d.%d\" %*[Ee]nd=\"%d.%d\"%*[^<]<clear/>%n", &a3, &a4, &b3, &b4, &plen)) < 4) &&
            ((len = sscanf(line, "<%*[tT]ime %*[bB]egin=\"%d.%d\" %*[Ee]nd=\"%d:%d.%d\"%*[^<]<clear/>%n", &a3, &a4, &b2, &b3, &b4, &plen)) < 5) &&
            ((len = sscanf(line, "<%*[tT]ime %*[bB]egin=\"%d:%d\" %*[Ee]nd=\"%d:%d\"%*[^<]<clear/>%n", &a2, &a3, &b2, &b3, &plen)) < 4) &&
            ((len = sscanf(line, "<%*[tT]ime %*[bB]egin=\"%d:%d\" %*[Ee]nd=\"%d:%d.%d\"%*[^<]<clear/>%n", &a2, &a3, &b2, &b3, &b4, &plen)) < 5) &&
            //          ((len=sscanf (line, "<%*[tT]ime %*[bB]egin=\"%d:%d.%d\" %*[Ee]nd=\"%d:%d\"%*[^<]<clear/>%n",&a2,&a3,&a4,&b2,&b3,&plen)) < 5) &&
            ((len = sscanf(line, "<%*[tT]ime %*[bB]egin=\"%d:%d.%d\" %*[Ee]nd=\"%d:%d.%d\"%*[^<]<clear/>%n", &a2, &a3, &a4, &b2, &b3, &b4, &plen)) < 6) &&
            ((len = sscanf(line, "<%*[tT]ime %*[bB]egin=\"%d:%d:%d.%d\" %*[Ee]nd=\"%d:%d:%d.%d\"%*[^<]<clear/>%n", &a1, &a2, &a3, &a4, &b1, &b2, &b3, &b4, &plen)) < 8) &&
            //now try it without end time
            ((len = sscanf(line, "<%*[tT]ime %*[bB]egin=\"%d.%d\"%*[^<]<clear/>%n", &a3, &a4, &plen)) < 2) &&
            ((len = sscanf(line, "<%*[tT]ime %*[bB]egin=\"%d:%d\"%*[^<]<clear/>%n", &a2, &a3, &plen)) < 2) &&
            ((len = sscanf(line, "<%*[tT]ime %*[bB]egin=\"%d:%d.%d\"%*[^<]<clear/>%n", &a2, &a3, &a4, &plen)) < 3) &&
            ((len = sscanf(line, "<%*[tT]ime %*[bB]egin=\"%d:%d:%d.%d\"%*[^<]<clear/>%n", &a1, &a2, &a3, &a4, &plen)) < 4)
        ) {
            continue;
        }
        current->start = a1 * 360000 + a2 * 6000 + a3 * 100 + a4 / 10;
        current->end   = b1 * 360000 + b2 * 6000 + b3 * 100 + b4 / 10;
        if (b1 == 0 && b2 == 0 && b3 == 0 && b4 == 0) {
            current->end = current->start + 200;
        }
        p = line;
        p += plen;
        i = 0;

        // TODO: I don't know what kind of convention is here for marking multiline subs, maybe <br/> like in xml?
        next = strstr(line, "<clear/>");
        if (next && strlen(next) > 8) {
            next += 8;
            i = 0;
            while (1) {
                next = internal_sub_readtext(next, &(current->text.text[i]));
                if (!next) {
                    break;
                }
                if (current->text.text[i] == ERR) {
                    return ERR;
                }
                i++;
                if (i >= SUB_MAX_TEXT) {
                    log_print(("Too many lines in a subtitle\n"));
                    current->text.lines = i;
                    return current;
                }
            }
        }
        current->text.lines = i + 1;
    }

    return current;
}

subtitle_t *internal_sub_read_line_ssa(int fd, subtitle_t *current)
{
    /*
     * Sub Station Alpha v4 (and v2?) scripts have 9 commas before subtitle
     * other Sub Station Alpha scripts have only 8 commas before subtitle
     * Reading the "ScriptType:" field is not reliable since many scripts appear
     * w/o it
     *
     * http://www.scriptclub.org is a good place to find more examples
     * http://www.eswat.demon.co.uk is where the SSA specs can be found
     */
    int comma;
    static int max_comma = 32;/* let's use 32 for the case that the */
    /* amount of commas increase with newer SSA versions */

    int hour1, min1, sec1, hunsec1,
        hour2, min2, sec2, hunsec2, nothing;
    int num;

    char line[LINE_LEN + 1], line3[LINE_LEN + 1], *line2;
    char *tmp;

    do {
        if (!internal_subf_gets(line, fd)) {
            return NULL;
        }
    } while ((sscanf(line, "Dialogue: Marked=%d,%d:%d:%d.%d,%d:%d:%d.%d,"
                     "%[^\n\r]", &nothing,
                     &hour1, &min1, &sec1, &hunsec1,
                     &hour2, &min2, &sec2, &hunsec2,
                     line3) < 9) &&
             (sscanf(line, "Dialogue: %d,%d:%d:%d.%d,%d:%d:%d.%d,"
                     "%[^\n\r]", &nothing,
                     &hour1, &min1, &sec1, &hunsec1,
                     &hour2, &min2, &sec2, &hunsec2,
                     line3) < 9));

    line2 = strchr(line3, ',');

    for (comma = 4; comma < max_comma; comma ++) {
        tmp = strchr(line2 + 1, ',');
        if (!tmp) {
            break;
        }
        if (*(++tmp) == ' ') {
            break;
        }
        /* a space after a comma means we're already in a sentence */
        line2 = tmp;
    }

    if (comma < max_comma) {
        max_comma = comma;
    }
    /* eliminate the trailing comma */
    if (*line2 == ',') {
        line2++;
    }

    current->text.lines = 0;
    num = 0;
    current->start = 360000 * hour1 + 6000 * min1 + 100 * sec1 + hunsec1;
    current->end   = 360000 * hour2 + 6000 * min2 + 100 * sec2 + hunsec2;

    while (((tmp = strstr(line2, "\\n")) != NULL) || ((tmp = strstr(line2, "\\N")) != NULL)) {
        current->text.text[num] = (char *)MALLOC(tmp - line2 + 1);
        strncpy(current->text.text[num], line2, tmp - line2);
        current->text.text[num][tmp - line2] = '\0';
        line2 = tmp + 2;
        num++;
        current->text.lines++;
        if (current->text.lines >=  SUB_MAX_TEXT) {
            return current;
        }
    }

    current->text.text[num] = internal_sub_strdup(line2);
    current->text.lines++;

    return current;
}

static void internal_sub_pp_ssa(subtitle_t *sub)
{
    int l = sub->text.lines;
    char *so, *de, *start;

    while (l) {
        /* eliminate any text enclosed with {}, they are font and color settings */
        so = de = sub->text.text[--l];
        while (*so) {
            if (*so == '{' && so[1] == '\\') {
                for (start = so; *so && *so != '}'; so++) {
                    ;
                }
                if (*so) {
                    so++;
                } else {
                    so = start;
                }
            }
            if (*so) {
                *de = *so;
                so++;
                de++;
            }
        }
        *de = *so;
    }
}

/*
 * PJS subtitles reader.
 * That's the "Phoenix Japanimation Society" format.
 * I found some of them in http://www.scriptsclub.org/ (used for anime).
 * The time is in tenths of second.
 */
subtitle_t *internal_sub_read_line_pjs(int fd, subtitle_t *current)
{
    char line[LINE_LEN + 1];
    char text[LINE_LEN + 1], *s, *d;

    if (!internal_subf_gets(line, fd)) {
        return NULL;
    }

    /* skip spaces */
    for (s = line; *s && isspace(*s); s++) {
        ;
    }

    /* allow empty lines at the end of the file */
    if (*s == 0) {
        return NULL;
    }

    /* get the time */
    if (sscanf(s, "%ld,%ld,", &(current->start), &(current->end)) < 2) {
        return ERR;
    }

    /* the files I have are in tenths of second */
    current->start *= 10;
    current->end *= 10;

    /* walk to the beggining of the string */
    for (; *s; s++) {
        if (*s == ',') {
            break;
        }
    }
    if (*s) {
        for (s++; *s; s++) if (*s == ',') {
                break;
            }
        if (*s) {
            s++;
        }
    }
    if (*s != '"') {
        return ERR;
    }

    /* copy the string to the text buffer */
    for (s++, d = text; *s && *s != '"'; s++, d++) {
        *d = *s;
    }

    *d = 0;
    current->text.text[0] = internal_sub_strdup(text);
    current->text.lines = 1;

    return current;
}

subtitle_t *internal_sub_read_line_mpsub(int fd, subtitle_t *current)
{
    char line[LINE_LEN + 1];
    float a, b;
    int num = 0;
    char *p, *q;

    do {
        if (!internal_subf_gets(line, fd)) {
            return NULL;
        }
    } while (sscanf(line, "%f %f", &a, &b) != 2);

    mpsub_position += a * 100;
    current->start = (int) mpsub_position;
    mpsub_position += b * 100;
    current->end = (int) mpsub_position;

    while (num < SUB_MAX_TEXT) {
        if (!internal_subf_gets(line, fd)) {
            return (num == 0) ? NULL : current;
        }
        p = line;
        while (isspace(*p)) {
            p++;
        }
        if (internal_eol(*p) && num > 0) {
            return current;
        }
        if (internal_eol(*p)) {
            return NULL;
        }

        for (q = p; !internal_eol(*q); q++) {
            ;
        }

        *q = '\0';

        if (strlen(p)) {
            current->text.text[num] = internal_sub_strdup(p);
            current->text.lines = ++num;
        } else {
            return (num == 0) ? NULL : current;
        }
    }
    return NULL; /* we should have returned before if it's OK */
}

#define str2ms(s) (((s[1]-0x30)*3600*10+(s[2]-0x30)*3600+(s[4]-0x30)*60*10+(s[5]-0x30)*60+(s[7]-0x30)*10+(s[8]-0x30))*1000+(s[10]-0x30)*100+(s[11]-0x30)*10+(s[12]-0x30))

SUBAPI subtitle_t *internal_divx_sub_add(subdata_t *subdata, unsigned char *data)
{
    unsigned char *s;
    subtitle_t *sub = (subtitle_t *)calloc(1, sizeof(subtitle_t));
    if (!sub) {
        return NULL;
    }
    s = &data[0];
    sub->start = str2ms(s) * 90;
    s = &data[13];
    sub->end = str2ms(s) * 90;
    sub->subdata = data;

    //AVSchedLock();

    list_add_tail(&sub->list, &subdata->list);
    subdata->sub_num++;

    //AVSchedUnlock();
    return sub;
}

SUBAPI void internal_divx_sub_delete(subdata_t *subdata, int pts)
{
    list_t *entry;

    entry = subdata->list.next;
    while (entry != &subdata->list) {
        subtitle_t *subp = list_entry(entry, subtitle_t, list);
        if (subp->start < pts) {
            if (subp->subdata) {
                FREE(subp->subdata);
            }

            //AVSchedLock();

            list_del(&subp->list);
            subdata->sub_num--;

            //AVSchedUnlock();

            entry = entry->next;
            FREE(subp);

        } else {
            break;
        }
    }

}

SUBAPI void internal_divx_sub_flush(subdata_t *subdata)
{
    list_t *entry;

    if (subdata->sub_format != SUB_DIVX) {
        return;
    }

    entry = subdata->list.next;
    while (entry != &subdata->list) {
        subtitle_t *subp = list_entry(entry, subtitle_t, list);
        if (subp->subdata) {
            FREE(subp->subdata);
        }
        list_del(&subp->list);
        entry = entry->next;
        FREE(subp);
    }
    subdata->sub_num = 0;
}

static subtitle_t *previous_aqt_sub;
subtitle_t *internal_sub_read_line_aqt(int fd, subtitle_t *current)
{
    char line[LINE_LEN + 1];
    char *next;
    int i;

    while (1) {
        // try to locate next subtitle
        if (!internal_subf_gets(line, fd)) {
            return NULL;
        }
        if (!(sscanf(line, "-->> %ld", &(current->start)) < 1)) {
            break;
        }
    }

    if (previous_aqt_sub != NULL) {
        previous_aqt_sub->end = current->start - 1;
    }

    previous_aqt_sub = current;

    if (!internal_subf_gets(line, fd)) {
        return NULL;
    }

    internal_sub_readtext((char *)&line, &current->text.text[0]);

    current->text.lines = 1;
    current->end = current->start; // will be corrected by next subtitle

    if (!internal_subf_gets(line, fd)) {
        return current;
    }

    next = line, i = 1;
    while (1) {
        next = internal_sub_readtext(next, &(current->text.text[i]));
        if (!next) {
            break;
        }
        if (current->text.text[i] == ERR) {
            return ERR;
        }
        i++;
        if (i >= SUB_MAX_TEXT) {
            log_print(("Too many lines in a subtitle\n"));
            current->text.lines = i;
            return current;
        }
    }
    current->text.lines = i + 1;

    if ((current->text.text[0] == '\0') && (current->text.text[1] == '\0')) {
        // void subtitle -> end of previous marked and exit
        previous_aqt_sub = NULL;
        return NULL;
    }

    return current;
}

subtitle_t *internal_previous_subrip09_sub = NULL;
subtitle_t *internal_sub_read_line_subrip09(int fd, subtitle_t *current)
{
    char line[LINE_LEN + 1];
    int a1, a2, a3;
    char * next = NULL;
    int i, len;

    while (1) {
        // try to locate next subtitle
        if (!internal_subf_gets(line, fd)) {
            return NULL;
        }
        if (!((len = sscanf(line, "[%d:%d:%d]", &a1, &a2, &a3)) < 3)) {
            break;
        }
    }

    current->start = a1 * 360000 + a2 * 6000 + a3 * 100;

    if (internal_previous_subrip09_sub != NULL) {
        internal_previous_subrip09_sub->end = current->start - 1;
    }

    internal_previous_subrip09_sub = current;

    if (!internal_subf_gets(line, fd)) {
        return NULL;
    }

    next = line, i = 0;

    current->text.text[0] = '\0'; // just to be sure that string is clear

    while (1) {
        next = internal_sub_readtext(next, &(current->text.text[i]));
        if (!next) {
            break;
        }
        if (current->text.text[i] == ERR) {
            return ERR;
        }
        i++;
        if (i >= SUB_MAX_TEXT) {
            log_print(("Too many lines in a subtitle\n"));
            current->text.lines = i;
            return current;
        }
    }
    current->text.lines = i + 1;

    if ((current->text.text[0] == '\0') && (i == 0)) {
        // void subtitle -> end of previous marked and exit
        internal_previous_subrip09_sub = NULL;
        return NULL;
    }

    return current;
}

subtitle_t *internal_sub_read_line_jacosub(int fd, subtitle_t * current)
{
    char line1[LINE_LEN], line2[LINE_LEN], directive[LINE_LEN], *p, *q;
    unsigned a1, a2, a3, a4, b1, b2, b3, b4, comment = 0;
    static unsigned jacoTimeres = 30;
    static int jacoShift = 0;

    memset(current, 0, sizeof(subtitle_t));
    memset(line1, 0, LINE_LEN);
    memset(line2, 0, LINE_LEN);
    memset(directive, 0, LINE_LEN);

    while (!current->text.text[0]) {
        if (!internal_subf_gets(line1, fd)) {
            return NULL;
        }
        if (sscanf(line1, "%u:%u:%u.%u %u:%u:%u.%u %[^\n\r]",
                   &a1, &a2, &a3, &a4,
                   &b1, &b2, &b3, &b4, line2) < 9) {
            if (sscanf(line1, "@%u @%u %[^\n\r]", &a4, &b4, line2) < 3) {
                if (line1[0] == '#') {
                    int hours = 0, minutes = 0, seconds, delta, inverter = 1;
                    unsigned units = jacoShift;
                    switch (toupper(line1[1])) {
                    case 'S':
                        if (isalpha(line1[2])) {
                            delta = 6;
                        } else {
                            delta = 2;
                        }
                        if (sscanf(&line1[delta], "%d", &hours)) {
                            if (hours < 0) {
                                hours *= -1;
                                inverter = -1;
                            }
                            if (sscanf(&line1[delta], "%*d:%d", &minutes)) {
                                if (sscanf(&line1[delta], "%*d:%*d:%d", &seconds)) {
                                    sscanf(&line1[delta], "%*d:%*d:%*d.%d", &units);
                                } else {
                                    hours = 0;
                                    sscanf(&line1[delta], "%d:%d.%d", &minutes, &seconds, &units);
                                    minutes *= inverter;
                                }
                            } else {
                                hours = minutes = 0;
                                sscanf(&line1[delta], "%d.%d", &seconds, &units);
                                seconds *= inverter;
                            }
                            jacoShift = ((hours * 3600 + minutes * 60 + seconds) * jacoTimeres + units)
                                        * inverter;
                        }
                        break;

                    case 'T':
                        if (isalpha(line1[2])) {
                            delta = 8;
                        } else {
                            delta = 2;
                        }
                        sscanf(&line1[delta], "%u", &jacoTimeres);
                        break;
                    }
                }
                continue;
            } else {
                current->start = (unsigned long)((a4 + jacoShift) * 100.0 / jacoTimeres);
                current->end = (unsigned long)((b4 + jacoShift) * 100.0 / jacoTimeres);
            }
        } else {
            current->start = (unsigned long)(((a1 * 3600 + a2 * 60 + a3) * jacoTimeres + a4 +
                                              jacoShift) * 100.0 / jacoTimeres);
            current->end = (unsigned long)(((b1 * 3600 + b2 * 60 + b3) * jacoTimeres + b4 +
                                            jacoShift) * 100.0 / jacoTimeres);
        }

        current->text.lines = 0;
        p = line2;
        while ((*p == ' ') || (*p == '\t')) {
            ++p;
        }

        if (isalpha(*p) || *p == '[') {
            int cont, jLength;

            if (sscanf(p, "%s %[^\n\r]", directive, line1) < 2) {
                return (subtitle_t *) ERR;
            }
            jLength = strlen(directive);
            for (cont = 0; cont < jLength; ++cont) {
                if (isalpha(*(directive + cont))) {
                    *(directive + cont) = toupper(*(directive + cont));
                }
            }
            if ((strstr(directive, "RDB") != NULL)
                || (strstr(directive, "RDC") != NULL)
                || (strstr(directive, "RLB") != NULL)
                || (strstr(directive, "RLG") != NULL)) {
                continue;
            }
            if (strstr(directive, "JL") != NULL) {
                current->text.alignment = SUB_ALIGNMENT_BOTTOMLEFT;
            } else if (strstr(directive, "JR") != NULL) {
                current->text.alignment = SUB_ALIGNMENT_BOTTOMRIGHT;
            } else {
                current->text.alignment = SUB_ALIGNMENT_BOTTOMCENTER;
            }
            strcpy(line2, line1);
            p = line2;
        }
        for (q = line1; (!internal_eol(*p)) && (current->text.lines < SUB_MAX_TEXT); ++p) {
            switch (*p) {
            case '{':
                comment++;
                break;
            case '}':
                if (comment) {
                    --comment;
                    //the next line to get rid of a blank after the comment
                    if ((*(p + 1)) == ' ') {
                        p++;
                    }
                }
                break;
            case '~':
                if (!comment) {
                    *q = ' ';
                    ++q;
                }
                break;
            case ' ':
            case '\t':
                if ((*(p + 1) == ' ') || (*(p + 1) == '\t')) {
                    break;
                }
                if (!comment) {
                    *q = ' ';
                    ++q;
                }
                break;
            case '\\':
                if (*(p + 1) == 'n') {
                    *q = '\0';
                    q = line1;
                    current->text.text[current->text.lines++] = internal_sub_strdup(line1);
                    ++p;
                    break;
                }
                if ((toupper(*(p + 1)) == 'C')
                    || (toupper(*(p + 1)) == 'F')) {
                    ++p, ++p;
                    break;
                }
                if ((*(p + 1) == 'B') || (*(p + 1) == 'b') || (*(p + 1) == 'D') ||  //actually this means "insert current date here"
                    (*(p + 1) == 'I') || (*(p + 1) == 'i') || (*(p + 1) == 'N') || (*(p + 1) == 'T') || //actually this means "insert current time here"
                    (*(p + 1) == 'U') || (*(p + 1) == 'u')) {
                    ++p;
                    break;
                }
                if ((*(p + 1) == '\\') ||
                    (*(p + 1) == '~') || (*(p + 1) == '{')) {
                    ++p;
                } else if (internal_eol(*(p + 1))) {
                    if (!internal_subf_gets(directive, fd)) {
                        return NULL;
                    }
                    internal_trail_space(directive);
                    strncat(line2, directive,
                            (LINE_LEN > 511) ? LINE_LEN : 511);
                    break;
                }
            default:
                if (!comment) {
                    *q = *p;
                    ++q;
                }
                break;
            } //-- switch
        } //-- for
        *q = '\0';
        current->text.text[current->text.lines] = internal_sub_strdup(line1);
    } //-- while
    current->text.lines++;
    return current;
}

static int internal_sub_autodetect(int fd)
{
    char line[LINE_LEN + 1];
    int i, j = 0;
    char p;

    j = sscanf("00:02:21,606 --> 00:02:23,073",
               "%d:%d:%d%[,.:]%d --> %d:%d:%d%[,.:]%d",
               &i, &i, &i, (char *)&i, &i, &i, &i, &i, (char *)&i, &i);
    j = sscanf("abc",
               "[aef]bc",
               (char *)&i);

    while (j < 100) {
        j++;
        if (!internal_subf_gets(line, fd)) {
            return SUB_INVALID;
        }

        if (sscanf(line, "{%d}{%d}", &i, &i) == 2) {
            return SUB_MICRODVD;
        }
        if (sscanf(line, "{%d}{}", &i) == 1) {
            return SUB_MICRODVD;
        }
        if (sscanf(line, "[%d][%d]", &i, &i) == 2) {
            return SUB_MPL2;
        }
        if (sscanf(line, "%d:%d:%d.%d,%d:%d:%d.%d",     &i, &i, &i, &i, &i, &i, &i, &i) == 8) {
            return SUB_SUBRIP;
        }
        if (sscanf(line, "%d:%d:%d%[,.:]%d --> %d:%d:%d%[,.:]%d", &i, &i, &i, (char *)&i, &i, &i, &i, &i, (char *)&i, &i) == 10) {
            return SUB_SUBVIEWER;
        }
        if (sscanf(line, "{T %d:%d:%d:%d", &i, &i, &i, &i) == 4) {
            return SUB_SUBVIEWER2;
        }
        if (strstr(line, "<SAMI>")) {
            return SUB_SAMI;
        }
        if (sscanf(line, "%d:%d:%d.%d %d:%d:%d.%d", &i, &i, &i, &i, &i, &i, &i, &i) == 8) {
            return SUB_JACOSUB;
        }
        if (sscanf(line, "@%d @%d", &i, &i) == 2) {
            return SUB_JACOSUB;
        }
        if (sscanf(line, "%d:%d:%d:", &i, &i, &i) == 3) {
            return SUB_VPLAYER;
        }
        if (sscanf(line, "%d:%d:%d ", &i, &i, &i) == 3) {
            return SUB_VPLAYER;
        }
        //TODO: just checking if first line of sub starts with "<" is WAY
        // too weak test for RT
        // Please someone who knows the format of RT... FIX IT!!!
        // It may conflict with other sub formats in the future (actually it doesn't)
        if (*line == '<') {
            return SUB_RT;
        }
        if (!memcmp(line, "Dialogue: Marked", 16)) {
            return SUB_SSA;
        }
        if (!memcmp(line, "Dialogue: ", 10)) {
            return SUB_SSA;
        }
        if (sscanf(line, "%d,%d,\"%c", &i, &i, (char *) &i) == 3) {
            return SUB_PJS;
        }
        if (sscanf(line, "FORMAT=%d", &i) == 1) {
            return SUB_MPSUB;
        }
        if (sscanf(line, "FORMAT=TIM%c", &p) == 1 && p == 'E') {
            return SUB_MPSUB;
        }
        if (strstr(line, "-->>")) {
            return SUB_AQTITLE;
        }
        if (sscanf(line, "[%d:%d:%d]", &i, &i, &i) == 3) {
            return SUB_SUBRIP09;
        }
    }

    return SUB_INVALID;  // too many bad lines
}

SUBAPI void internal_sub_close(subdata_t *subdata)
{
    int i;
    list_t *entry;

    entry = subdata->list.next;
    while (entry != &subdata->list) {
        subtitle_t *subt = list_entry(entry, subtitle_t, list);
        if (subdata->sub_format == SUB_DIVX) {
            if (subt->subdata) {
                FREE(subt->subdata);
            }
        } else {
            for (i = 0; i < subt->text.lines; i++) {
                FREE(subt->text.text[i]);
            }
        }
        entry = entry->next;
        FREE(subt);
    }

    FREE(subdata);
}

SUBAPI subdata_t *internal_sub_open(char *filename, unsigned int rate)
{
    int fd = 0;
    subtitle_t *sub, *sub_read;
    subdata_t *subdata;
    int sub_format = SUB_INVALID;

    subreader_t sr[] = {
        { internal_sub_read_line_microdvd, NULL, "microdvd" },
        { internal_sub_read_line_subrip, NULL, "subrip" },
        { internal_sub_read_line_subviewer, NULL, "subviewer" },
        { internal_sub_read_line_sami, NULL, "sami" },
        { internal_sub_read_line_vplayer, NULL, "vplayer" },
        { internal_sub_read_line_rt, NULL, "rt" },
        { internal_sub_read_line_ssa, internal_sub_pp_ssa, "ssa" },
        { internal_sub_read_line_pjs, NULL, "pjs" },
        { internal_sub_read_line_mpsub, NULL, "mpsub" },
        { internal_sub_read_line_aqt, NULL, "aqt" },
        { internal_sub_read_line_subviewer2, NULL, "subviewer 2.0" },
        { internal_sub_read_line_subrip09, NULL, "subrip 0.9" },
        { internal_sub_read_line_jacosub, NULL, "jacosub" },
        { internal_sub_read_line_mpl2, NULL, "mpl2" }
    };
    subreader_t *srp;

    if (filename == NULL) {
        return NULL;
    } else if (0 == strcmp(filename, "subdivx")) {
        sub_format = SUB_DIVX;
    } else {
        fd = open(filename, O_RDONLY);
        if (fd < 0) {
            return NULL;
        }
    }

    subdata = (subdata_t *)MALLOC(sizeof(subdata_t));
    if (!subdata) {
        close(fd);
        return NULL;
    }
    memset(subdata, 0, sizeof(subdata_t));
    INIT_LIST_HEAD(&subdata->list);

    if (sub_format == SUB_DIVX) {
        subdata->sub_format = sub_format;
        subdata->sub_num = 0;
        return subdata;
    } else {
        subdata->sub_format = internal_sub_autodetect(fd);
    }

    if (subdata->sub_format == SUB_INVALID) {
        log_print(("SUB: Could not determine file format\n"));
        internal_sub_close(subdata);
        close(fd);
        return NULL;
    }
    srp = sr + subdata->sub_format;
    //log_print(("SUB: Detected subtitle file format: %s\n", srp->name));

    lseek(fd, 0, SEEK_SET);
    if (subfile_buffer) {
        FREE(subfile_buffer);
        subfile_buffer = NULL;
    }

    while (1) {
        sub = (subtitle_t *)MALLOC(sizeof(subtitle_t));
        if (!sub) {
            break;
        }
        memset(sub, 0, sizeof(subtitle_t));

        sub->end = rate;
        sub_read = srp->read(fd, sub);
        if (!sub_read) {
            FREE(sub);
            break;   // EOF
        }

        if (sub_read == ERR) {
            FREE(sub);
            subdata->sub_error++;
        } else {
            // Apply any post processing that needs recoding first
            if (!sub_no_text_pp && srp->post) {
                srp->post(sub_read);
            }

            /* 10ms to pts conversion */
            sub->start = sub_ms2pts(sub->start);
            sub->end = sub_ms2pts(sub->end);

            //log_print("return: %s\n", sub->text.text[0]);

            list_add_tail(&sub->list, &subdata->list);
            subdata->sub_num++;
        }
    }

    if (subfile_buffer) {
        FREE(subfile_buffer);
        subfile_buffer = NULL;
    }

    close(fd);

    if (subdata->sub_num <= 0) {
        internal_sub_close(subdata);
        return NULL;
    }

    log_print("SUB: Read %d subtitles", subdata->sub_num);
    if (subdata->sub_error) {
        log_print(", %d bad line(s).\n", subdata->sub_error);
    } else {
        log_print((".\n"));
    }

    return subdata;
}

SUBAPI char *internal_sub_filenames(char *filename, unsigned perfect_match)
{
    char *dir;
    char *sub_exts[] = {"utf", "sub", "srt", "smi",
                        "rt", "txt", "ssa", "aqt", "jss", "js", "ass", NULL
                       };
    char *p = NULL, *p2 = NULL;
    char *ext, *fn, *long_fn;
    DIR *pDir;
    int i, looking;
    struct dirent *pDirEntry;
    char *subname = NULL;
    unsigned videofile_lnamlen = 0;

    /* get directory name first */
    fn = p;
    p = strrchr(filename, '/');
    p2 = strrchr(filename, '\\');
    if (p2 > p) {
        p = p2;
    }

    if (p) {
        dir = (char *)MALLOC(p - filename + 2);
        if (!dir) {
            return NULL;
        }
        memcpy(dir, filename, p - filename + 1);   /* including final '/' or '\\' */
        dir[p - filename + 1] = '\0';
        fn = p + 1;
    } else {
        dir = (char *)MALLOC(3);
        if (!dir) {
            return NULL;
        }
        strcpy(dir, "./");
    }

    /* get long file name of input file */
    pDir = opendir(dir);
    if (!pDir) {
        FREE(dir);
        return NULL;
    }

    looking = 1;
    while (looking) {
        pDirEntry = readdir(pDir);
        if (!pDirEntry) {
            break;
        }
        if (pDirEntry->d_type & S_IFDIR) {
            continue;
        }
        /*
        if(pDirEntry->d_reclen){
            if(strncasecmp(fn,pDirEntry->d_name,pDirEntry->d_reclen) == 0){
                looking = 0;
            }
        }
        else{
            if(strncasecmp(fn, pDirEntry->d_name, pDirEntry->d_namlen) == 0){
                looking = 0;
            }
        }*/
        if (strncasecmp(fn, pDirEntry->d_name, pDirEntry->d_reclen) == 0) {
            looking = 0;
        }
    }

    if (looking == 1) {
        closedir(pDir);
        FREE(dir);
        return NULL;
    } else {
        /* sometimes long file name is not available, just use short name */
#if 0
        if (pDirEntry->d_lnamlen) {
            long_fn = MALLOC(pDirEntry->d_lnamlen);
        } else {
            long_fn = MALLOC(pDirEntry->d_namlen);
        }
#endif
        long_fn = MALLOC(pDirEntry->d_reclen);

        if (!long_fn) {
            closedir(pDir);
            FREE(dir);
            return NULL;
        }

#if 0
        if (pDirEntry->d_lnamlen) {
            memcpy(long_fn, pDirEntry->d_lname, pDirEntry->d_lnamlen);
            videofile_lnamlen = pDirEntry->d_lnamlen ;
        } else {
            memcpy(long_fn, pDirEntry->d_name, pDirEntry->d_namlen);
        }
#endif
        memcpy(long_fn, pDirEntry->d_name, pDirEntry->d_reclen);
        if (pDirEntry->d_reclen) {
            videofile_lnamlen = pDirEntry->d_reclen;
        }
    }
    closedir(pDir);

    /* match subtitle file names */
    pDir = opendir(dir);
    if (!pDir) {
        FREE(long_fn);
        FREE(dir);
        return NULL;
    }

    looking = 1;
    while (looking) {
        pDirEntry = readdir(pDir);
        if (!pDirEntry) {
            break;
        }
        if (pDirEntry->d_type & S_IFDIR) {
            continue;
        }

        /* find an entry with extension */
        ext = strrchr(pDirEntry->d_name, '.');
        if (!ext) {
            continue;
        }
        ext++;

        i = 0;
        while (sub_exts[i]) {
            if (strcasecmp(ext, sub_exts[i]) == 0) {
                if (subname) {
                    FREE(subname);
                }
                subname = internal_sub_strdup(pDirEntry->d_name);

#if 0
                if (pDirEntry->d_lnamlen) {
                    if (memcmp(long_fn, pDirEntry->d_lname, videofile_lnamlen - 4) == 0) {
                        /* we have a perfect match here, same filename and right extension */
                        looking = 0;
                    }
                } else {
                    if (memcmp(long_fn, pDirEntry->d_name, (pDirEntry->d_namlen - 4)) == 0) {
                        looking = 0;
                    }
                }
#endif
                if (memcmp(long_fn, pDirEntry->d_name, (strlen(long_fn) - 4)) == 0) {
                    looking = 0;
                }
                break;
            }
            i++;
        }
    }

    closedir(pDir);
    FREE(long_fn);

    /* join dir and subname */
    if ((subname) && (looking == 0))
        /*((perfect_match == 0) || (looking == 0)))*/
    {
        p = (char *)realloc(dir, strlen(dir) + strlen(subname) + 1);
        if (p) {
            strcat(p, subname);
        }
        FREE(subname);
        return p;
    } else {
        FREE(dir);
        return NULL;
    }
}

/* find the subtitle with it's endtime mostly after current playing time */
SUBAPI subtitle_t *internal_sub_search(subdata_t *subdata, subtitle_t *ref, int pts)
{
    subtitle_t *sub_start;
    subtitle_t *sub_search;
    list_t *entry;

    sub_start = ref;
    if (!sub_start) {
        /* use first entry */
        sub_start = list_entry(subdata->list.next, subtitle_t, list);
    }

    if (sub_start->end > pts) {
        /* do backward search */
        for (entry = &sub_start->list; entry != &subdata->list; entry = entry->prev) {
            sub_search = list_entry(entry, subtitle_t, list);
            if (sub_search->end <= pts) {
                break;
            }
        }

        return list_entry(entry->next, subtitle_t, list);
    } else {
        /* do forward search */
        for (entry = &sub_start->list; entry != &subdata->list; entry = entry->next) {
            sub_search = list_entry(entry, subtitle_t, list);
            if (sub_search->end >= pts) {
                break;
            }
        }

        return (entry == &subdata->list) ? NULL : list_entry(entry, subtitle_t, list);
    }

}

SUBAPI int internal_sub_get_starttime(subtitle_t *subt)
{
    return subt->start;
}

SUBAPI int internal_sub_get_endtime(subtitle_t *subt)
{
    return subt->end;
}

SUBAPI subtext_t *internal_sub_get_text(subtitle_t *subt)
{
    return &subt->text;
}

#ifdef DUMP_SUB
void internal_dump_sub_infofile(subdata_t *subdata)
{
    int i, j;
    list_t *entry;
    FILE *tmp_file;

    tmp_file = fopen("/tmp/sub.txt", "w+");
    if (tmp_file == NULL) {
        log_print("can not open dump file\n");
        return;
    }

    list_for_each(entry, &subdata->list) {
        subtitle_t *subt = list_entry(entry, subtitle_t, list);
        for (i = 0; i < subt->text.lines; i++) {
            fprintf(tmp_file, "%i line%c (%li-%li)\n", subt->text.lines, (1 == subt->text.lines) ? ' ' : 's',
                    subt->start, subt->end);
            for (j = 0; j < subt->text.lines; j++) {
                fprintf(tmp_file, "\t\t%d: %s%s", j, subt->text.text[j], (j == subt->text.lines - 1) ? "" : " \n");
            }
            fprintf(tmp_file, "\n");
        }
    }

    //log_print("$$$$$$end of internal_dump_sub_infofile, m %d\n", m);
}
#endif
