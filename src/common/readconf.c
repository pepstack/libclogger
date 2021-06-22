/***********************************************************************
 * Copyright (c) 2008-2080 pepstack.com, 350137278@qq.com
 *
 * ALL RIGHTS RESERVED.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **********************************************************************/

/**
 * @filename   readconf.c
 *  read ini file api for Linux and Windows.
 *
 * @author     Liang Zhang <350137278@qq.com>
 * @version    0.0.9
 * @create     2012-05-21 10:14:00
 * @update     2020-05-19 18:34:50
 */
#include "readconf.h"
#include <math.h>


# if defined (_MSC_VER)
    # pragma warning(disable:4996)
#endif

#ifndef NOWARNING_UNUSED
    # if defined(__GNUC__) || defined(__CYGWIN__)
        # define NOWARNING_UNUSED(x) __attribute__((unused)) x
    # else
        # define NOWARNING_UNUSED(x) x
    # endif
#endif


typedef struct _conf_position_t
{
    FILE *_fp;
    char  _secname[READCONF_MAX_SECNAME + 4];
    char  _linebuf[READCONF_MAX_LINESIZE + 4];
} conf_position_t;


NOWARNING_UNUSED(static) char * trim(char *s, char c)
{
    return (*s==0)?s:(((*s!=c)?(((trim(s+1,c)-1)==s)?s:(*(trim(s+1,c)-1)=*s,*s=c,trim(s+1,c))):trim(s+1,c)));
}


NOWARNING_UNUSED(static) char * trim_whitespace (char * s)
{
    return (*s==0)?s:((( ! isspace((int)*s) )?(((trim_whitespace(s+1)-1)==s)? s : (*(trim_whitespace(s+1)-1)=*s, *s=32 ,trim_whitespace(s+1))):trim_whitespace(s+1)));
}


NOWARNING_UNUSED(static) char * ltrim (char *s, char c)
{
    while(*s!=0&&*s==c){s++;}return s;
}


NOWARNING_UNUSED(static) char * rtrim (char *s, char c)
{
    char *p=s,*q=s;while(*p!=0){if(*p!=c){q=p;q++;}p++;}if(q!=s)*q=0;return s;
}


NOWARNING_UNUSED(static) char * dtrim (char *s, char c)
{
    return rtrim(ltrim(s, c), c);
}


NOWARNING_UNUSED(static) int readln (FILE *fp, char *strbuf, int bufsize)
{
    int  i, j;
    char ch;

    for (i=0, j=0; i<bufsize; j++) {
        if (fread(&ch, sizeof(char), 1, fp) != 1) {
            /* read error */
            if (feof(fp) != 0) {
                if (j==0) {
                    return -1;  /* end file */
                } else {
                    break;
                }
            }

            return -2;
        } else {
            /* read a char */
            if (ch==10 || ch==0) { /* 10='\n' */
                /* read a line ok */
                break;
            }

            if (ch==12 || ch==0x1A) { /* 12='\f' */
                strbuf[i++]=ch;
                break;
            }

            if (ch != 13) { /* '\r' */
                strbuf[i++]=ch;
            }
        }

        if (i==bufsize) {
            return -3; /* exceed max chars */
        }
    }

    strbuf[i] = 0;
    return i;
}


NOWARNING_UNUSED(static) int splitpair (char *strbuf, char sep, char **key, char **val)
{
    char *at = strchr(strbuf, sep);

    if (at) {
        *at++ = 0;
    }

    *key = strbuf;
    *key = dtrim(dtrim(*key, 32), 9);

    *val = at;
    if (at) {
        *val = dtrim(dtrim(dtrim(*val, 32), 9), 34); /* '\"' */
    }

    if (*key != 0) {
        return READCONF_TRUE;
    } else {
        return READCONF_FALSE;
    }
}


NOWARNING_UNUSED(static) char** _SectionListAlloc (int numSections)
{
    char **secs = (char**) ConfMemAlloc(numSections+1, sizeof(char*));
    secs[0] = (char*) (uintptr_t) numSections;
    return secs;
}


void * ConfMemAlloc (int numElems, int sizeElem)
{
    void *p = calloc(numElems, sizeElem);
    if (!p) {
        exit(READCONF_RET_OUTMEM);
    }
    return p;
}


void * ConfMemRealloc (void *oldBuffer, int oldSize, int newSize)
{
    void *newBuffer = 0;
    assert(newSize >= 0);
    if (newSize==0) {
        ConfMemFree(oldBuffer);
        return newBuffer;
    }

    if (oldSize==newSize) {
        return oldBuffer? oldBuffer : ConfMemAlloc(1, newSize);
    }

    newBuffer = realloc(oldBuffer, newSize);
    if (!newBuffer) {
        exit(READCONF_RET_OUTMEM);
    }

    if (newSize > oldSize) {
        memset((char*)newBuffer+oldSize, 0, (size_t)(newSize-oldSize));
    }
    return newBuffer;
}


