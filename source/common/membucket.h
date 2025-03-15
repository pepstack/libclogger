/***********************************************************************************************
* Copyright © 2024-2025 mapaware.top, 350137278@qq.com. All rights reserved.                   *
*                                                                                              *
* This software is a proprietary product of [mapaware.top]  (Email: 350137278@qq.com).  It is  *
* protected by copyright laws and international treaties. The following restrictions apply to  *
* the use, reproduction, and distribution of this software:                                    *
*                                                                                              *
* 1. Use                                                                                       *
*   This software may only be used in accordance with the terms and conditions of the software *
*   license agreement provided by [mapaware.top]. Any unauthorized use is strictly prohibited. *
*                                                                                              *
* 2.Reproduction                                                                               *
*   No part of this software may be reproduced,stored in a retrieval system, or transmitted in *
*   any form or by any means (electronic, mechanical, photocopying, recording, or otherwise)   *
*   without the prior written permission of [mapaware.top].                                    *
*                                                                                              *
* 3.Distribution                                                                               *
*   Redistribution of this software, whether in its original form or modified, is strictly     *
*   prohibited without the express written consent of [mapaware.top].                          *
*                                                                                              *
* Infringers of this copyright will be subject to civil and criminal penalties.                *
*                                                                                              *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,EXPRESS OR IMPLIED, INCLUDING *
* BUT NOT LIMITED TO THE WARRANTIES OF  MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND *
* NONINFRINGEMENT.  IN NO EVENT SHALL [mapaware.top] BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  TORT OR OTHERWISE,  ARISING FROM, OUT OF OR IN *
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                   *
***********************************************************************************************/
/*
** @file membucket.h
**   内存桶池实现，用于高效管理固定大小的内存块分配与释放。
** @author Liang Zhang <350137278@qq.com>
** @version 0.0.1
** @since 2025-03-14 02:54:00
** @date
**
** @usage
    membucket_pool pool = membucket_pool_create(256, 4096);
    for (uint32_t i = 0; i < LOOPS; ++i) {
        void* pMemory = membucket_pool_alloc(pool, (i + 1) % 4096);
        if (!pMemory) {
            printf("[%d] failed to fetch memory: %d B\n", i, (i + 1) % 4096);
        }
        membucket_pool_free(pool, pMemory);
    }
    membucket_pool_destroy(pool);
*/
#ifndef MEM_BUCKET_H_
#define MEM_BUCKET_H_

#include <assert.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct membucket_pool_t* membucket_pool;


/**
 * @brief 创建内存池
 * @param bktsize 单个桶的大小（单位：字节）
 * @param numbkts 桶的数量
 * @return 成功返回内存池指针，失败返回 NULL
 */
extern membucket_pool membucket_pool_create(uint32_t bktsize, uint32_t numbkts);

/**
 * @brief 销毁内存池
 * @param pool 内存池指针
 */
extern void membucket_pool_destroy(membucket_pool pool);

/**
 * @brief 从内存池中分配内存
 * @param pool 内存池指针
 * @param bsize 请求的内存大小（单位：字节）
 * @return 成功返回分配的内存指针，失败返回 NULL
 */
extern void* membucket_pool_alloc(membucket_pool pool, uint32_t bsize);

/**
 * @brief 释放内存到内存池
 * @param pool 内存池指针
 * @param pMemory 待释放的内存指针
 */
extern void membucket_pool_free(membucket_pool pool, void* pMemory);

#ifdef    __cplusplus
}
#endif
#endif /* MEM_BUCKET_H_ */