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
 * @filename   readconf.h
 *  read ini file api for Linux and Windows.
 *
 * @author     Liang Zhang <350137278@qq.com>
 * @version    0.0.9
 * @create     2012-05-21
 * @update     2021-04-02 15:32:50
 */
#ifndef _READCONF_H__
#define _READCONF_H__

#if defined(__cplusplus)
extern "C" {
#endif


#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#ifndef _MSC_VER
  #include <unistd.h>
#endif

typedef int READCONF_BOOL;
#define READCONF_TRUE   1
#define READCONF_FALSE  0

typedef int READCONF_RESULT;
#define READCONF_RET_SUCCESS       (0)
#define READCONF_RET_ERROR        (-1)
#define READCONF_RET_OUTMEM       (-4)

#define READCONF_SEC_BEGIN         91   /* '[' */
#define READCONF_SEC_END           93   /* ']' */
#define READCONF_SEPARATOR         61   /* '=' */
#define READCONF_NOTE_CHAR         35   /* '#' */
#define READCONF_SEC_SEMI          58   /* ':' */


#ifndef READCONF_MAX_LINESIZE
    # define READCONF_MAX_LINESIZE     4096
#endif

#ifndef READCONF_MAX_SECNAME
    # define READCONF_MAX_SECNAME      READCONF_MAX_LINESIZE
#endif


typedef struct _conf_position_t * CONF_position;

extern void* ConfMemAlloc (int numElems, int sizeElem);

extern void* ConfMemRealloc (void *oldBuffer, int oldSize, int newSize);

extern void ConfMemFree (void *pBuffer);

extern char* ConfMemCopyString (char **dst, const char *src);

extern CONF_position ConfOpenFile (const char *conf);

extern void ConfCloseFile (CONF_position cpos);

extern char *ConfGetNextPair (CONF_position cpos, char **key, char **val);

extern char *ConfGetFirstPair (CONF_position cpos, char **key, char **val);

extern const char *ConfGetSection (CONF_position cpos);

extern READCONF_RESULT ConfCopySection (CONF_position cpos, char *secName);

extern int ConfReadValue (const char *confFile, const char *sectionName, const char *keyName, char *valbuf, size_t maxbufsize);

extern int ConfReadValueRef (const char *confFile, const char *sectionName, const char *keyName, char **ppRefVal);

extern int ConfReadValueParsed (const char *confFile, const char *family, const char *qualifier, const char *key, char *valbuf, size_t maxbufsize);

extern int ConfReadValueParsedAlloc (const char *confFile, const char *family, const char *qualifier, const char *key, char **value);

extern int ConfGetSectionList (const char *confFile, void **sectionList);

extern void ConfSectionListFree (void *sectionList);

extern char * ConfSectionListGetAt (void *sectionList, int secIndex);

extern READCONF_BOOL ConfSectionGetFamily (const char *sectionName, char *family);

extern READCONF_BOOL ConfSectionGetQualifier (const char *sectionName, char *qualifier);

extern int ConfSectionParse (char *sectionName, char **family, char **qualifier);

extern int ConfParseBoolValue (const char *value, int defvalue);

extern double ConfParseSizeBytesValue (char *valuebuf, double defvalue, int *base, int *exponent);


#if defined(__cplusplus)
}
#endif

#endif /* _READCONF_H__ */