void ConfMemFree (void *pBuffer)
{
    if (pBuffer) {
        free(pBuffer);
    }
}


char * ConfMemCopyString (char **dst, const char *src)
{
    *dst = 0;

    if (src) {
        size_t cb = strlen(src) + 1;
        *dst = (char*) malloc(cb);
        memcpy(*dst, src, cb);
    }
    return *dst;
}


CONF_position ConfOpenFile (const char *conf)
{
    CONF_position cpos;
    FILE *fp = fopen(conf, "rb");
    if (! fp) {
        return 0;
    }
    cpos = (CONF_position) ConfMemAlloc(1, sizeof(conf_position_t));
    cpos->_fp = fp;
    return cpos;
}


void ConfCloseFile (CONF_position cpos)
{
    fclose(cpos->_fp);
    ConfMemFree(cpos);
}


char * ConfGetNextPair (CONF_position cpos, char **key, char **val)
{
    char *start;
    int   nch;

    for (;;) {
        nch = readln(cpos->_fp, cpos->_linebuf, READCONF_MAX_LINESIZE);
        if (nch < 0) {
            return 0;
        }
        start = dtrim(dtrim(cpos->_linebuf, 32), 9);
        if (*start == READCONF_NOTE_CHAR) { /* # */
            continue;
        }

        nch = (int) strlen(start);
        if (nch > 2) {
            if (nch <= READCONF_MAX_SECNAME && *start==READCONF_SEC_BEGIN && *(start+nch-1)==READCONF_SEC_END) {
                /* find a section */
                memcpy(cpos->_secname, start+1, nch-1);
                cpos->_secname[nch-1] = 0;
                cpos->_secname[nch-2] = 0;
                continue;
            }

            if (splitpair(start, READCONF_SEPARATOR, key, val)==READCONF_TRUE) {
                return start;
            }
        }
    }
    return 0;
}


char * ConfGetFirstPair (CONF_position cpos, char **key, char **val)
{
    if (! cpos) {
        return 0;
    }

    rewind(cpos->_fp);
    return ConfGetNextPair(cpos, key, val);
}


const char * ConfGetSection (CONF_position cpos)
{
    return cpos->_secname;
}


READCONF_RESULT ConfCopySection (CONF_position cpos, char *secName)
{
    if (!cpos->_secname) {
        return READCONF_RET_ERROR;
    }
    if (!secName) {
        return (int) strlen(cpos->_secname)+2;
    }
    memcpy(secName, cpos->_secname, strlen(cpos->_secname)+2);
    return READCONF_RET_SUCCESS;
}


int ConfReadValue (const char *confFile, const char *sectionName, const char *keyName, char *valbuf, size_t maxbufsize)
{
    char *str, *key, *val, endchr;
    int valsz;

    CONF_position cpos = ConfOpenFile(confFile);

    str = ConfGetFirstPair(cpos, &key, &val);
    while (str) {
        if (! sectionName || ! strcmp(ConfGetSection(cpos), sectionName)) {
            if (! strcmp(key, keyName)) {
                if (val) {
                    valsz = (int) strlen(val) + 1;

                    if (! valbuf) {
                        ConfCloseFile(cpos);
                        return valsz;
                    }

                    memcpy(valbuf, val, valsz);

                    /* check last char: '\' */
                    if (valbuf[valsz - 2] != '\\') {
                        /* end of val */
                        ConfCloseFile(cpos);
                        return valsz;
                    }

                    endchr = valbuf[valsz - 2];

                    while (endchr == '\\') {
                        char *start;

                        /* read next line for: '+' */
                        int nch = readln(cpos->_fp, cpos->_linebuf, READCONF_MAX_LINESIZE);
                        if (nch < 0) {
                            /* end of val */
                            ConfCloseFile(cpos);
                            return valsz;
                        }

                        start = dtrim(dtrim(cpos->_linebuf, 32), 9);
                        if (*start == READCONF_NOTE_CHAR) { /* # */
                            continue;
                        }

                        if (*start != '+') {
                            /* end of val */
                            valbuf[valsz-- - 2] = '\0';
                            ConfCloseFile(cpos);
                            return valsz;
                        }

                        start = dtrim(dtrim(start+1, 32), 9);
                        nch = (int) strlen(start);
                        endchr = start[nch - 1];

                        if (valsz - 2 + nch < (int)maxbufsize) {
                            memcpy(valbuf + valsz - 2, start, nch + 1);
                        }

                        valsz = valsz - 2 + nch + 1;
                    }

                    ConfCloseFile(cpos);
                    return valsz;
                } else {
                    *valbuf = 0;
                    ConfCloseFile(cpos);
                    return 1;
                }
            }
        }

        str = ConfGetNextPair(cpos, &key, &val);
    }

    ConfCloseFile(cpos);
    return 0;
}


