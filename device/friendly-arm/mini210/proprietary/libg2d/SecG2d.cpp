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
 * G2D HAL MODULE
 *
 * Author  : Sangwoo, Park(sw5771.park@samsung.com)
 * Date    : 02 May 2010
 * Purpose : Initial version
 */

#define LOG_NDEBUG 0
#define LOG_TAG "SecG2d"
#include <utils/Log.h>

#include "SecG2d.h"

#define GET_32BPP_FRAME_SIZE(w, h)  (((w) * (h)) << 2)
#define GET_24BPP_FRAME_SIZE(w, h)  (((w) * (h)) * 3)
#define GET_16BPP_FRAME_SIZE(w, h)  (((w) * (h)) << 1)

SecG2d::SecG2d()
{
    m_flagCreate = false;
}

SecG2d::~SecG2d()
{
    if (m_flagCreate == true)
        LOGE("%s::this is not Destroyed fail", __func__);
}

bool SecG2d::create(void)
{
    bool ret = false;

    if (t_lock() == false) {
        LOGE("%s::t_lock() fail", __func__);
        goto CREATE_DONE;
    }
     
    if (m_flagCreate == true) {
        LOGE("%s::Already Created fail", __func__);
        goto CREATE_DONE;
    }

    if (t_create() == false) {
        LOGE("%s::t_create() fail", __func__);
        goto CREATE_DONE;
    }

    m_flagCreate = true;

    ret = true;

CREATE_DONE :

    t_unLock();

    return ret;
}

bool SecG2d::destroy(void)
{
    bool ret = false;

    if (t_lock() == false) {
        LOGE("%s::t_lock() fail", __func__);
        goto DESTROY_DONE;
    }     

    if (m_flagCreate == false) {
        LOGE("%s::Already Destroyed fail", __func__);
        goto DESTROY_DONE;
    }
    
    if (t_destroy() == false) {
        LOGE("%s::t_destroy() fail", __func__);
        goto DESTROY_DONE;
    }

    m_flagCreate = false;

    ret = true;

DESTROY_DONE :

    t_unLock();

    return ret;
}

bool SecG2d::stretch(G2dRect *src,
                     G2dRect *dst,
                     G2dClip *clip,
                     G2dFlag *flag)
{
    bool ret = false;

    if (t_lock() == false) {
        LOGE("%s::t_lock() fail", __func__);
        goto STRETCH_DONE;
    }    

    if (m_flagCreate == false) {
        LOGE("%s::This is not Created fail", __func__);
        goto STRETCH_DONE;
    }

    if (t_stretch(src, dst, clip, flag) == false) {
        goto STRETCH_DONE;
    }

    ret = true;

STRETCH_DONE :

    t_unLock();

    return ret;
}

bool SecG2d::sync(void)
{
    bool ret = false;

    if (m_flagCreate == false) {
        LOGE("%s::This is not Created fail", __func__);
        goto SYNC_DONE;
    }

    if (t_sync() == false) {
        goto SYNC_DONE;
    }

    ret = true;

SYNC_DONE :

    return ret;
}

bool SecG2d::t_create(void)
{
    LOGE("%s::This is empty virtual function fail", __func__);
    return false;
}

bool SecG2d::t_destroy(void)
{
    LOGE("%s::This is empty virtual function fail", __func__);
    return false;
}

bool SecG2d::t_stretch(G2dRect *src,
                       G2dRect *dst,
                       G2dClip *clip,
                       G2dFlag *flag)
{
    LOGE("%s::This is empty virtual function fail", __func__);
    return false;
}

bool SecG2d::t_sync(void)
{
    LOGE("%s::This is empty virtual function fail", __func__);
    return false;
}

bool SecG2d::t_lock(void)
{
    LOGE("%s::This is empty virtual function fail", __func__);
    return false;
}

bool SecG2d::t_unLock(void)
{
    LOGE("%s::This is empty virtual function fail", __func__);
    return false;
}

