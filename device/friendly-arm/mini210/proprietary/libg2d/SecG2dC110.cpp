/*
 * Copyright Samsung Electronics Co.,LTD.
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * C210 G2D Porting MODULE
 *
 * Author  : Sangwoo, Park(sw5771.park@samsung.com)
 * Date    : 1 June 2010
 * Purpose : C110 Porting
 */

#define LOG_NDEBUG 0
#define LOG_TAG "SecG2dC110"
#include <utils/Log.h>

#include "SecG2dC110.h"

namespace android
{

Mutex   SecG2dC110::m_instanceLock;
int     SecG2dC110::m_curSecG2dC110Index = 0;
int     SecG2dC110::m_numOfInstance = 0;
SecG2d *SecG2dC110::m_ptrG2dList[NUMBER_G2D_LIST] = {NULL, };

SecG2dC110::SecG2dC110()
         : m_fd(0),
           m_g2dVirtAddr(NULL),
           m_g2dPhysAddr(0),
           m_g2dSize(0),
           m_srcVirtAddr(NULL),
           m_srcPhysAddr(0),
           m_srcSize(0),
           m_dstVirtAddr(NULL),
           m_dstPhysAddr(0),
           m_dstSize(0)
{
    m_lock = new Mutex(Mutex::SHARED, "SecG2dC110");
}

SecG2dC110::~SecG2dC110()
{
    delete m_lock;
}

SecG2d* SecG2dC110::createInstance(void)
{
    Mutex::Autolock autolock(m_instanceLock);

    SecG2d *ptrG2d = NULL;

    // Using List like RingBuffer...
    for (int i = m_curSecG2dC110Index; i < NUMBER_G2D_LIST; i++) {
        if (m_ptrG2dList[i] == NULL)
            m_ptrG2dList[i] = new SecG2dC110;

        if (m_ptrG2dList[i]->flagCreate() == false) {
            if (m_ptrG2dList[i]->create() == false) {
                LOGE("%s::create(%d) fail", __func__, i);
                goto CREATE_INSTANCE_END;
            }
            else
                m_numOfInstance++;
        }

        if (i < NUMBER_G2D_LIST - 1)
            m_curSecG2dC110Index = i + 1;
        else
            m_curSecG2dC110Index = 0;

        ptrG2d = m_ptrG2dList[i];
        goto CREATE_INSTANCE_END;
    }

CREATE_INSTANCE_END :

    return ptrG2d;
}

void SecG2dC110::destroyInstance(SecG2d *g2d)
{
    Mutex::Autolock autolock(m_instanceLock);

    for (int i = 0; i < NUMBER_G2D_LIST; i++) {
        if (   m_ptrG2dList[i] != NULL
            && m_ptrG2dList[i] == g2d) {
            if (   m_ptrG2dList[i]->flagCreate() == true
                && m_ptrG2dList[i]->destroy()    == false) {
                LOGE("%s::destroy() fail", __func__);
            } else {
                SecG2dC110 *tempSecG2dC110 = (SecG2dC110 *)m_ptrG2dList[i];
                delete tempSecG2dC110;
                m_ptrG2dList[i] = NULL;

                m_numOfInstance--;
            }
            break;
        }
    }
}

void SecG2dC110::destroyAllInstance(void)
{
    Mutex::Autolock autolock(m_instanceLock);

    for (int i = 0; i < NUMBER_G2D_LIST; i++) {
        if (m_ptrG2dList[i] != NULL) {
            if (   m_ptrG2dList[i]->flagCreate() == true
                && m_ptrG2dList[i]->destroy()    == false) {
                LOGE("%s::destroy() fail", __func__);
            } else {
                SecG2dC110 *tempSecG2dC110 = (SecG2dC110 *)m_ptrG2dList[i];
                delete tempSecG2dC110;
                m_ptrG2dList[i] = NULL;
            }
        }
    }
}

bool SecG2dC110::t_create(void)
{
    bool ret = true;

    if (m_createG2d() == false) {
        LOGE("%s::m_createG2d() fail", __func__);

        if (m_destroyG2d() == false)
            LOGE("%s::m_destroyG2d() fail", __func__);

        ret = false;
    }

    return ret;
}

bool SecG2dC110::t_destroy(void)
{
    bool ret = true;

    if (m_destroyG2d() == false) {
        LOGE("%s::m_destroyG2d() fail", __func__);
        ret = false;
    }

    return ret;
}

bool SecG2dC110::t_stretch(G2dRect *src, G2dRect *dst, G2dClip *clip, G2dFlag *flag)
{
#ifdef CHECK_G2D_C110_PERFORMANCE
    #define     NUM_OF_STEP (10)
    StopWatch   stopWatch("CHECK_G2D_C110_PERFORMANCE");
    const char *stopWatchName[NUM_OF_STEP];
    nsecs_t     stopWatchTime[NUM_OF_STEP];
    int         stopWatchIndex = 0;
#endif // CHECK_G2D_C110_PERFORMANCE

    G2dRect *srcMidRect = src;
    G2dRect *dstMidRect = dst;
    unsigned int srcMidRectSize = 0;
    unsigned int dstMidRectSize = 0;

    // optimized size
    if (src && src->phys_addr == 0) {
        G2dRect srcTempMidRect = {0, 0, src->w, src->h, src->w, src->h, src->color_format, m_srcPhysAddr, m_srcVirtAddr};

        // use optimized size
        srcMidRect     = &srcTempMidRect;
        srcMidRectSize = t_frameSize(srcMidRect);

        if (m_srcSize < srcMidRectSize) {
            LOGE("%s::src size is too big fail", __func__);
            return false;
        }
    }

    G2dRect dstTempMidRect = {0, 0, dst->w, dst->h, dst->w, dst->h, dst->color_format, m_dstPhysAddr, m_dstVirtAddr};
    if (dst->phys_addr == 0) {
        // use optimized size
        dstMidRect = &dstTempMidRect;
        dstMidRectSize = t_frameSize(dstMidRect);

        if (m_dstSize < dstMidRectSize) {
            LOGE("%s::src size is too big fail", __func__);
            return false;
        }
    }

#ifdef CHECK_G2D_C110_PERFORMANCE
    stopWatchName[stopWatchIndex] = "StopWatch Ready";
    stopWatchTime[stopWatchIndex] = stopWatch.elapsedTime();
    stopWatchIndex++;
#endif // CHECK_G2D_C110_PERFORMANCE

#ifdef G2D_NONE_BLOCKING_MODE
    if (m_pollG2d(&m_poll) == false) {
        LOGE("%s::m_pollG2d 0() fail", __func__);
        return false;
    }
#endif

    if (src && src->phys_addr == 0) {
        if (t_copyFrame(src, srcMidRect) == false) {
            LOGE("%s::t_copyFrame() fail", __func__);
            return false;
        }

#ifdef CHECK_G2D_C110_PERFORMANCE
        stopWatchName[stopWatchIndex] = "Src Copy";
        stopWatchTime[stopWatchIndex] = stopWatch.elapsedTime();
        stopWatchIndex++;
#endif // CHECK_G2D_C110_PERFORMANCE

        if (m_cleanG2d(srcMidRect->phys_addr, srcMidRectSize) == false) {
            LOGE("%s::m_cleanG2d() fail", __func__);
            return false;
        }

#ifdef CHECK_G2D_C110_PERFORMANCE
        stopWatchName[stopWatchIndex] = "Src CacheClean";
        stopWatchTime[stopWatchIndex] = stopWatch.elapsedTime();
        stopWatchIndex++;
#endif // CHECK_G2D_C110_PERFORMANCE
    }

    if (dst->phys_addr == 0) {
        bool flagDstCopy = false;

        if (flag->alpha_val <= ALPHA_MAX) // <= 255
            flagDstCopy = true;

        if (flagDstCopy == true) {
            if (t_copyFrame(dst, dstMidRect) == false) {
                LOGE("%s::t_copyFrame() fail", __func__);
                return false;
            }

#ifdef CHECK_G2D_C110_PERFORMANCE
            stopWatchName[stopWatchIndex] = "Dst Copy";
            stopWatchTime[stopWatchIndex] = stopWatch.elapsedTime();
            stopWatchIndex++;
#endif // CHECK_G2D_C110_PERFORMANCE

            if (m_cleanG2d(dstMidRect->phys_addr, dstMidRectSize) == false) {
                LOGE("%s::m_cleanG2d() fail", __func__);
                return false;
            }

#ifdef CHECK_G2D_C110_PERFORMANCE
            stopWatchName[stopWatchIndex] = "Dst CacheClean";
            stopWatchTime[stopWatchIndex] = stopWatch.elapsedTime();
            stopWatchIndex++;
#endif // CHECK_G2D_C110_PERFORMANCE
        }
    }

    if (m_doG2d(srcMidRect, dstMidRect, flag) == false) {
        LOGE("%s::m_doG2d fail", __func__);
        return false;
    }

#ifdef CHECK_G2D_C110_PERFORMANCE
    stopWatchName[stopWatchIndex] = "G2D(HW) Run";
    stopWatchTime[stopWatchIndex] = stopWatch.elapsedTime();
    stopWatchIndex++;
#endif // CHECK_G2D_C110_PERFORMANCE

    if (dst->phys_addr == 0) {
        if (m_invalidG2d(dstMidRect->phys_addr, dstMidRectSize) == false) {
            LOGE("%s::m_invalidG2d() fail", __func__);
            return false;
        }

#ifdef CHECK_G2D_C110_PERFORMANCE
        stopWatchName[stopWatchIndex] = "Dst Cache Invalidate";
        stopWatchTime[stopWatchIndex] = stopWatch.elapsedTime();
        stopWatchIndex++;
#endif // CHECK_G2D_C110_PERFORMANCE

#ifdef G2D_NONE_BLOCKING_MODE
        if (m_pollG2d(&m_poll) == false) {
            LOGE("%s::m_pollG2d 1() fail", __func__);
            return false;
        }
#endif

#ifdef CHECK_G2D_C110_PERFORMANCE
        stopWatchName[stopWatchIndex] = "Poll";
        stopWatchTime[stopWatchIndex] = stopWatch.elapsedTime();
        stopWatchIndex++;
#endif // CHECK_G2D_C110_PERFORMANCE

        if (t_copyFrame(dstMidRect, dst) == false) {
            LOGE("%s::t_copyFrame() fail", __func__);
            return false;
        }

#ifdef CHECK_G2D_C110_PERFORMANCE
        stopWatchName[stopWatchIndex] = "Dst Copy";
        stopWatchTime[stopWatchIndex] = stopWatch.elapsedTime();
        stopWatchIndex++;
#endif // CHECK_G2D_C110_PERFORMANCE
    }
#ifdef G2D_NONE_BLOCKING_MODE
    else {
        if (m_pollG2d(&m_poll) == false) {
            LOGE("%s::m_pollG2d 2() fail", __func__);
            return false;
        }
    }
#endif

#ifdef CHECK_G2D_C110_PERFORMANCE
    m_printSecG2dC110Performance(src, dst, stopWatchIndex, stopWatchName, stopWatchTime);
#endif // CHECK_G2D_C110_PERFORMANCE

    return true;
}

bool SecG2dC110::t_lock(void)
{
    m_lock->lock();
    return true;
}

bool SecG2dC110::t_unLock(void)
{
    m_lock->unlock();
    return true;
}

bool SecG2dC110::m_createG2d(void)
{
    void *mmap_base;

    if (m_fd != 0) {
        LOGE("%s::m_fd(%d) is not 0 fail", __func__, m_fd);
        return false;
    }

#ifdef G2D_NONE_BLOCKING_MODE
    m_fd = open(SEC_G2D_DEV_NAME, O_RDWR | O_NONBLOCK);
#else
    m_fd = open(SEC_G2D_DEV_NAME, O_RDWR);
#endif
    if (m_fd < 0) {
        LOGE("%s::open(%s) fail(%s)", __func__, SEC_G2D_DEV_NAME, strerror(errno));
        m_fd = 0;
        return false;
    }

#ifdef G2D_NONE_BLOCKING_MODE
    memset(&m_poll, 0, sizeof(m_poll));
    m_poll.fd     = m_fd;
    m_poll.events = POLLOUT | POLLERR;
#endif

    unsigned int g2d_phys_addr = 0;
    if (ioctl(m_fd, G2D_GET_MEMORY, &g2d_phys_addr) < 0) {
        LOGE("%s::G2D_GET_MEMORY fail", __func__);
        return false;
    }

    unsigned int g2d_total_size = 0;
    if (ioctl(m_fd, G2D_GET_MEMORY_SIZE, &g2d_total_size) < 0) {
        LOGE("%s::G2D_GET_MEMORY_SIZE fail", __func__);
        return false;
    }

    mmap_base = mmap(0, g2d_total_size, PROT_READ|PROT_WRITE, MAP_SHARED, m_fd, 0);
    if (mmap_base == MAP_FAILED) {
        LOGE("%s::mmap failed : %d (%s)", __func__, errno, strerror(errno));
        return false;
    }

    m_g2dVirtAddr = (char *)mmap_base;
    m_g2dPhysAddr = g2d_phys_addr;
    m_g2dSize     = g2d_total_size;

    m_srcVirtAddr = m_g2dVirtAddr;
    m_srcPhysAddr = m_g2dPhysAddr;
    m_srcSize     = m_g2dSize >> 1;

    m_dstVirtAddr = m_srcVirtAddr + m_srcSize;
    m_dstPhysAddr = m_srcPhysAddr + m_srcSize;
    m_dstSize     = m_srcSize;

    return true;
}

bool SecG2dC110::m_destroyG2d(void)
{
    if (m_g2dVirtAddr != NULL) {
        munmap(m_g2dVirtAddr, m_g2dSize);
        m_g2dVirtAddr = NULL;
        m_g2dPhysAddr = 0;
        m_g2dSize = 0;
    }

    if (0 < m_fd)
        close(m_fd);
    m_fd = 0;

    return true;
}

bool SecG2dC110::m_doG2d(G2dRect *src, G2dRect *dst, G2dFlag *flag)
{
    g2d_params params;
    g2d_rect   src_rect;
	g2d_rect   dst_rect;
	g2d_flag   temp_flag;

    src_rect.x            = src->x;
    src_rect.y            = src->y;
    src_rect.w            = src->w;
    src_rect.h            = src->h;
    src_rect.full_w       = src->full_w;
    src_rect.full_h       = src->full_h;
    src_rect.color_format = m_colorFormatSecG2d2G2dHw(src->color_format);
    src_rect.phys_addr    = src->phys_addr;
    src_rect.virt_addr    = (unsigned char *)src->virt_addr;

    dst_rect.x            = dst->x;
    dst_rect.y            = dst->y;
    dst_rect.w            = dst->w;
    dst_rect.h            = dst->h;
    dst_rect.full_w       = dst->full_w;
    dst_rect.full_h       = dst->full_h;
    dst_rect.color_format = m_colorFormatSecG2d2G2dHw(dst->color_format);
    dst_rect.phys_addr    = dst->phys_addr;
    dst_rect.virt_addr    = (unsigned char *)dst->virt_addr;

    temp_flag.rotate_val       = m_rotateValueSecG2d2G2dHw(flag->rotate_val);
    temp_flag.alpha_val        = flag->alpha_val;
    temp_flag.blue_screen_mode = flag->blue_screen_mode;
    temp_flag.color_key_val    = flag->color_switch_val;
    temp_flag.color_val        = flag->src_color;
    temp_flag.third_op_mode    = flag->third_op_mode;
    temp_flag.rop_mode         = flag->rop_mode;
    temp_flag.mask_mode        = flag->mask_mode;

    params.src_rect = &src_rect;
    params.dst_rect = &dst_rect;
    params.flag     = &temp_flag;

    if (ioctl(m_fd, G2D_BLIT, &params) < 0) {
        LOGE("%s::G2D_BLIT fail", __func__);

#if 0
        LOGE("---------------------------------------");
        if (src) {
            LOGE("src.color_format : %d", src->color_format);
            LOGE("src.full_w       : %d", src->full_w);
            LOGE("src.full_h       : %d", src->full_h);
            LOGE("src.x            : %d", src->x);
            LOGE("src.y            : %d", src->y);
            LOGE("src.w            : %d", src->w);
            LOGE("src.h            : %d", src->h);
            LOGE("src.phys_addr    : %x", src->phys_addr);
        }
        else
            LOGE("flag.color_val   : %d", flag->color_val);

        LOGE("dst.color_format : %d", dst->color_format);
        LOGE("dst.full_w       : %d", dst->full_w);
        LOGE("dst.full_h       : %d", dst->full_h);
        LOGE("dst.x            : %d", dst->x);
        LOGE("dst.y            : %d", dst->y);
        LOGE("dst.w            : %d", dst->w);
        LOGE("dst.h            : %d", dst->h);
        LOGE("dst.phys_addr    : %x", dst->phys_addr);

        LOGE("flag.rotate_val  : %d", flag->rotate_val);
        LOGE("flag.alpha_val   : %d(%d)", flag->alpha_val);
        LOGE("flag.color_key_mode  : %d(%d)", flag->color_key_mode, flag->color_key_val);
        LOGE("---------------------------------------");
#endif

        return false;
    }

    return true;
}

inline bool SecG2dC110::m_pollG2d(struct pollfd *events)
{
    #define G2D_POLL_TIME (200)

    int ret;

    ret = poll(events, 1, G2D_POLL_TIME);

    if (ret < 0) {
        LOGE("%s::poll fail", __func__);
        return false;
    } else if (ret == 0) {
        LOGE("%s::No data in %d milli secs..", __func__, G2D_POLL_TIME);
        return false;
    }

    return true;
}

inline bool SecG2dC110::m_cleanG2d(unsigned int physAddr, unsigned int size)
{
    g2d_dma_info dma_info = { physAddr, size };

    if (ioctl(m_fd, G2D_DMA_CACHE_CLEAN, &dma_info) < 0) {
        LOGE("%s::G2D_DMA_CACHE_CLEAN(%d, %d) fail", __func__, physAddr, size);
        return false;
    }
    return true;
}

inline bool SecG2dC110::m_invalidG2d(unsigned int physAddr, unsigned int size)
{
    g2d_dma_info dma_info = { physAddr, size };

    if (ioctl(m_fd, G2D_DMA_CACHE_INVAL, &dma_info) < 0) {
        LOGE("%s::G2D_DMA_CACHE_INVAL(%d, %d) fail", __func__, physAddr, size);
        return false;
    }
    return true;
}

inline bool SecG2dC110::m_flushG2d(unsigned int physAddr, unsigned int size)
{
    g2d_dma_info dma_info = { physAddr, size };

    if (ioctl(m_fd, G2D_DMA_CACHE_FLUSH, &dma_info) < 0)
    {
        LOGE("%s::G2D_DMA_CACHE_FLUSH(%d, %d) fail", __func__, physAddr, size);
        return false;
    }
    return true;
}

inline int SecG2dC110::m_colorFormatSecG2d2G2dHw(int color_format)
{
    // The byte order is different between Android & G2d.
    //               [ 0][ 8][16][24][32]
    //     Android :     R   G   B   A
    //      G2d    :     A   B   G   R

    switch (color_format) {
    case COLOR_FORMAT_RGB_565   : return G2D_RGB_565;

    case COLOR_FORMAT_RGBA_8888 : return G2D_RGBA_8888;
    case COLOR_FORMAT_ARGB_8888 : return G2D_ARGB_8888;
    case COLOR_FORMAT_BGRA_8888 : return G2D_BGRA_8888;
    case COLOR_FORMAT_ABGR_8888 : return G2D_ABGR_8888;

    case COLOR_FORMAT_RGBX_8888 : return G2D_RGBX_8888;
    case COLOR_FORMAT_XRGB_8888 : return G2D_XRGB_8888;
    case COLOR_FORMAT_BGRX_8888 : return G2D_BGRX_8888;
    case COLOR_FORMAT_XBGR_8888 : return G2D_XBGR_8888;

    case COLOR_FORMAT_RGBA_5551 : return G2D_RGBA_5551;
    case COLOR_FORMAT_ARGB_1555 : return G2D_ARGB_1555;
    case COLOR_FORMAT_BGRA_5551 : return G2D_BGRA_5551;
    case COLOR_FORMAT_ABGR_1555 : return G2D_ABGR_1555;

    case COLOR_FORMAT_RGBX_5551 : return G2D_RGBX_5551;
    case COLOR_FORMAT_XRGB_1555 : return G2D_XRGB_1555;
    case COLOR_FORMAT_BGRX_5551 : return G2D_BGRX_5551;
    case COLOR_FORMAT_XBGR_1555 : return G2D_XBGR_1555;

    case COLOR_FORMAT_RGBA_4444 : return G2D_RGBA_4444;
    case COLOR_FORMAT_ARGB_4444 : return G2D_ARGB_4444;
    case COLOR_FORMAT_BGRA_4444 : return G2D_BGRA_4444;
    case COLOR_FORMAT_ABGR_4444 : return G2D_ABGR_4444;

    case COLOR_FORMAT_RGBX_4444 : return G2D_RGBX_4444;
    case COLOR_FORMAT_XRGB_4444 : return G2D_XRGB_4444;
    case COLOR_FORMAT_BGRX_4444 : return G2D_BGRX_4444;
    case COLOR_FORMAT_XBGR_4444 : return G2D_XBGR_4444;

    case COLOR_FORMAT_PACKED_RGB_888 : return G2D_PACKED_RGB_888;
    case COLOR_FORMAT_PACKED_BGR_888 : return G2D_PACKED_BGR_888;
    case COLOR_FORMAT_YUV_420SP :
    case COLOR_FORMAT_YUV_420P :
    case COLOR_FORMAT_YUV_420I :
    case COLOR_FORMAT_YUV_422SP :	
    case COLOR_FORMAT_YUV_422P :
    case COLOR_FORMAT_YUV_422I :
    case COLOR_FORMAT_YUYV :
    default :
        LOGE("%s::not matched frame format : %d", __func__, color_format);
        break;
    }
    return -1;
}

inline int SecG2dC110::m_rotateValueSecG2d2G2dHw(int rotateValue)
{
    switch (rotateValue) {
    case ROTATE_0:      return G2D_ROT_0;
    case ROTATE_90:     return G2D_ROT_90;
    case ROTATE_180:    return G2D_ROT_180;
    case ROTATE_270:    return G2D_ROT_270;
    case ROTATE_X_FLIP: return G2D_ROT_X_FLIP;
    case ROTATE_Y_FLIP: return G2D_ROT_Y_FLIP;
    }

    return -1;
}

bool SecG2dC110::m_getCalibratedNum(unsigned int *num1, unsigned int *num2)
{
    // sw5771.park : This is calibration for (X,1) and (1,X)
    //               This should be fixed on Device Driver..
    //               This info is VERY IMPORTANT..
    unsigned int w = *num1;
    unsigned int h = *num2;

    if (h == 1) {
        unsigned int mulNum = 0;

        #define NUM_OF_PRIMENUM (4)
        unsigned int primeNumList[NUM_OF_PRIMENUM] = { 2, 3, 5, 7 };

        for (int i = 0; i < NUM_OF_PRIMENUM; i++) {
            if ((w % primeNumList[i]) == 0) {
                if (w == primeNumList[i])
                    break; // this can make another variable as 1.. so this is fail.

                mulNum = primeNumList[i];
                break;
            }
        }

        if (mulNum == 0) {
            // there is no calibration number..
            LOGD("%s::calibration for (%d,%d) is fail", __func__, w, h);
            return false;
        }

        h = mulNum;

        if (mulNum == 2)
            w = (w >> 1);
        else
            w = (w / mulNum);
    } else if (w == 1) {
        unsigned int mulNum = 0;

        #define NUM_OF_PRIMENUM (4)
        unsigned int primeNumList[NUM_OF_PRIMENUM] = { 2, 3, 5, 7};

        for (int i = 0; i < NUM_OF_PRIMENUM; i++) {
            if ((h % primeNumList[i]) == 0) {
                if (h == primeNumList[i])
                    break; // this can make another variable as 1.. so this is fail.

                mulNum = primeNumList[i];
                break;
            }
        }

        if (mulNum == 0) {
            // there is no calibration number..
            LOGE("%s::calibration for (X,1) and (1,X) is fail", __func__);
            return false;
        }

        w = mulNum;

        if (mulNum == 2)
            h = (h >> 1);
        else
            h = (h / mulNum);
    }

    *num1 = w;
    *num2 = h;

    return true;
}
#ifdef CHECK_G2D_C110_PERFORMANCE
void SecG2dC110::m_printSecG2dC110Performance(G2dRect    *src,
                                              G2dRect    *dst,
                                              int         stopWatchIndex,
                                              const char *stopWatchName[],
                                              nsecs_t     stopWatchTime[])
{
    char *srcColorFormat = "UNKNOW_COLOR_FORMAT";
    char *dstColorFormat = "UNKNOW_COLOR_FORMAT";

    switch (src->color_format) {
    case COLOR_FORMAT_RGB_565   :
        srcColorFormat = "RGB_565";
        break;
    case COLOR_FORMAT_RGBA_8888 :
        srcColorFormat = "RGBA_8888";
        break;
    case COLOR_FORMAT_RGBX_8888 :
        srcColorFormat = "RGBX_8888";
        break;
    default :
        srcColorFormat = "UNKNOW_COLOR_FORMAT";
        break;
    }

    switch (dst->color_format) {
    case COLOR_FORMAT_RGB_565   :
        dstColorFormat = "RGB_565";
        break;
    case COLOR_FORMAT_RGBA_8888 :
        dstColorFormat = "RGBA_8888";
        break;
    case COLOR_FORMAT_RGBX_8888 :
        dstColorFormat = "RGBX_8888";
        break;
    default :
        dstColorFormat = "UNKNOW_COLOR_FORMAT";
        break;
    }

#ifdef CHECK_G2D_C110_CRITICAL_PERFORMANCE
#else
    LOGE("===============================================");
    LOGE("src[%3d, %3d | %10s] -> dst[%3d, %3d | %10s]",
          src->w, src->h, srcColorFormat,
          dst->w, dst->h, dstColorFormat);
#endif

    nsecs_t totalTime = stopWatchTime[stopWatchIndex - 1];

    for (int i = 0 ; i < stopWatchIndex; i++) {
        nsecs_t sectionTime;

        if (i != 0)
            sectionTime = stopWatchTime[i] - stopWatchTime[i-1];
        else
            sectionTime = stopWatchTime[i];

#ifdef CHECK_G2D_C110_CRITICAL_PERFORMANCE
        if (1500 < (sectionTime / 1000)) // check 1.5 mille second..
#endif
        {
            LOGE("===============================================");
            LOGE("src[%3d, %3d | %10s] -> dst[%3d, %3d | %10s]",
                src->w, src->h, srcColorFormat,
                dst->w, dst->h, dstColorFormat);

            LOGE("%20s : %5lld msec(%5.2f %%)",
                stopWatchName[i],
                sectionTime / 1000,
                ((float)sectionTime / (float)totalTime) * 100.0f);
        }
    }
}
#endif // CHECK_G2D_C110_PERFORMANCE

extern "C" struct SecG2d* createSecG2d()
{
    if (secG2dAutoFreeThread == 0)
        secG2dAutoFreeThread = new SecG2dAutoFreeThread();
    else
        secG2dAutoFreeThread->setOneMoreSleep();

    return SecG2dC110::createInstance();
}

extern "C" void destroySecG2d(SecG2d *g2d)
{
    // Dont' call DestrotInstance..
    // return SecSecG2dC110::DestroyInstance(g2d);
}

}; // namespace android
