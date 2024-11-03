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
** @file cssparse.h
** @brief
**
** @author mapaware@hotmail.com
** @copyright © 2024-2030 mapaware.top All Rights Reserved.
** @version 0.0.3
**
** @since 2024-10-08 23:51:27
** @date 2024-11-04 00:26:19
**
** @note
*/
#ifndef CSS_PARSE_H__
#define CSS_PARSE_H__

#if defined(__cplusplus)
extern "C"
{
#endif


typedef struct CssKeyField *CssKeyArray, *CssKeyArrayNode;


// NOTE:
//   Input CSS String's Max Length  < 1024*1024 bytes
//   Max Key Index = 4095
//   Key's or Value's Max Length = 255 bytes
// For Example:
//   char cssString[1024*1024];
//   int keyIndex[4096];
//   char keyValue[256];
// NEVER CHANGE BELOW DEFINITIONS!
#define CSS_STRING_BSIZE_MAX_1048576     0x100000   // 20bit: 最长 1048575 (1M - 1) 字节: (1024*1024 - 1)
#define CSS_KEY_FLAGS_INVALID_65536      0x10000    // 16bit: 最大 65535
#define CSS_KEYINDEX_INVALID_4096        0x1000     // 12bit: 最多 4096 个 Keys, 索引=[0 ... 4095 (0xFFF)]
#define CSS_VALUELEN_INVALID_256         0x100      // 8bit:  键值的长度最大 255 个字符: CSS_VALUELEN_INVALID - 1


typedef struct CssStringBuffer {
    unsigned int sbsize;
    unsigned int sblen;
    char sbbuf[0];
} *CssString;


typedef enum {
    css_type_none = 0,
    css_type_key = 1,
    css_type_value = 2,
    css_type_class = 46,    // '.' class
    css_type_id = 35,       // '#' id
    css_type_asterisk = 42  // '*' any
} CssKeyType;


// Max up to 16 Bits(Flags)
typedef enum {
    css_bitflag_none = 0,
    css_bitflag_readonly = 1,      // 只读 2^0
    css_bitflag_hidden = 2,        // 隐藏 2^1
    css_bitflag_hilight = 4,       // 掠过: mouse move in
    css_bitflag_pickup = 8,        // 拾取
    css_bitflag_dragging = 16,     // 拖动中
    css_bitflag_deleting = 32,     // 删除中
    css_bitflag_fault = 64,        // 错误
    css_bitflag_flash = 128,       // 闪烁
    css_bitflag_zoomin = 256,      // 视图窗口放大
    css_bitflag_zoomout = 512,     // 视图窗口缩小
    css_bitflag_panning = 1024     // 视图窗口移动
} CssBitFlag;


extern CssString CssStringNew(const char* cssStr, size_t cssStrLen);
extern CssString CssStringNewFromFile(FILE *cssfile);
extern void CssStringFree(CssString cssString);

extern CssKeyArray CssStringParse(CssString cssString);
extern void CssKeyArrayFree(CssKeyArray keys);

extern const char * CssKeyArrayGetString(const CssKeyArray cssKeys, unsigned int offset);

extern int CssKeyArrayGetSize(const CssKeyArray cssKeys);
extern int CssKeyArrayGetUsed(const CssKeyArray cssKeys);

extern const CssKeyArrayNode CssKeyArrayGetNode(const CssKeyArray cssKeys, int index);
extern CssKeyType CssKeyGetType(const CssKeyArrayNode cssKey);
extern int CssKeyGetFlag(const CssKeyArrayNode cssKey);
extern int CssKeyOffsetLength(const CssKeyArrayNode cssKeyNode, int* bOffset);
extern int CssKeyTypeIsClass(const CssKeyArrayNode cssKeyNode);
extern int CssKeyFlagToString(int keyflag, char* outbuf, size_t buflen);

// Only for class node, to get it's {} node index or else returns: -1
extern int CssClassGetKeyIndex(const CssKeyArrayNode cssClassKey);

// 完全使用头文件 API, 展示了如何使用 cssparse 解析和输出 CSS
extern void CssKeyArrayPrint(const CssKeyArray cssKeys, FILE* outfd);

// 查询指定名称的 class 节点
extern int CssKeyArrayQueryClass(const CssKeyArray cssKeys, CssKeyType classType, const char* className, int classNameLen, CssKeyArrayNode classNodes[32]);

#ifdef __cplusplus
}
#endif
#endif /* CSS_PARSE_H__ */