READCONF_BOOL ConfReadValueRef (const char *confFile, const char *sectionName, const char *keyName, char **ppRefVal)
{
    char *str;
    char *key;
    char *val;

    CONF_position cpos = ConfOpenFile(confFile);

    str = ConfGetFirstPair(cpos, &key, &val);
    while (str) {
        if (! sectionName || ! strcmp(ConfGetSection(cpos), sectionName)) {
            if (! strcmp(key, keyName)) {
                *ppRefVal = val;
                ConfCloseFile(cpos);
                return READCONF_TRUE;
            }
        }

        str = ConfGetNextPair(cpos, &key, &val);
    }

    ConfCloseFile(cpos);
    return READCONF_FALSE;
}


int ConfReadValueParsed (const char *confFile, const char *family, const char *qualifier, const char *key, char *valbuf, size_t maxbufsize)
{
    if (! qualifier) {
        return  ConfReadValue(confFile, family, key, valbuf, maxbufsize);
    } else {
        char section[READCONF_MAX_SECNAME + 4];
        snprintf(section, sizeof(section), "%s:%s", family, qualifier);
        return  ConfReadValue(confFile, section, key, valbuf, maxbufsize);
    }
}


int ConfReadValueParsedAlloc (const char *confFile, const char *family, const char *qualifier, const char *key, char **value)
{
    int maxbufsize = READCONF_MAX_LINESIZE;
    char *valbuf = ConfMemAlloc(1, maxbufsize);

    int valsz = ConfReadValueParsed(confFile, family, qualifier, key, valbuf, maxbufsize);
    if (valsz > maxbufsize) {
        maxbufsize = valsz;

        ConfMemFree(valbuf);
        valbuf = ConfMemAlloc(1, maxbufsize);

        valsz = ConfReadValueParsed(confFile, family, qualifier, key, valbuf, maxbufsize);
    }

    *value = valbuf;
    return valsz;
}


int ConfGetSectionList (const char *confFile, void **sectionList)
{
    char *key;
    char *val;
    int   numSections = 0;
    *sectionList = 0;

    CONF_position cpos = ConfOpenFile(confFile);
    if (!cpos) {
        return READCONF_RET_ERROR;
    }

    /* get count of secions */
    char  secName[READCONF_MAX_SECNAME];
    secName[0] = 0;
    while (ConfGetNextPair(cpos, &key, &val) != 0) {
        const char *sec = ConfGetSection(cpos);
        if (strcmp(sec, secName)) {
            ConfCopySection(cpos, secName);
            numSections++;
        }
    }
    ConfCloseFile(cpos);

    if (numSections > 0) {
        int  secNo = 1;
        char **secs = _SectionListAlloc(numSections);

        /* again get all of secions */
        cpos = ConfOpenFile(confFile);
        if (cpos) {
            while (secNo <= numSections && ConfGetNextPair(cpos, &key, &val) != 0) {
                const char *sec = ConfGetSection(cpos);
                if (secNo==1 || strcmp(sec, secs[secNo-1])) {
                    int secSize = ConfCopySection(cpos, 0);
                    secs[secNo] = (char*) ConfMemAlloc(secSize, sizeof(char));
                    ConfCopySection(cpos, secs[secNo]);
                    secNo++;
                }
            }
            ConfCloseFile(cpos);
        }
        *sectionList = (void*) secs;
    }

    return numSections;
}


void ConfSectionListFree (void *sections)
{
    if (sections) {
        char ** secs = (char **) sections;
        int numSections = (int) (uintptr_t) secs[0];
        while(numSections>0) {
            ConfMemFree(secs[numSections--]);
        }
        ConfMemFree(secs);
    }
}


char * ConfSectionListGetAt (void *sections, int secIndex)
{
    char ** secs = (char **) sections;
    int numSections = (int) (uintptr_t) secs[0];
    if (secIndex < 0 || secIndex >= numSections) {
        return (char *) 0;
    }
    return secs[secIndex+1];
}


