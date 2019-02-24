/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 *
 * @author Rama, Meka(v.meka@samsung.com)
           Sangwoo, Park(sw5771.park@samsung.com)
           Jamie, Oh (jung-min.oh@samsung.com)
 * @date   2011-03-11
 *
 */

#ifndef ANDROID_SEC_HWC_UTILS_H_
#define ANDROID_SEC_HWC_UTILS_H_


#include <fcntl.h>
#include <errno.h>
#include <cutils/log.h>
#include <stdlib.h>
#include <linux/videodev.h>
#include <linux/videodev.h>
#include <linux/android_pmem.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "linux/fb.h"
#include <hardware/gralloc.h>
#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>


#include "s5p_fimc.h"
#include "s3c_lcd.h"
#include "s3c_mem.h"
#include "sec_format.h"
#include "sec_utils.h"
#include "sec_g2d.h"
#include "../libg2d/SecG2d.h"

//#define HWC_DEBUG
//#define SUB_TITLES_HWC

#define DEFAULT_LCD_WIDTH   (800)
#define DEFAULT_LCD_HEIGHT  (480)
#define DEFAULT_LCD_BPP     (32)

#define NUM_OF_WIN          (2)
#define NUM_OF_WIN_BUF      (2)
#define NUM_OF_MEM_OBJ      (1)

#if (NUM_OF_WIN_BUF < 2)
    #define ENABLE_FIMD_VSYNC
#endif

#define MAX_RESIZING_RATIO_LIMIT  (63)

#define USE_HW_PMEM

#define PMEM_SIZE (720 * 1280 * 2)

struct sec_rect {
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
};

struct sec_img {
    uint32_t w;
    uint32_t h;
    uint32_t format;
    uint32_t base;
    uint32_t offset;
    int      mem_id;
    int      mem_type;
};

inline int SEC_MIN(int x, int y) {
    return ((x < y) ? x : y);
}

inline int SEC_MAX(int x, int y) {
    return ((x > y) ? x : y);
}

struct s3c_mem_t {
    int                  fd;
    struct s3c_mem_alloc mem_alloc[NUM_OF_MEM_OBJ];
};

#ifdef USE_HW_PMEM
typedef struct __sec_pmem_alloc{
    int          fd;
    int          total_size;
    int          offset;
    unsigned int size;
    unsigned int virt_addr;
    unsigned int phys_addr;
}sec_pmem_alloc_t;

typedef struct __sec_pmem {
    int    pmem_master_fd;
    void  *pmem_master_base;
    int    pmem_total_size;
    sec_pmem_alloc_t sec_pmem_alloc[NUM_OF_MEM_OBJ];
}sec_pmem_t;

inline size_t roundUpToPageSize(size_t x) {
    return (x + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1);
}
#endif

struct hwc_win_info_t{
    int        fd;
    int        size;
    sec_rect   rect_info;
    uint32_t   addr[NUM_OF_WIN_BUF];
    int        buf_index;

    int        power_state;
    int        blending;
    int        layer_index;
    uint32_t   layer_prev_buf;
    int        status;
    int        vsync;

    struct fb_fix_screeninfo fix_info;
    struct fb_var_screeninfo var_info;
    struct fb_var_screeninfo lcd_info;
};

enum {
    HWC_WIN_FREE = 0,
    HWC_WIN_RESERVED,
};

enum {
    HWC_UNKNOWN_MEM_TYPE = 0,
    HWC_PHYS_MEM_TYPE,
    HWC_VIRT_MEM_TYPE,
};

struct hwc_context_t {
    hwc_composer_device_t device;

    /* our private state goes below here */
    struct hwc_win_info_t     win[NUM_OF_WIN];
    struct fb_var_screeninfo  lcd_info;
           s5p_fimc_t         fimc;
    struct s3c_mem_t          s3c_mem;
#ifdef USE_HW_PMEM
    sec_pmem_t                sec_pmem;
#endif
    unsigned int              num_of_fb_layer;
    unsigned int              num_of_hwc_layer;
    int                       num_2d_blit_layer;
};

typedef enum _LOG_LEVEL
{
    HWC_LOG_DEBUG,
    HWC_LOG_WARNING,
    HWC_LOG_ERROR,
} HWC_LOG_LEVEL;

#define SEC_HWC_LOG_TAG		"SECHWC_LOG"

