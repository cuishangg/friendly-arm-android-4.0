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
 * Author  : Chanwook, Park(cwlsi.park@samsung.com)
 * Date    : 05 Nov 2010
 * Purpose : C210 Porting
 */

#define LOG_NDEBUG 0
#define LOG_TAG "SecG2dC210"
#include <utils/Log.h>

#include "SecG2dC210.h"

namespace android
{

Mutex   SecG2dC210::m_instanceLock;
int     SecG2dC210::m_curG2dC210Index = 0;
int     SecG2dC210::m_numOfInstance = 0;
SecG2d *SecG2dC210::m_ptrG2dList[NUMBER_G2D_LIST] = {NULL, };

SecG2dC210::SecG2dC210()
         : m_fd(0)
{
    m_lock = new Mutex(Mutex::SHARED, "SecG2dC210");
}

SecG2dC210::~SecG2dC210()
{
    delete m_lock;
}

SecG2d* SecG2dC210::createInstance(void)
{ 
    Mutex::Autolock autolock(m_instanceLock);

    SecG2d *ptrG2d = NULL;

    // Using List like RingBuffer...
    for (int i = m_curG2dC210Index; i < NUMBER_G2D_LIST; i++) {
        if (m_ptrG2dList[i] == NULL)
            m_ptrG2dList[i] = new SecG2dC210;

        if (m_ptrG2dList[i]->flagCreate() == false) {
            if (m_ptrG2dList[i]->create() == false) {
                LOGE("%s::create(%d) fail", __func__, i);
                goto CREATE_INSTANCE_END;
            }
            else
                m_numOfInstance++;
        }
                
        if (i < NUMBER_G2D_LIST - 1)
            m_curG2dC210Index = i + 1;
        else
            m_curG2dC210Index = 0;

        ptrG2d = m_ptrG2dList[i];
        goto CREATE_INSTANCE_END;
    }

CREATE_INSTANCE_END :

    return ptrG2d;
}

void SecG2dC210::destroyInstance(SecG2d *g2d)
{
    Mutex::Autolock autolock(m_instanceLock);

    for (int i = 0; i < NUMBER_G2D_LIST; i++) {
        if (   m_ptrG2dList[i] != NULL
            && m_ptrG2dList[i] == g2d) {
            if (m_ptrG2dList[i]->flagCreate() == true
               && m_ptrG2dList[i]->destroy() == false) {
                LOGE("%s::destroy() fail", __func__);
            } else {
                SecG2dC210 *tempG2dC210 = (SecG2dC210 *)m_ptrG2dList[i];
                delete tempG2dC210;
                m_ptrG2dList[i] = NULL;

                m_numOfInstance--;
            }
            break;
        }
    }
}

void SecG2dC210::destroyAllInstance(void)
{
    Mutex::Autolock autolock(m_instanceLock);

    for (int i = 0; i < NUMBER_G2D_LIST; i++) {
        if (m_ptrG2dList[i] != NULL) {
            if (   m_ptrG2dList[i]->flagCreate() == true
                && m_ptrG2dList[i]->destroy() == false) {
                LOGE("%s::destroy() fail", __func__);
            } else {
                SecG2dC210 *tempG2dC210 = (SecG2dC210 *)m_ptrG2dList[i];
                delete tempG2dC210;
                m_ptrG2dList[i] = NULL;
            }
        }
    }
}

bool SecG2dC210::t_create(void)
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

bool SecG2dC210::t_destroy(void)
{
    bool ret = true;

    if (m_destroyG2d() == false) {
        LOGE("%s::m_destroyG2d() fail", __func__);
        ret = false;
    }

    return ret;
}

bool SecG2dC210::t_stretch(G2dRect *src, G2dRect *dst, G2dClip *clip, G2dFlag *flag)
{
#ifdef CHECK_G2D_C210_PERFORMANCE
    #define     NUM_OF_STEP (10)
    StopWatch   stopWatch("CHECK_G2D_C210_PERFORMANCE");
    const char *stopWatchName[NUM_OF_STEP];
    nsecs_t     stopWatchTime[NUM_OF_STEP];
    int         stopWatchIndex = 0;
#endif // CHECK_G2D_C210_PERFORMANCE

    if (flag->render_mode & RENDER_MODE_CACHE_ON) {
        if (m_cleanG2d((unsigned int)src->virt_addr, t_frameSize(src)) == false) {
            LOGE("%s::m_cleanG2d() fail", __func__);
            return false;
        }
    }

    if (m_doG2d(src, dst, clip, flag) == false) {
        LOGE("%s::m_doG2d() fail", __func__);
        return false;
    }

#ifdef CHECK_G2D_C210_PERFORMANCE
    m_PrintG2dC210Performance(src, dst, stopWatchIndex, stopWatchName, stopWatchTime);
#endif // CHECK_G2D_C210_PERFORMANCE

    return true;

STRETCH_FAIL:
    return false;
}

bool SecG2dC210::t_sync(void)
{
#if 1
    if (ioctl(m_fd, FIMG2D_BITBLT_WAIT) < 0) {
        LOGE("%s::G2D_Sync fail", __func__);
        goto SYNC_FAIL;  
    }
#else
    if (m_pollG2d(&m_poll) == false) {
        LOGE("%s::m_pollG2d() fail", __func__);
        goto SYNC_FAIL;  
    }
#endif
    return true;

SYNC_FAIL:
    return false;
}

bool SecG2dC210::t_lock(void)
{
    m_lock->lock();
    return true;
}

bool SecG2dC210::t_unLock(void)
{
    m_lock->unlock();
    return true;
}

bool SecG2dC210::m_createG2d(void)
{
    void *mmap_base;

    if (m_fd != 0) {
        LOGE("%s::m_fd(%d) is not 0 fail", __func__, m_fd);
        return false;
    }

    m_fd = open(SEC_G2D_DEV_NAME, O_RDWR);
    if (m_fd < 0) {
        LOGE("%s::open(%s) fail(%s)", __func__, SEC_G2D_DEV_NAME, strerror(errno));
        m_fd = 0;
        return false;
    }

    memset(&m_poll, 0, sizeof(m_poll));
    m_poll.fd     = m_fd;
    m_poll.events = POLLOUT | POLLERR;
 
    return true;
}

bool SecG2dC210::m_destroyG2d(void)
{
    if (0 < m_fd) {
        close(m_fd);
    }
    m_fd = 0;

    return true;
}

bool SecG2dC210::m_doG2d(G2dRect *src, G2dRect *dst, G2dClip *clip, G2dFlag *flag)
{
    int order = 0;

    if (src) {    
        m_src_img.type      = NORMAL;
        m_src_img.addr      = (unsigned long)src->virt_addr;
        m_src_img.addr_type = FIMG2D_ADDR_USER;
        m_src_img.width     = src->full_w;
        m_src_img.height    = src->full_h;
    
        m_src_img.fmt       = (FIMG2D_RGB_FORMAT_T)m_colorFormatSecG2d2G2dHw(src->color_format, &order);
        m_src_img.order     = (FIMG2D_RGB_ORDER_T)order;
        m_src_img.stride    = src->w;

        m_src_img.rect.x1   = src->x;
        m_src_img.rect.x2   = src->w;
        m_src_img.rect.y1   = src->y;
        m_src_img.rect.y2   = src->h;

        m_user_ctx.src = &m_src_img;
    }
    else
        m_user_ctx.src = NULL;    

    if (dst) {
        m_dst_img.type      = NORMAL;
        m_dst_img.addr      = (unsigned long)dst->virt_addr;
        m_dst_img.addr_type = FIMG2D_ADDR_USER;
        m_dst_img.width     = dst->full_w;
        m_dst_img.height    = dst->full_h;
    
        m_dst_img.fmt       = (FIMG2D_RGB_FORMAT_T)m_colorFormatSecG2d2G2dHw(dst->color_format, &order);
        m_dst_img.order     = (FIMG2D_RGB_ORDER_T)order;
        m_dst_img.stride    = dst->w;

        m_dst_img.rect.x1   = dst->x;
        m_dst_img.rect.x2   = dst->w;
        m_dst_img.rect.y1   = dst->y;
        m_dst_img.rect.y2   = dst->h;

        m_user_ctx.dst = &m_dst_img;
    }
    else
        m_user_ctx.dst = NULL;  

    m_param.op    = (FIMG2D_BITBLT_T)m_portterDuffSecG2d2G2dHw(flag->potterduff_mode);  
    m_param.color = flag->src_color;
    m_param.dx    = 0;
    m_param.dy    = 0;

    if (flag->alpha_val == ALPHA_OPAQUE)
        m_param.alpha.enabled = 0;
    else {
        m_param.alpha.enabled = 1;
    }

    m_param.alpha.nonpre_type = DISABLED;
    m_param.alpha.value = flag->alpha_val;
    m_param.rot = (FIMG2D_ROTATION_T)m_rotateValueSecG2d2G2dHw(flag->rotate_val);
    m_param.scale = 1;

    m_user_ctx.param = &m_param;

    if (ioctl(m_fd, FIMG2D_BITBLT_CONFIG, &m_user_ctx) < 0) {
#if 0
        m_PrintSecG2dArg(src, dst, clip, flag);
#endif
        LOGE("%s::FIMG2D_BITBLT_CONFIG fail", __func__);
        return false;
    }

    if (src)
        m_user_region.src = &m_src_img.rect;
    else
        m_user_region.src = NULL;

    if (dst)
        m_user_region.dst = &m_dst_img.rect;
    else
        m_user_region.dst = NULL;

    if(clip) {    
        m_dst_clip.x1 = clip->l;
        m_dst_clip.x2 = clip->r;
        m_dst_clip.y1 = clip->t;
        m_dst_clip.y2 = clip->b;

        m_user_region.dst_clip = &m_dst_clip;
    } else
        m_user_region.dst_clip = NULL;

    if (ioctl(m_fd, FIMG2D_BITBLT_UPDATE, &m_user_region) < 0) {
        LOGE("%s::FIMG2D_BITBLT_UPDATE fail", __func__);
        return false;
    }

    if (ioctl(m_fd, FIMG2D_BITBLT_CLOSE) < 0) {
        LOGE("%s::FIMG2D_BITBLT_CLOSE fail", __func__);
        return false;
    }

    // kcoolsw
    LOGD("#### kcoolsw blit_wait 1");
    //if ((flag->render_mode & RENDER_MODE_NON_BLOCKING) == 0) {
        if (ioctl(m_fd, FIMG2D_BITBLT_WAIT) < 0) {
                LOGE("%s::FIMG2D_BITBLT_WAIT fail", __func__);
                return false;
        }
    //}
    LOGD("#### kcoolsw blit_wait 2");
    return true;
}

inline bool SecG2dC210::m_pollG2d(struct pollfd *events)
{
    #define G2D_POLL_TIME (1000)

    int ret;

    ret = poll(events, 1, G2D_POLL_TIME);

    if (ret < 0) {
        /*
        if (ioctl(m_fd, G2D_RESET) < 0) {
            LOGE("%s::G2D_RESET fail", __func__);
        }
        */
        LOGE("%s::poll fail", __func__);
        return false;
    } else if (ret == 0) {
        /*
        if (ioctl(m_fd, G2D_RESET) < 0) {
            LOGE("%s::G2D_RESET fail", __func__);
        }
        */
        LOGE("%s::No data in %d milli secs..", __func__, G2D_POLL_TIME);
        return false;
    }

    return true;
}

inline bool SecG2dC210::m_cleanG2d(unsigned int virtAddr, unsigned int size)
{
    fimg2d_dma_info dma_info = { virtAddr, size, FIMG2D_ADDR_USER };

    if (ioctl(m_fd, FIMG2D_DMA_CACHE_CLEAN, &dma_info) < 0) {
        LOGE("%s::FIMG2D_DMA_CACHE_CLEAN(%0d, %d) fail", __func__, virtAddr, size);
        return false;
    }
    return true;
}

inline bool SecG2dC210::m_flushG2d(unsigned int virtAddr, unsigned int size)
{
    fimg2d_dma_info dma_info = { virtAddr, size, FIMG2D_ADDR_USER };

    if (ioctl(m_fd, FIMG2D_DMA_CACHE_FLUSH, &dma_info) < 0) {
        LOGE("%s::FIMG2D_DMA_CACHE_FLUSH(%d, %d) fail", __func__, virtAddr, size);
        return false;
    }
    return true;
}

inline int SecG2dC210::m_colorFormatSecG2d2G2dHw(int color_format, int *order)
{
    // The byte order is different between Android & G2d.
    //               [ 0][ 8][16][24][32]
    //     Android :     R   G   B   A
    //      G2d    :     A   B   G   R

    int g2dColorFormat = 0;

    switch (color_format) {
    case COLOR_FORMAT_RGB_565   : *order = RGB_AX;
        g2dColorFormat = RGB565;
        break;
    case COLOR_FORMAT_RGBA_8888 : *order = RGB_AX;
    case COLOR_FORMAT_ARGB_8888 : *order = AX_RGB;
    case COLOR_FORMAT_BGRA_8888 : *order = BGR_AX;
    case COLOR_FORMAT_ABGR_8888 : *order = AX_BGR;
        g2dColorFormat = ARGB8888;
        break;
    case COLOR_FORMAT_RGBX_8888 : *order = RGB_AX;
    case COLOR_FORMAT_XRGB_8888 : *order = AX_RGB;
    case COLOR_FORMAT_BGRX_8888 : *order = BGR_AX;
    case COLOR_FORMAT_XBGR_8888 : *order = AX_BGR;
        g2dColorFormat = XRGB8888;
        break;
    case COLOR_FORMAT_RGBA_5551 : *order = RGB_AX;
    case COLOR_FORMAT_ARGB_1555 : *order = AX_RGB;
    case COLOR_FORMAT_BGRA_5551 : *order = BGR_AX;
    case COLOR_FORMAT_ABGR_1555 : *order = AX_BGR;
        g2dColorFormat = ARGB1555;
        break;
    case COLOR_FORMAT_RGBX_5551 : *order = RGB_AX;
    case COLOR_FORMAT_XRGB_1555 : *order = AX_RGB;
    case COLOR_FORMAT_BGRX_5551 : *order = BGR_AX;
    case COLOR_FORMAT_XBGR_1555 : *order = AX_BGR;
        g2dColorFormat = XRGB1555;
        break;
    case COLOR_FORMAT_RGBA_4444 : *order = RGB_AX;
    case COLOR_FORMAT_ARGB_4444 : *order = AX_RGB;
    case COLOR_FORMAT_BGRA_4444 : *order = BGR_AX;
    case COLOR_FORMAT_ABGR_4444 : *order = AX_BGR;
        g2dColorFormat = ARGB4444;
        break;
    case COLOR_FORMAT_RGBX_4444 : *order = RGB_AX;
    case COLOR_FORMAT_XRGB_4444 : *order = AX_RGB;
    case COLOR_FORMAT_BGRX_4444 : *order = BGR_AX;
    case COLOR_FORMAT_XBGR_4444 : *order = AX_BGR;
        g2dColorFormat = XRGB4444;
        break;
    case COLOR_FORMAT_PACKED_RGB_888 : *order = RGB_AX;
    case COLOR_FORMAT_PACKED_BGR_888 : *order = BGR_AX;
        g2dColorFormat = RGB888;
        break;
    case COLOR_FORMAT_YUV_420SP :
    case COLOR_FORMAT_YUV_420P :
    case COLOR_FORMAT_YUV_420I :
    case COLOR_FORMAT_YUV_422SP :	
    case COLOR_FORMAT_YUV_422P :
    case COLOR_FORMAT_YUV_422I :
    case COLOR_FORMAT_YUYV :
    default :
        LOGE("%s::not matched frame format : %d", __func__, color_format);
        g2dColorFormat = -1;
        break;
    }
    return g2dColorFormat;
}

inline int SecG2dC210::m_rotateValueSecG2d2G2dHw(int rotateValue)
{
    switch (rotateValue) {
    case ROTATE_0:      return ORIGIN;
    case ROTATE_90:     return ROT90;
    case ROTATE_180:    return ROT180;
    case ROTATE_270:    return ROT270;
    case ROTATE_X_FLIP: return XFLIP;
    case ROTATE_Y_FLIP: return YFLIP;	
    }

    return -1;
}

inline int SecG2dC210::m_portterDuffSecG2d2G2dHw(int portterDuff)
{
    switch (portterDuff) {
    case PORTTERDUFF_Src:     return OP_SRC;
    case PORTTERDUFF_Dst:     return OP_DST;
    case PORTTERDUFF_Clear:   return OP_CLEAR;
    case PORTTERDUFF_SrcOver: return OP_OVER;
    case PORTTERDUFF_DstOver: return OP_OVER_REV;
    case PORTTERDUFF_SrcIn:   return OP_OVER_REV;
    case PORTTERDUFF_DstIn:   return OP_IN_REV;
    case PORTTERDUFF_SrcOut:  return OP_OUT;
    case PORTTERDUFF_DstOut:  return OP_OUT_REV;
    case PORTTERDUFF_SrcATop: return OP_ATOP;
    case PORTTERDUFF_DstATop: return OP_ATOP_REV;
    case PORTTERDUFF_Xor:     return OP_XOR;

    case PORTTERDUFF_Plus:    return OP_ADD;
    case PORTTERDUFF_Multiply:
    case PORTTERDUFF_Screen:
    case PORTTERDUFF_Overlay:
    case PORTTERDUFF_Darken:
    case PORTTERDUFF_Lighten:
    case PORTTERDUFF_ColorDodge:
    case PORTTERDUFF_ColorBurn:
    case PORTTERDUFF_HardLight:
    case PORTTERDUFF_SoftLight:
    case PORTTERDUFF_Difference:
    case PORTTERDUFF_Exclusion:
    default:
        break;
    }
    return -1;    
}

void SecG2dC210::m_PrintSecG2dArg(G2dRect *src, G2dRect *dst, G2dClip *clip, G2dFlag *flag)
{
    LOGD("---------------------------------------");
    if (src) {    
        LOGD("src.color_format : %d", src->color_format);
        LOGD("src.full_w       : %d", src->full_w);
        LOGD("src.full_h       : %d", src->full_h);
        LOGD("src.x            : %d", src->x);
        LOGD("src.y            : %d", src->y);
        LOGD("src.w            : %d", src->w);
        LOGD("src.h            : %d", src->h);
    } else
        LOGD("src is NULL");

    if (dst) {    
        LOGD("dst.color_format : %d", dst->color_format);
        LOGD("dst.full_w       : %d", dst->full_w);
        LOGD("dst.full_h       : %d", dst->full_h);
        LOGD("dst.x            : %d", dst->x);
        LOGD("dst.y            : %d", dst->y);
        LOGD("dst.w            : %d", dst->w);
        LOGD("dst.h            : %d", dst->h);
    } else
        LOGD("dst is NULL");

    if (clip) {
        LOGD("clip->l          : %d", clip->l);
        LOGD("clip->r          : %d", clip->r);
        LOGD("clip->t          : %d", clip->t);
        LOGD("clip->b          : %d", clip->b);

    } else
        LOGD("clip is NULL");

    if (flag) {
        LOGD("flag.rotate_val  : %d", flag->rotate_val);
        LOGD("flag.alpha_val   : %d", flag->alpha_val);
    }
    else
        LOGD("flag is NULL");

    LOGD("---------------------------------------");
}

#ifdef CHECK_G2D_C210_PERFORMANCE
void SecG2dC210::m_PrintG2dC210Performance(G2dRect    *src,
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

#ifdef CHECK_G2D_C210_CRITICAL_PERFORMANCE
#else
    LOGD("===============================================");
    LOGD("src[%3d, %3d | %10s] -> dst[%3d, %3d | %10s]",
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

#ifdef CHECK_G2D_C210_CRITICAL_PERFORMANCE
        if (1500 < (sectionTime / 1000)) // check 1.5 mille second..
#endif
        {
            LOGD("===============================================");
            LOGD("src[%3d, %3d | %10s] -> dst[%3d, %3d | %10s]",
                   src->w, src->h, srcColorFormat,
                   dst->w, dst->h, dstColorFormat);

            LOGD("%20s : %5lld msec(%5.2f %%)",
              stopWatchName[i],
              sectionTime / 1000,
              ((float)sectionTime / (float)totalTime) * 100.0f);
        }
    }

}
#endif // CHECK_G2D_C210_PERFORMANCE

extern "C" struct SecG2d* createSecG2d(void)
{
    if (secG2dApiAutoFreeThread == 0)
        secG2dApiAutoFreeThread = new SecG2dAutoFreeThread();
    else
        secG2dApiAutoFreeThread->setOneMoreSleep();

    return SecG2dC210::createInstance();
}

extern "C" void destroySecG2d(SecG2d *g2d)
{
    // Dont' call DestrotInstance..		
    // return SecG2dC210::DestroyInstance(g2d);
}

}; // namespace android