unsigned int SecG2d::t_frameSize(G2dRect * rect)
{
    unsigned int frame_size = 0;
    unsigned int size       = 0;

    switch(rect->color_format)
    {
        // 16bpp
        case COLOR_FORMAT_RGB_565   :
            frame_size = GET_16BPP_FRAME_SIZE(rect->full_w, rect->full_h);
            //size = rect->full_w * rect->full_h;
            //frame_size = (size << 1);
            break;

        // 32bpp
        case COLOR_FORMAT_RGBA_8888 :
        case COLOR_FORMAT_ARGB_8888 :
        case COLOR_FORMAT_BGRA_8888 :
        case COLOR_FORMAT_ABGR_8888 :

        case COLOR_FORMAT_RGBX_8888 :
        case COLOR_FORMAT_XRGB_8888 :
        case COLOR_FORMAT_BGRX_8888 :
        case COLOR_FORMAT_XBGR_8888 :
            frame_size = GET_32BPP_FRAME_SIZE(rect->full_w, rect->full_h);
            //size = rect->full_w * rect->full_h;
            //frame_size = (size << 2);
            break;

        // 16bpp
        case COLOR_FORMAT_RGBA_5551 :
        case COLOR_FORMAT_ARGB_1555 :
        case COLOR_FORMAT_BGRA_5551 :
        case COLOR_FORMAT_ABGR_1555 :

        case COLOR_FORMAT_RGBX_5551 :
        case COLOR_FORMAT_XRGB_1555 :
        case COLOR_FORMAT_BGRX_5551 :
        case COLOR_FORMAT_XBGR_1555 :

        case COLOR_FORMAT_RGBA_4444 :
        case COLOR_FORMAT_ARGB_4444 :
        case COLOR_FORMAT_BGRA_4444 :
        case COLOR_FORMAT_ABGR_4444 :

        case COLOR_FORMAT_RGBX_4444 :
        case COLOR_FORMAT_XRGB_4444 :
        case COLOR_FORMAT_BGRX_4444 :
        case COLOR_FORMAT_XBGR_4444 :
            frame_size = GET_16BPP_FRAME_SIZE(rect->full_w, rect->full_h);
            break;

        // 24bpp
        case COLOR_FORMAT_PACKED_RGB_888 :
        case COLOR_FORMAT_PACKED_BGR_888 :
            frame_size = GET_24BPP_FRAME_SIZE(rect->full_w, rect->full_h);
            break;

        // 18bpp
        case COLOR_FORMAT_YUV_420SP :
        case COLOR_FORMAT_YUV_420P  :
        case COLOR_FORMAT_YUV_420I  :
            size = rect->full_w * rect->full_h;
            //frame_size = w * h * 3 / 2;
            // sw5771.park : very curious...
            //frame_size = size + (( size / 4) * 2);
            frame_size = size + (( size >> 2) << 1);
            break;

        // 16bpp
        case COLOR_FORMAT_YUV_422SP :
        case COLOR_FORMAT_YUV_422P  :
        case COLOR_FORMAT_YUV_422I  :
        case COLOR_FORMAT_YUYV      :
            frame_size = GET_16BPP_FRAME_SIZE(rect->full_w, rect->full_h);
            break;

        default :
            LOGE("%s::no matching source colorformat(%d), w(%d), h(%d) fail",
                    __func__, rect->color_format, rect->full_w, rect->full_h);
            break;
    }
    return frame_size;
}

