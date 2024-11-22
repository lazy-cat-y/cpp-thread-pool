/*****************************************************************************
Copyright (c) 2024 Haoxin Yang

Author: Haoxin Yang (lazy-cat-y)

Licensed under the MIT License. You may obtain a copy of the License at:
https://opensource.org/licenses/MIT

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

File: m-define
*****************************************************************************/


/** @file /include/m-define.h
 *  @brief Contains the macro definitions.
 *  @author H. Y.
 *  @date 11/19/2024
 */

#ifndef M_DEFINE_H
#define M_DEFINE_H

#define WORKER_STATUS_CREATED 0
#define WORKER_STATUS_IDLE 1
#define WORKER_STATUS_RUNNING 2
#define WORKER_STATUS_STOPPING 3
#define WORKER_STATUS_STOPPED 4

// Thread pool status
#define THREAD_POOL_STATUS_IDLE 0
#define THREAD_POOL_STATUS_RUNNING 1
#define THREAD_POOL_STATUS_STOPPING 2
#define THREAD_POOL_STATUS_STOPPED 3

#endif  // M_DEFINE_H