#ifdef HWC_DEBUG
#define SEC_HWC_Log(a, ...)    ((void)_SEC_HWC_Log(a, SEC_HWC_LOG_TAG, __VA_ARGS__))
#else
#define SEC_HWC_Log(a, ...)                                         \
    do {                                                            \
        if (a == HWC_LOG_ERROR)                                     \
            ((void)_SEC_HWC_Log(a, SEC_HWC_LOG_TAG, __VA_ARGS__)); \
    } while (0)
#endif

extern void _SEC_HWC_Log(HWC_LOG_LEVEL logLevel, const char *tag, const char *msg, ...);

/* copied from gralloc module ..*/
typedef struct
{
    native_handle_t base;

    /* These fields can be sent cross process. They are also valid
     * to duplicate within the same process.
     *
     * A table is stored within psPrivateData on gralloc_module_t (this
     * is obviously per-process) which maps stamps to a mapped
     * PVRSRV_CLIENT_MEM_INFO in that process. Each map entry has a lock
     * count associated with it, satisfying the requirements of the
     * Android API. This also prevents us from leaking maps/allocations.
     *
     * This table has entries inserted either by alloc()
     * (alloc_device_t) or map() (gralloc_module_t). Entries are removed
     * by free() (alloc_device_t) and unmap() (gralloc_module_t).
     *
     * As a special case for framebuffer_device_t, framebuffer_open()
     * will add and framebuffer_close() will remove from this table.
     */

#define IMG_NATIVE_HANDLE_NUMFDS 1
    /* The `fd' field is used to "export" a meminfo to another process.
     * Therefore, it is allocated by alloc_device_t, and consumed by
     * gralloc_module_t. The framebuffer_device_t does not need a handle,
     * and the special value IMG_FRAMEBUFFER_FD is used instead.
     */
    int fd;

#if 1
    int format;
    int magic;
    int flags;
    int size;
    int offset;
    int base_addr;
    int stride;
#define IMG_NATIVE_HANDLE_NUMINTS ((sizeof(uint64_t) / sizeof(int)) + 4 + 7)
#else
#define IMG_NATIVE_HANDLE_NUMINTS ((sizeof(IMG_UINT64) / sizeof(int)) + 4)
#endif
    /* A KERNEL unique identifier for any exported kernel meminfo. Each
     * exported kernel meminfo will have a unique stamp, but note that in
     * userspace, several meminfos across multiple processes could have
     * the same stamp. As the native_handle can be dup(2)'d, there could be
     * multiple handles with the same stamp but different file descriptors.
     */
    uint64_t ui64Stamp;

    /* We could live without this, but it lets us perform some additional
     * validation on the client side. Normally, we'd have no visibility
     * of the allocated usage, just the lock usage.
     */
    int usage;

    /* In order to do efficient cache flushes we need the buffer dimensions
     * and format. These are available on the android_native_buffer_t,
     * but the platform doesn't pass them down to the graphics HAL.
     *
     * TODO: Ideally the platform would be modified to not require this.
     */
    int width;
    int height;
    int bpp;
}
__attribute__((aligned(sizeof(int)),packed)) sec_native_handle_t;

int window_open       (struct hwc_win_info_t *win, int id);
int window_close      (struct hwc_win_info_t *win);
int window_set_pos    (struct hwc_win_info_t *win);
int window_get_info   (struct hwc_win_info_t *win);
int window_pan_display(struct hwc_win_info_t *win);
int window_show       (struct hwc_win_info_t *win);
int window_hide       (struct hwc_win_info_t *win);
int window_get_global_lcd_info(int fd, struct fb_var_screeninfo *lcd_info);

int createFimc (s5p_fimc_t *fimc);
int destroyFimc(s5p_fimc_t *fimc);
int runFimc(struct hwc_context_t *ctx,
            struct sec_img *src_img, struct sec_rect *src_rect,
            struct sec_img *dst_img, struct sec_rect *dst_rect,
            uint32_t transform);

int runG2d(struct hwc_context_t *ctx,
            G2dRect *src_rect,  G2dRect *dst_rect,
            uint32_t transform);

int createMem (struct s3c_mem_t *mem, unsigned int index, unsigned int size);
int destroyMem(struct s3c_mem_t *mem);
int checkMem  (struct s3c_mem_t *mem, unsigned int index, unsigned int size);

#ifdef USE_HW_PMEM
int createPmem (sec_pmem_t *pm, unsigned int size);
int destroyPmem(sec_pmem_t *pm);
int checkPmem  (sec_pmem_t *pm, unsigned int index, unsigned int size);
#endif

#endif /* ANDROID_SEC_HWC_UTILS_H_*/
