/*******************************************************************************
* Copyright © 2024-2025 Light Zhang <mapaware@hotmail.com>, MapAware, Inc.     *
* ALL RIGHTS RESERVED.                                                         *
*                                                                              *
* PERMISSION IS HEREBY GRANTED, FREE OF CHARGE, TO ANY PERSON OR ORGANIZATION  *
* OBTAINING A COPY OF THE SOFTWARE COVERED BY THIS LICENSE TO USE, REPRODUCE,  *
* DISPLAY, DISTRIBUTE, EXECUTE, AND TRANSMIT THE SOFTWARE, AND TO PREPARE      *
* DERIVATIVE WORKS OF THE SOFTWARE, AND TO PERMIT THIRD - PARTIES TO WHOM THE  *
* SOFTWARE IS FURNISHED TO DO SO, ALL SUBJECT TO THE FOLLOWING :               *
*                                                                              *
* THE COPYRIGHT NOTICES IN THE SOFTWARE AND THIS ENTIRE STATEMENT, INCLUDING   *
* THE ABOVE LICENSE GRANT, THIS RESTRICTION AND THE FOLLOWING DISCLAIMER, MUST *
* BE INCLUDED IN ALL COPIES OF THE SOFTWARE, IN WHOLE OR IN PART, AND ALL      *
* DERIVATIVE WORKS OF THE SOFTWARE, UNLESS SUCH COPIES OR DERIVATIVE WORKS ARE *
* SOLELY IN THE FORM OF MACHINE - EXECUTABLE OBJECT CODE GENERATED BY A SOURCE *
* LANGUAGE PROCESSOR.                                                          *
*                                                                              *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     *
* FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON - INFRINGEMENT.IN NO EVENT   *
* SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE    *
* FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,  *
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER  *
* DEALINGS IN THE SOFTWARE.                                                    *
*******************************************************************************/
/**
* uthash.h
*
Copyright (c) 2003-2020, Troy D. Hanson
http://troydhanson.github.com/uthash/
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/
#ifndef UTHASH_INCL_H__
#define UTHASH_INCL_H__

#ifdef MEMAPI_USE_LIBJEMALLOC
  # define uthash_malloc(sz) je_malloc(sz)
  # define uthash_free(ptr,sz) je_free(ptr)
#endif

#include "uthash/uthash.h"


#define HASH_FIND_STR_LEN(head,findstr,findstrlen,out)                           \
do {                                                                             \
    unsigned _uthash_hfstr_keylen = (unsigned)(findstrlen);                      \
    HASH_FIND(hh, head, findstr, _uthash_hfstr_keylen, out);                     \
} while (0)

#define HASH_ADD_STR_LEN(head,strfield,strfieldlen,add)                          \
do {                                                                             \
    unsigned _uthash_hastr_keylen = (unsigned)(strfieldlen);                     \
    HASH_ADD(hh, head, strfield[0], _uthash_hastr_keylen, add);                  \
} while (0)

#define HASH_REPLACE_STR_LEN(head,strfield,strfieldlen,add,replaced)             \
do {                                                                             \
    unsigned _uthash_hrstr_keylen = (unsigned)(strfieldlen);                     \
    HASH_REPLACE(hh, head, strfield[0], _uthash_hrstr_keylen, add, replaced);    \
} while (0)

#endif /* UTHASH_INCL_H__ */