bool SecG2d::t_copyFrame(G2dRect * src, G2dRect * dst)
{
    // sw5771.park : This Function assume
    // src->color_format == dst->color_format
    // src->w           == dst->w
    // src->h           == dst->h

    unsigned int bppShift = 0;

    unsigned int realSrcFullW  = 0;
    unsigned int realSrcX      = 0;
    unsigned int realDstFullW  = 0;
    unsigned int realDstX      = 0;

    unsigned int realW         = 0;

    char * curSrcAddr = src->virt_addr;
    char * curDstAddr = dst->virt_addr;

    switch(src->color_format)
    {
        // 16bit
        case COLOR_FORMAT_RGB_565   :
            bppShift = 1;
            break;

        // 32bit
        case COLOR_FORMAT_RGBA_8888 :
        case COLOR_FORMAT_ARGB_8888 :
        case COLOR_FORMAT_BGRA_8888 :
        case COLOR_FORMAT_ABGR_8888 :

        case COLOR_FORMAT_RGBX_8888 :
        case COLOR_FORMAT_XRGB_8888 :
        case COLOR_FORMAT_BGRX_8888 :
        case COLOR_FORMAT_XBGR_8888 :
            bppShift = 2;
            break;

        // 16bit
        case COLOR_FORMAT_RGBA_5551 :
        case COLOR_FORMAT_ARGB_1555 :
        case COLOR_FORMAT_BGRA_5551 :
        case COLOR_FORMAT_ABGR_1555 :

        case COLOR_FORMAT_RGBX_5551 :
        case COLOR_FORMAT_XRGB_1555 :
        case COLOR_FORMAT_BGRX_5551 :
        case COLOR_FORMAT_XBGR_1555 :

        case COLOR_FORMAT_RGBA_4444 :
        case COLOR_FORMAT_ARGB_4444 :
        case COLOR_FORMAT_BGRA_4444 :
        case COLOR_FORMAT_ABGR_4444 :

        case COLOR_FORMAT_RGBX_4444 :
        case COLOR_FORMAT_XRGB_4444 :
        case COLOR_FORMAT_BGRX_4444 :
        case COLOR_FORMAT_XBGR_4444 :
            bppShift = 1;
            break;

        //case COLOR_FORMAT_RGB_888   :
        //case COLOR_FORMAT_BGR_888   :
        default:
            LOGE("%s::no matching source colorformat(%d), w(%d), h(%d) fail",
                  __func__, src->color_format, src->full_w, src->full_h);
            return false;
            break;
    }

    if(    src->full_w == dst->full_w
        && src->full_w == src->w)
    {
        // if the rectangles of src & dst are exactly the same.
        if(   src->full_h == dst->full_h
           && src->full_h == src->h)
        {
            unsigned int fullFrameSize  = 0;

            fullFrameSize = ((src->full_w * src->full_h) << bppShift);
            memcpy(dst->virt_addr, src->virt_addr, fullFrameSize);
        }
        // src y and dst y is different..
        else
        {
            realSrcFullW = (src->full_w  << bppShift);
            realDstFullW = (dst->full_w  << bppShift);

            realW        = (src->w       << bppShift);

            curSrcAddr += (realSrcFullW * src->y);
            curDstAddr += (realDstFullW * dst->y);

            memcpy(curDstAddr, curSrcAddr, realW * src->h);
        }
    }
    else
    {
        realSrcFullW = (src->full_w  << bppShift);
        realSrcX     = (src->x       << bppShift);

        realDstFullW = (dst->full_w  << bppShift);
        realDstX     = (dst->x       << bppShift);

        realW        = (src->w      << bppShift);

        curSrcAddr += (realSrcFullW * src->y);
        curDstAddr += (realDstFullW * dst->y);

        for(int i = 0; i < src->h; i++)
        {
            memcpy(curDstAddr + realDstX,
                   curSrcAddr + realSrcX,
                   realW);

            curSrcAddr += realSrcFullW;
            curDstAddr += realDstFullW;
        }
    }

    return true;
}

extern "C" int stretchSecG2d(G2dRect *src,
                             G2dRect *dst,
                             G2dClip *clip,
                             G2dFlag *flag)
{
    SecG2d *g2d = createSecG2d();
    
    if (g2d == NULL) {
        LOGE("%s::createSecG2d() fail", __func__);
        return -1;
    }

    if (g2d->stretch(src, dst, clip, flag) == false) {
        if (g2d != NULL)
            destroySecG2d(g2d);

        return -1;
    }

    if (g2d != NULL)
        destroySecG2d(g2d);

    return 0;
}

extern "C" int syncSecG2d(void)
{
    SecG2d *g2d = createSecG2d();
    if (g2d == NULL) {
        LOGE("%s::createSecG2d() fail", __func__);
        return -1;
    }

    if (g2d->sync() == false) {
        if (g2d != NULL)
            destroySecG2d(g2d);

        return -1;
    }

    if (g2d != NULL)
        destroySecG2d(g2d);

    return 0;
}

