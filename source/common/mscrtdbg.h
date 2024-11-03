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
** @file mscrtdbg.h
** @brief MSVC _CRTDBG_MAP_ALLOC Marco definition.
**
** @author mapaware@hotmail.com
** @copyright © 2024-2030 mapaware.top All Rights Reserved.
** @version 0.0.29
**
** @since 2019-09-30 12:37:44
** @date 2024-11-03 23:19:44
**
** @note
**   The file should be included at first line.
*/
#ifndef MSCRTDBG_H__
#define MSCRTDBG_H__

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_MSC_VER)
    // warning C4996: 'vsnprintf': This function or variable may be unsafe.
    // Consider using vsnprintf_s instead.
    //  To disable deprecation, use _CRT_SECURE_NO_WARNINGS
    # pragma warning(disable:4996)

    # if defined(_DEBUG)
        /** memory leak auto-detect in MSVC
         * https://blog.csdn.net/lyc201219/article/details/62219503
         */
        # include <crtdbg.h>
        # ifndef _CRTDBG_MAP_ALLOC
            # define _CRTDBG_MAP_ALLOC
        # endif
        # include <stdlib.h>
        # include <malloc.h>

        # ifndef WINDOWS_CRTDBG_ON
            # define WINDOWS_CRTDBG_ON  _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
        # endif
    # else
        # ifdef _CRTDBG_MAP_ALLOC
            # undef _CRTDBG_MAP_ALLOC
        # endif

        # include <stdlib.h>
        # include <malloc.h>

        # ifndef WINDOWS_CRTDBG_ON
            # define WINDOWS_CRTDBG_ON
        # endif
    # endif
#endif

#if defined(__cplusplus)
}
#endif

#endif /* MSCRTDBG_H__ */