int ConfSectionParse (char *sectionName, char **family, char **qualifier)
{
    char *p, *q;

    if (! sectionName) {
        /* null string */
        return -1;
    }

    if (! sectionName[0]) {
        /* empty string */
        return 0;
    }

    *qualifier = 0;
    *family = sectionName;

    p = strchr(sectionName, (char)READCONF_SEC_SEMI);
    q = strrchr(sectionName, (char)READCONF_SEC_SEMI);

    if (p == q && p) {
        *p++ = 0;

        *qualifier = p;
        return 2;
    }

    return 1;
}


/**
 * 1  true
 * 0  false
 *
 * -1 error
 */
#ifdef _MSC_VER
#  define StrNCaseComp  strnicmp
#else
#  define StrNCaseComp  strncasecmp
#endif

int ConfParseBoolValue (const char *value, int defvalue)
{
    int c;

    if (! value || *value == '\0') {
        return defvalue;
    }

    c = (int) strnlen(value, 10);
    if (c > 8) {
        return (-1);
    }

    if (! StrNCaseComp("1", value, c) ||
        ! StrNCaseComp("yes", value, c) || ! StrNCaseComp("y", value, c) ||
        ! StrNCaseComp("on", value, c) || ! StrNCaseComp("open", value, c) ||
        ! StrNCaseComp("true", value, c) || ! StrNCaseComp("t", value, c)) {
        return 1;
    }

    if (! StrNCaseComp("0", value, c) ||
        ! StrNCaseComp("no", value, c) || ! StrNCaseComp("n", value, c) ||
        ! StrNCaseComp("off", value, c) || ! StrNCaseComp("close", value, c) ||
        ! StrNCaseComp("false", value, c) || ! StrNCaseComp("f", value, c)) {
        return 0;
    }

    return (-1);
}


/**
 * -----------------------------------------------------------
 *  NAME       ABBR. BYTES        NAME      ABBR.  BYTES
 * -----------------------------------------------------------
 *  bit        *bit   1/8         Byte        B      1
 *  kilobyte   KB    10^3 (1000)  kibibyte  KiB    2^10 (1024)
 *  megabyte   MB    10^6         mebibyte  MiB    2^20
 *  gigabyte   GB    10^9         gibibyte  GiB    2^30
 *  terabyte   TB    10^12        tebibyte  TiB    2^40
 *  petabyte   PB    10^15        pebibyte  PiB    2^50
 *  exabyte    EB    10^18        exbibyte  EiB    2^60
 *  zettabyte  ZB    10^21        zebibyte  ZiB    2^70
 *  yottabyte  YB    10^24        yobibyte  YiB    2^80
 * -----------------------------------------------------------
 */
double ConfParseSizeBytesValue(char *valuebuf, double defvalue, int *base, int *exponent)
{
    static const struct {
        char name[4];
        int base;
        int exponent;
    } namebytes[] = {
        {"B",   10,  0},
        {"KiB",  2, 10},
        {"KB",  10,  3},
        {"K",   10,  3},
        {"MiB",  2, 20},
        {"MB",  10,  6},
        {"M",   10,  6},
        {"GiB",  2, 30},
        {"GB",  10,  9},
        {"G",   10,  9},
        {"TiB",  2, 40},
        {"TB",  10, 12},
        {"T",   10, 12},
        {"PiB",  2, 50},
        {"PB",  10, 15},
        {"P",   10, 15},
        {"EiB",  2, 60},
        {"EB",  10, 18},
        {"E",   10, 18},
        {"ZiB",  2, 70},
        {"ZB",  10, 21},
        {"Z",   10, 21},
        {"YiB",  2, 80},
        {"YB",  10, 24},
        {"Y",   10, 24}
    };

    if (base && exponent) {
        *base = 10;
        *exponent = 0;
    }

    if (valuebuf) {
        size_t num = sizeof(namebytes)/sizeof(namebytes[0]);
        char *value = trim_whitespace(valuebuf);
        int len = (int) strnlen(value, 128);
        int end;

        for (end = len - 1; end >= 0; end--) {
            if (value[end] >= '0' && value[end] <= '9') {
                size_t i;
                double retbytes = defvalue;

                char *name = &value[end + 1];

                for (i = 0; i != num; i++) {
                    if (!memcmp(namebytes[i].name, name, len - end)) {
                        if (base && exponent) {
                            *base = namebytes[i].base;
                            *exponent = namebytes[i].exponent;
                        }

                        *name = 0;

                        retbytes = atof(value);

                        if (base && exponent) {
                            return retbytes;
                        } else {
                            return retbytes * pow(namebytes[i].base, namebytes[i].exponent);
                        }
                    }
                }
            }
        }
    }

    return defvalue;
}