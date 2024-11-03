/******************************************************************************
* Copyright © 2024-2035 Light Zhang <mapaware@hotmail.com>, MapAware, Inc.
* ALL RIGHTS RESERVED.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
******************************************************************************/
/*
** @file readconf.h
** @brief read ini file api for Linux and Windows.
**
** @author mapaware@hotmail.com
** @copyright © 2024-2030 mapaware.top All Rights Reserved.
** @version 1.0.4
**
** @since 2012-05-21 12:37:44
** @date 2024-11-04 01:11:01
**
** @note
**   The first file you should included.
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


#ifndef READCONF_LINESIZE_MAX
  # define READCONF_LINESIZE_MAX     4096
#endif

#ifndef READCONF_KEYLEN_MAX
  # define READCONF_KEYLEN_MAX       60
#endif

#ifndef READCONF_SECNAME_MAX
  # define READCONF_SECNAME_MAX     READCONF_KEYLEN_MAX
#endif

typedef struct _conf_position_t* CONF_position;


typedef struct {
    int count;

    char** keys;
    char** values;

    int* keylens;
    int* valuelens;
} ConfVariables;


extern void* ConfMemAlloc (int numElems, int sizeElem);

extern void* ConfMemRealloc (void *oldBuffer, int oldSize, int newSize);

extern void ConfMemFree (void *pBuffer);

extern void ConfVariablesClear(ConfVariables* env);

extern char** ConfStringArrayNew(int numElems);

extern void ConfStringArrayFree(char** strs, int numElems);

extern char* ConfMemCopyString(const char* src, size_t chlen);

extern CONF_position ConfOpenFile (const char *conf);

extern const char * ConfGetEncode (CONF_position cpos);

extern void ConfCloseFile (CONF_position cpos);

extern char *ConfGetNextPair (CONF_position cpos, char **key, char **val);

extern char *ConfGetFirstPair (CONF_position cpos, char **key, char **val);

extern const char *ConfGetSection (CONF_position cpos);

extern READCONF_RESULT ConfCopySection (CONF_position cpos, char *secName);

extern int ConfReadSectionVariables (const char* confFile, const char* sectionName, ConfVariables *outVars);

extern int ConfVariablesReplace (const char* input, int inlen, const ConfVariables* variables, char** output);

extern int ConfReadValue (const char *confFile, const char *sectionName, const char *keyName, char *valbuf, size_t maxbufsize);

extern int ConfReadValueRef (const char *confFile, const char *sectionName, const char *keyName, char **ppRefVal);

extern int ConfReadValueParsed (const char *confFile, const char *family, const char *qualifier, const char *key, char *valbuf, size_t maxbufsize);

extern int ConfReadValueParsed2(const char* confFile, const char* family, const char* qualifier, size_t qualifierlen, const char* key, char* valbuf, size_t maxbufsize);

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
