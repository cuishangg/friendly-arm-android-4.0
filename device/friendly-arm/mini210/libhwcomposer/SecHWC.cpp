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
           Jamie Oh (jung-min.oh@samsung.com)
 * @date   2011-03-11
 *
 */

#include <cutils/log.h>
#include <cutils/atomic.h>

#include <EGL/egl.h>

#include "SecHWCUtils.h"

#if defined(S5P_BOARD_USES_HDMI)
#include "SecHdmiClient.h"
#include "SecTVOutService.h"
#include "s3c_lcd.h"
#include <pthread.h>
#include <signal.h>
#include <utils/Timers.h>

static int              lcd_width, lcd_height;
static bool             pthread_blit2Hdmi_created = false;
static pthread_t        pthread_blit2Hdmi;
static pthread_cond_t   sync_condition;
static pthread_mutex_t  sync_mutex;
static int              num_of_hwc_layer, num_of_fb_layer;
static sec_img          hdmi_src_img;
static sec_rect         hdmi_src_rect;
static unsigned int     prevLCDAddr = 0;
static int              LCDFd = -1;
#endif

//#define HWC_DEBUG

/*****************************************************************************/

static int hwc_device_open(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device);

static struct hw_module_methods_t hwc_module_methods = {
    open: hwc_device_open
};

hwc_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
        id: HWC_HARDWARE_MODULE_ID,
        name: "Samsung S5PC11X hwcomposer module",
        author: "SAMSUNG",
	methods: &hwc_module_methods,
	dso: NULL, /* remove compilation warnings */
	reserved: {0}, /* remove compilation warnings */
    }
};

/*****************************************************************************/

static void dump_layer(hwc_layer_t const* l) {
    LOGD("\ttype=%d, flags=%08x, handle=%p, tr=%02x, blend=%04x, {%d,%d,%d,%d}, {%d,%d,%d,%d}",
            l->compositionType, l->flags, l->handle, l->transform, l->blending,
            l->sourceCrop.left,
            l->sourceCrop.top,
            l->sourceCrop.right,
            l->sourceCrop.bottom,
            l->displayFrame.left,
            l->displayFrame.top,
            l->displayFrame.right,
            l->displayFrame.bottom);
}

static int set_src_dst_img(hwc_layer_t *cur,
                           struct hwc_win_info_t *win,
                           struct sec_img *src_img,
                           struct sec_img *dst_img,
                           int win_idx)
{
    sec_native_handle_t *prev_handle = (sec_native_handle_t *)(cur->handle);

    // set src image
    src_img->w       = prev_handle->width;
    src_img->h       = prev_handle->height;
    src_img->format  = prev_handle->format;
    src_img->base    = (uint32_t)prev_handle->base_addr;
    src_img->offset  = prev_handle->offset;
    src_img->mem_id  = prev_handle->fd;

    src_img->mem_type = HWC_VIRT_MEM_TYPE;

    switch (prev_handle->format) {
        case HAL_PIXEL_FORMAT_YV12:
            src_img->w = (prev_handle->stride + 15) & (~15);
            src_img->h = (src_img->h + 1) & (~1) ;
            break;
        case HAL_PIXEL_FORMAT_YCbCr_422_SP:
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
        case HAL_PIXEL_FORMAT_YCbCr_422_P:
        case HAL_PIXEL_FORMAT_YCbCr_420_P:
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:
        case HAL_PIXEL_FORMAT_YCrCb_422_SP:
        case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP:
        case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_420_SP:
        case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP_TILED:
        case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_SP:
        case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_SP:
            src_img->w = (src_img->w + 15) & (~15);
            src_img->h = (src_img->h + 1) & (~1) ;
            break;
        default:
            break;
    }

    //set dst image
    dst_img->w = win->lcd_info.xres;
    dst_img->h = win->lcd_info.yres;

    switch (win->lcd_info.bits_per_pixel) {
    case 32:
        dst_img->format = HAL_PIXEL_FORMAT_RGBX_8888;
        break;
    default:
        dst_img->format = HAL_PIXEL_FORMAT_RGB_565;
        break;
    }

    dst_img->base     = win->addr[win->buf_index];
    dst_img->offset   = 0;
    dst_img->mem_id   = 0;
    dst_img->mem_type = HWC_PHYS_MEM_TYPE;

    SEC_HWC_Log(HWC_LOG_DEBUG, "%s:: s_w %d s_h %d s_f %x s_base %x s_offset %d s_mem_id %d s_mem_type %d  d_w %d d_h %d d_f %x d_base %x d_offset %d d_mem_id %d d_mem_type %d rot %d win_idx %d",
                    __func__, src_img->w, src_img->h,  src_img->format, src_img->base, src_img->offset,  src_img->mem_id, src_img->mem_type,
                    dst_img->w, dst_img->h,  dst_img->format, dst_img->base, dst_img->offset,  dst_img->mem_id, dst_img->mem_type, cur->transform, win_idx);

    return 0;
}

static int set_src_dst_rect(hwc_layer_t *cur,
                            struct hwc_win_info_t *win,
                            struct sec_rect *src_rect,
                            struct sec_rect *dst_rect)
{
    sec_native_handle_t *prev_handle = (sec_native_handle_t *)(cur->handle);
    //set src rect
    src_rect->x = SEC_MAX(cur->sourceCrop.left, 0);
    src_rect->y = SEC_MAX(cur->sourceCrop.top, 0);
    src_rect->w = SEC_MAX(cur->sourceCrop.right - cur->sourceCrop.left, 0);
    src_rect->w = SEC_MIN(src_rect->w, prev_handle->width);
    src_rect->h = SEC_MAX(cur->sourceCrop.bottom - cur->sourceCrop.top, 0);
    src_rect->h = SEC_MIN(src_rect->h, prev_handle->height);

    //set dst rect
    //fimc dst image will be stored from left top corner
    dst_rect->x = 0;
    dst_rect->y = 0;
    dst_rect->w = win->rect_info.w;
    dst_rect->h = win->rect_info.h;

    SEC_HWC_Log(HWC_LOG_DEBUG, "%s::sr_x %d sr_y %d sr_w %d sr_h %d dr_x %d dr_y %d dr_w %d dr_h %d ", __func__,
            src_rect->x, src_rect->y, src_rect->w, src_rect->h, dst_rect->x, dst_rect->y, dst_rect->w, dst_rect->h);

    return 0;
}

#ifdef SUB_TITLES_HWC
static int set_src_dst_g2d_rect(hwc_layer_t *cur,
                            struct hwc_win_info_t *win,
                            G2dRect *src_rect,
                            G2dRect *dst_rect)
{
    sec_native_handle_t *prev_handle = (sec_native_handle_t *)(cur->handle);
    
    src_rect->x = SEC_MAX(cur->sourceCrop.left, 0);
    src_rect->y = SEC_MAX(cur->sourceCrop.top, 0);
    src_rect->w = SEC_MAX(cur->sourceCrop.right - cur->sourceCrop.left, 0);
    src_rect->w = SEC_MIN(src_rect->w, prev_handle->width);
    src_rect->h = SEC_MAX(cur->sourceCrop.bottom - cur->sourceCrop.top, 0);
    src_rect->h = SEC_MIN(src_rect->h, prev_handle->height);
    src_rect->full_w = prev_handle->width;
    src_rect->full_h = prev_handle->height;
    src_rect->virt_addr = (char *)prev_handle->base_addr;
    src_rect->color_format = prev_handle->format;

    dst_rect->x = 0;
    dst_rect->y = 0;
    dst_rect->w = win->rect_info.w;
    dst_rect->h = win->rect_info.h;
    dst_rect->full_w =  win->lcd_info.xres;
    dst_rect->full_h =  win->lcd_info.yres; 
    dst_rect->phys_addr = win->addr[win->buf_index];

    switch (win->lcd_info.bits_per_pixel) {
    case 32:
        dst_rect->color_format  = HAL_PIXEL_FORMAT_RGBX_8888;
        break;
    default:
        dst_rect->color_format  = HAL_PIXEL_FORMAT_RGB_565;
        break;
    }    
    
    return 0;
}
#endif

static int get_hwc_compos_decision(hwc_layer_t* cur, int iter, int win_cnt)
{
    if ((cur->flags & HWC_SKIP_LAYER  || !cur->handle) ||
        (cur->blending != HWC_BLENDING_NONE)){
        SEC_HWC_Log(HWC_LOG_DEBUG, "%s::is_skip_layer  %d  cur->handle %x ", __func__, cur->flags & HWC_SKIP_LAYER, cur->handle);
        return HWC_FRAMEBUFFER;
    }

    sec_native_handle_t *prev_handle = (sec_native_handle_t *)(cur->handle);
    int compositionType = HWC_FRAMEBUFFER;

    if(iter == 0){
	/* check here....if we have any resolution constraints */
	if (((cur->sourceCrop.right - cur->sourceCrop.left) < 16) || 
		((cur->sourceCrop.bottom - cur->sourceCrop.top) < 8) )
        return compositionType;

	if ((cur->transform == HAL_TRANSFORM_ROT_90) || (cur->transform == HAL_TRANSFORM_ROT_270)){
            if(((cur->displayFrame.right - cur->displayFrame.left) < 4)|| 
                ((cur->displayFrame.bottom - cur->displayFrame.top) < 8))
	   	return compositionType;
	}else if (((cur->displayFrame.right - cur->displayFrame.left) < 8) || 
                 ((cur->displayFrame.bottom - cur->displayFrame.top) < 4))
		return compositionType;

	    switch (prev_handle->format) {
		    case HAL_PIXEL_FORMAT_YV12:
		    case HAL_PIXEL_FORMAT_YCbCr_422_SP:
		    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
		    case HAL_PIXEL_FORMAT_YCbCr_422_I:
		    case HAL_PIXEL_FORMAT_YCbCr_422_P:
		    case HAL_PIXEL_FORMAT_YCbCr_420_P:
		    case HAL_PIXEL_FORMAT_YCbCr_420_I:
		    case HAL_PIXEL_FORMAT_CbYCrY_422_I:
		    case HAL_PIXEL_FORMAT_CbYCrY_420_I:
		    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
		    case HAL_PIXEL_FORMAT_YCrCb_422_SP:
		    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP:
		    case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_420_SP:
		    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP_TILED:
		    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_SP:
		    case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_SP:
		    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_I:
		    case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_I:
		    case HAL_PIXEL_FORMAT_CUSTOM_CbYCrY_422_I:
		    case HAL_PIXEL_FORMAT_CUSTOM_CrYCbY_422_I:
		         compositionType = HWC_OVERLAY;
		         break;
		    default:
		        compositionType = HWC_FRAMEBUFFER;
		        break;
	    }
	}
#ifdef SUB_TITLES_HWC
	else if((win_cnt > 0) && (prev_handle->usage &  GRALLOC_USAGE_EXTERNAL_DISP)){
	    switch (prev_handle->format) {
            case    HAL_PIXEL_FORMAT_RGBA_8888:
	    case    HAL_PIXEL_FORMAT_RGBX_8888:
	    case    HAL_PIXEL_FORMAT_BGRA_8888:
            case    HAL_PIXEL_FORMAT_RGB_888:
		    case HAL_PIXEL_FORMAT_RGB_565:
            case    HAL_PIXEL_FORMAT_RGBA_5551:
            case    HAL_PIXEL_FORMAT_RGBA_4444:                  
				compositionType = HWC_OVERLAY;
				break;
			default:
                compositionType = HWC_FRAMEBUFFER;
                break;
	    }

        SEC_HWC_Log(HWC_LOG_DEBUG, "2nd iter###%s:: compositionType %d  bpp %d format %x src[%d %d %d %d] dst[%d %d %d %d] srcImg[%d %d]",
            __func__, compositionType, prev_handle->bpp, prev_handle->format,
            cur->sourceCrop.left, cur->sourceCrop.right, cur->sourceCrop.top, cur->sourceCrop.bottom,
            cur->displayFrame.left, cur->displayFrame.right, cur->displayFrame.top, cur->displayFrame.bottom, 
            prev_handle->width, prev_handle->height);
	}
#endif

    SEC_HWC_Log(HWC_LOG_DEBUG, "%s:: compositionType %d  bpp %d format %x ",
        __func__,compositionType, prev_handle->bpp, prev_handle->format);

    return  compositionType;
}

static int assign_overlay_window(struct hwc_context_t *ctx,
                                 hwc_layer_t *cur,
                                 int win_idx,
                                 int layer_idx)
{
    struct hwc_win_info_t   *win;
	sec_rect   rect;
    int ret = 0;

    if(NUM_OF_WIN <= win_idx)
        return -1;

    win = &ctx->win[win_idx];

    rect.x = SEC_MAX(cur->displayFrame.left, 0);
    rect.y = SEC_MAX(cur->displayFrame.top, 0);
    rect.w = SEC_MIN(cur->displayFrame.right - rect.x, win->lcd_info.xres - rect.x);
    rect.h = SEC_MIN(cur->displayFrame.bottom - rect.y, win->lcd_info.yres - rect.y);

    if((rect.x != win->rect_info.x) || (rect.y != win->rect_info.y) ||
        (rect.w != win->rect_info.w) || (rect.h != win->rect_info.h)){
            win->rect_info.x = rect.x;
            win->rect_info.y = rect.y;
            win->rect_info.w = rect.w;
            win->rect_info.h = rect.h;

            //turnoff the window and set the window position with new conf...
            if(window_set_pos(win) < 0){
                SEC_HWC_Log(HWC_LOG_ERROR, "%s::window_set_pos is failed : %s", __func__, strerror(errno));
                return -1;
            }
            win->layer_prev_buf = 0;
	}

    win->layer_index = layer_idx;
    //win->win_index = win_idx;
    win->status = HWC_WIN_RESERVED;

    SEC_HWC_Log(HWC_LOG_DEBUG, "%s:: win_x %d win_y %d win_w %d win_h %d  lay_idx %d win_idx %d",
        __func__,
        win->rect_info.x, win->rect_info.y,
        win->rect_info.w, win->rect_info.h,
        win->layer_index, win_idx );

    return 0;
}

#if defined(S5P_BOARD_USES_HDMI)
static void BlockSignals(void)
{
    sigset_t signal_set;
    /* add all signals */
    sigfillset(&signal_set);
    /* set signal mask */
    pthread_sigmask(SIG_BLOCK, &signal_set, 0);
}

static void *blit2Hdmi(void *arg)
{
    hwc_context_t *ctx = (hwc_context_t *)arg;

    sec_img temp_hdmi_src_img;
    sec_rect temp_hdmi_src_rect;
    int temp_num_of_hwc_layer, temp_num_of_fb_layer;
    unsigned int temp_prevLCDAddr = 0;

    BlockSignals();
    android::SecHdmiClient *mHdmiClient = android::SecHdmiClient::getInstance();

    while(1) {
        pthread_mutex_lock(&sync_mutex);
        pthread_cond_wait(&sync_condition, &sync_mutex);
        temp_hdmi_src_img = hdmi_src_img;
        temp_hdmi_src_rect = hdmi_src_rect;
        temp_num_of_hwc_layer = num_of_hwc_layer;
        temp_num_of_fb_layer = num_of_fb_layer;
        temp_prevLCDAddr = prevLCDAddr;
        pthread_mutex_unlock(&sync_mutex);

        if (mHdmiClient) {
            if (temp_num_of_hwc_layer == 0) {
                unsigned int curLCDAddr = 0;
                int time_out = 10;
                int ret_val = true;

                /* wait until pan_display() */
                do {
                    if (LCDFd != -1) {
                        if (ioctl(LCDFd, S3CFB_GET_LCD_ADDR, &curLCDAddr) == -1) {
                            LOGE("%s:ioctl(S3CFB_GET_LCD_ADDR) fail\n", __func__);
                            ret_val = false;
                            break;
                        }
                    } else {
                        LOGE("%s:LCDFd is -1\n", __func__);
                        ret_val = false;
                        break;
                    }

                    if (temp_prevLCDAddr != curLCDAddr)
                        break;

                    usleep(3000);
                    time_out--;
                } while(time_out);

                if (ret_val)
                    mHdmiClient->blit2Hdmi(lcd_width, lcd_height,
                                HAL_PIXEL_FORMAT_BGRA_8888, android::SecHdmiClient::HDMI_MODE_UI,
                                curLCDAddr, 0, temp_num_of_hwc_layer);

            } else if (temp_num_of_hwc_layer == 1) {
                if (temp_hdmi_src_img.format == HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP_TILED) {
                    ADDRS * addr = (ADDRS *)(temp_hdmi_src_img.base);
                    mHdmiClient->blit2Hdmi(temp_hdmi_src_rect.w, temp_hdmi_src_rect.h,
                            temp_hdmi_src_img.format, android::SecHdmiClient::HDMI_MODE_VIDEO,
                            (unsigned int)addr->addr_y, (unsigned int)addr->addr_cbcr,
                            temp_num_of_hwc_layer);
                } else if (temp_hdmi_src_img.format == HAL_PIXEL_FORMAT_YCbCr_420_P) {
                    mHdmiClient->blit2Hdmi(temp_hdmi_src_rect.w, temp_hdmi_src_rect.h,
                            temp_hdmi_src_img.format, android::SecHdmiClient::HDMI_MODE_VIDEO,
                            (unsigned int)ctx->fimc.params.src.buf_addr_phy_rgb_y,
                            (unsigned int)ctx->fimc.params.src.buf_addr_phy_cb,
                            temp_num_of_hwc_layer);
                }
#if defined(S5P_BOARD_USES_HDMI_SUBTITLES)
                if (temp_num_of_fb_layer != 0 ) {
                    mHdmiClient->blit2Hdmi(lcd_width, lcd_height,
                            HAL_PIXEL_FORMAT_BGRA_8888, android::SecHdmiClient::HDMI_MODE_UI,
                            0, 0, temp_num_of_hwc_layer);
                }
#endif
            }
        }
    }
    return NULL;
}
#endif

static int hwc_prepare(hwc_composer_device_t *dev, hwc_layer_list_t* list) {

    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    int overlay_win_cnt = 0;
    int compositionType = 0;
    int ret;
    struct sec_img src_img;
    struct sec_img dst_img;
    struct sec_rect src_work_rect;
    struct sec_rect dst_work_rect;

    //if geometry is not changed, there is no need to do any work here
    if( !list || (!(list->flags & HWC_GEOMETRY_CHANGED)))
        return 0;

    //all the windows are free here....
    for (int i = 0; i < NUM_OF_WIN; i++) {
        ctx->win[i].status = HWC_WIN_FREE;
        ctx->win[i].buf_index = 0;
    }

    ctx->num_of_hwc_layer = 0;
    ctx->num_of_fb_layer = 0;
    ctx->num_2d_blit_layer = 0;

    SEC_HWC_Log(HWC_LOG_DEBUG, "%s:: hwc_prepare list->numHwLayers  %d ", __func__, list->numHwLayers);

    for (unsigned int i = 0; i < list->numHwLayers ; i++) {
        hwc_layer_t* cur = &list->hwLayers[i];

        if (overlay_win_cnt < NUM_OF_WIN) {
            compositionType = get_hwc_compos_decision(cur, 0, overlay_win_cnt);

            if (compositionType == HWC_FRAMEBUFFER){
                cur->compositionType = HWC_FRAMEBUFFER;
                ctx->num_of_fb_layer++;
            } else {
                ret = assign_overlay_window(ctx, cur, overlay_win_cnt, i);
                if (ret != 0) {
                    cur->compositionType = HWC_FRAMEBUFFER;
                    ctx->num_of_fb_layer++;
                    continue;
                }

                cur->compositionType = HWC_OVERLAY;
                cur->hints = HWC_HINT_CLEAR_FB;
                overlay_win_cnt++;
                ctx->num_of_hwc_layer++;
            }
        } else {
            cur->compositionType = HWC_FRAMEBUFFER;
            ctx->num_of_fb_layer++;
        }
    }

#ifdef SUB_TITLES_HWC
    for (int i = 0; i < list->numHwLayers ; i++) {
        if (overlay_win_cnt < NUM_OF_WIN) {
            hwc_layer_t* cur = &list->hwLayers[i];
            if(get_hwc_compos_decision(cur, 1, overlay_win_cnt) == HWC_OVERLAY){
                ret = assign_overlay_window(ctx, cur, overlay_win_cnt, i);
                if (ret == 0) {
                    cur->compositionType = HWC_OVERLAY;
                    cur->hints = HWC_HINT_CLEAR_FB;
                    overlay_win_cnt++;
                    ctx->num_of_hwc_layer++;
                    ctx->num_of_fb_layer--; 					
                    ctx->num_2d_blit_layer = 1;                    
                }
            }
        }
        else
            break;
    }
#endif

    if(list->numHwLayers != (ctx->num_of_fb_layer + ctx->num_of_hwc_layer))
        SEC_HWC_Log(HWC_LOG_DEBUG, "%s:: numHwLayers %d num_of_fb_layer %d num_of_hwc_layer %d ",
        __func__, list->numHwLayers, ctx->num_of_fb_layer, ctx->num_of_hwc_layer);

        //turn off the free windows
    for (int i = overlay_win_cnt; i < NUM_OF_WIN; i++)
            window_hide(&ctx->win[i]);

    return 0;
}

static int hwc_set(hwc_composer_device_t *dev,
                   hwc_display_t dpy,
                   hwc_surface_t sur,
                   hwc_layer_list_t* list)
{
    struct hwc_context_t *ctx = (struct hwc_context_t *)dev;
    int skipped_window_mask = 0;
    hwc_layer_t* cur;
    struct hwc_win_info_t   *win;
    int ret;
    struct sec_img src_img;
    struct sec_img dst_img;
    struct sec_rect src_work_rect;
    struct sec_rect dst_work_rect;
    int num_of_dirty_fb_layers = 0;

    if (!list) {
        //turn off the all windows
        for (int i = 0; i < NUM_OF_WIN; i++) {
            window_hide(&ctx->win[i]);
            ctx->win[i].status = HWC_WIN_FREE;
        }
        ctx->num_of_hwc_layer = 0;
        ctx->num_2d_blit_layer = 0;
        return 0;
    }

    if(ctx->num_of_hwc_layer > NUM_OF_WIN)
        ctx->num_of_hwc_layer = NUM_OF_WIN;

    //compose hardware layers here
    for (unsigned int i = 0; i < ctx->num_of_hwc_layer - ctx->num_2d_blit_layer; i++) {
        win = &ctx->win[i];
        if (win->status == HWC_WIN_RESERVED) {
            cur = &list->hwLayers[win->layer_index];

            if (cur->compositionType == HWC_OVERLAY) {
                //Skip duplicate frame rendering
                if (win->layer_prev_buf == (uint32_t)cur->handle)
                    continue;

                win->layer_prev_buf = (uint32_t)cur->handle;

                // initialize the src & dist context for fimc
                set_src_dst_img (cur, win, &src_img, &dst_img, i);
                set_src_dst_rect(cur, win, &src_work_rect, &dst_work_rect);

#if defined(S5P_BOARD_USES_HDMI)
                pthread_mutex_lock(&sync_mutex);
                memcpy(&hdmi_src_img, &src_img, sizeof(sec_img));
                memcpy(&hdmi_src_rect, &src_work_rect, sizeof(sec_rect));
                pthread_mutex_unlock(&sync_mutex);
#endif

               ret = runFimc(ctx,
                        &src_img, &src_work_rect,
                        &dst_img, &dst_work_rect,
                        cur->transform);
               if(ret < 0){
                   SEC_HWC_Log(HWC_LOG_ERROR, "%s::runFimc fail : ret=%d\n", __func__, ret);
                   skipped_window_mask |= (1 << i);
                   continue;
                }

                window_pan_display(win);

                win->buf_index = (win->buf_index + 1) % NUM_OF_WIN_BUF;
                if(win->power_state == 0)
                    window_show(win);
            } else {
                SEC_HWC_Log(HWC_LOG_ERROR, "%s:: error : layer %d compositionType should have been HWC_OVERLAY  ", __func__, win->layer_index);
                skipped_window_mask |= (1 << i);
                continue;
            }
         } else {
             SEC_HWC_Log(HWC_LOG_ERROR, "%s:: error : window status should have been HWC_WIN_RESERVED by now... ", __func__);
             skipped_window_mask |= (1 << i);
             continue;
         }
    }

#ifdef SUB_TITLES_HWC
    if (ctx->num_2d_blit_layer) {
        win = &ctx->win[ctx->num_of_hwc_layer - 1];
        cur = &list->hwLayers[win->layer_index];
        G2dRect srcRect;
        G2dRect dstRect;
        set_src_dst_g2d_rect(cur, win, &srcRect, &dstRect);

        if (runG2d(ctx, &srcRect,  &dstRect, cur->transform) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::runG2d fail", __func__);
            skipped_window_mask |= (1 << (ctx->num_of_hwc_layer - 1));
            goto g2d_error;
        }

         window_pan_display(win);

         win->buf_index = (win->buf_index + 1) % NUM_OF_WIN_BUF;
         if(win->power_state == 0)
             window_show(win);
    }

g2d_error:
#endif

    for (unsigned int i = 0; i < list->numHwLayers ; i++) {
        hwc_layer_t* cur = &list->hwLayers[i];
	if(cur && (cur->flags & 0x80000000)){ //dirty region is not empty
	    num_of_dirty_fb_layers++;
	}
    }

    if (skipped_window_mask) {
        //turn off the free windows
        for (int i = 0; i < NUM_OF_WIN; i++){
            if (skipped_window_mask & (1 << i))
                window_hide(&ctx->win[i]);
        }
    }

    if (0 < num_of_dirty_fb_layers) {

#if defined(S5P_BOARD_USES_HDMI)
        pthread_mutex_lock(&sync_mutex);
        if (LCDFd != -1) {
            if (ioctl(LCDFd, S3CFB_GET_LCD_ADDR, &prevLCDAddr) == -1) {
                LOGE("%s:ioctl(S3CFB_GET_LCD_ADDR) fail\n", __func__);
            }
        }
        pthread_mutex_unlock(&sync_mutex);
#endif

        EGLBoolean sucess = eglSwapBuffers((EGLDisplay)dpy, (EGLSurface)sur);
        if (!sucess) {
            return HWC_EGL_ERROR;
        }
    }

#if defined(S5P_BOARD_USES_HDMI)
    pthread_mutex_lock(&sync_mutex);
    num_of_hwc_layer    = ctx->num_of_hwc_layer;
    num_of_fb_layer     = ctx->num_of_fb_layer;

    pthread_cond_signal(&sync_condition);               /* signal to blit2Hdmi thread */
    pthread_mutex_unlock(&sync_mutex);
#endif

    return 0;
}

static int hwc_device_close(struct hw_device_t *dev)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    int ret = 0;
    int i;
    if (ctx) {
        if (destroyFimc(&ctx->fimc) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::destroyFimc fail", __func__);
            ret = -1;
        }

        if (destroyMem(&ctx->s3c_mem) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::destroyMem fail", __func__);
            ret = -1;
        }

#ifdef USE_HW_PMEM
        if (destroyPmem(&ctx->sec_pmem) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::destroyPmem fail", __func__);
            ret = -1;
        }
#endif
        for (i = 0; i < NUM_OF_WIN; i++) {
            if (window_close(&ctx->win[i]) < 0)
                SEC_HWC_Log(HWC_LOG_DEBUG, "%s::window_close() fail", __func__);
        }

        free(ctx);
    }

#if defined(S5P_BOARD_USES_HDMI)
    if (LCDFd != -1) {
        close(LCDFd);
        LCDFd = -1;
    }
#endif

    return ret;
}

/*****************************************************************************/

static int hwc_device_open(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device)
{
    int status = 0;
    struct hwc_win_info_t   *win;

    if (strcmp(name, HWC_HARDWARE_COMPOSER))
        return  -EINVAL;

    struct hwc_context_t *dev;
    dev = (hwc_context_t*)malloc(sizeof(*dev));

    /* initialize our state here */
    memset(dev, 0, sizeof(*dev));

    /* initialize the procs */
    dev->device.common.tag = HARDWARE_DEVICE_TAG;
    dev->device.common.version = 0;
    dev->device.common.module = const_cast<hw_module_t*>(module);
    dev->device.common.close = hwc_device_close;

    dev->device.prepare = hwc_prepare;
    dev->device.set = hwc_set;

    *device = &dev->device.common;

    //initializing
    memset(&(dev->fimc),    0, sizeof(s5p_fimc_t));
    memset(&(dev->s3c_mem), 0, sizeof(struct s3c_mem_t));
#ifdef USE_HW_PMEM
    memset(&(dev->sec_pmem),    0, sizeof(sec_pmem_t));
#endif
     /* open WIN0 & WIN1 here */
     for (int i = 0; i < NUM_OF_WIN; i++) {
        if (window_open(&(dev->win[i]), i)  < 0) {
             SEC_HWC_Log(HWC_LOG_ERROR, "%s:: Failed to open window %d device ", __func__, i);
             status = -EINVAL;
             goto err;
        }
     }

    if (window_get_global_lcd_info(dev->win[0].fd, &dev->lcd_info) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::window_get_global_lcd_info is failed : %s", __func__, strerror(errno));
        status = -EINVAL;
        goto err;
    }

    dev->lcd_info.yres_virtual = dev->lcd_info.yres * NUM_OF_WIN_BUF;

#if defined(S5P_BOARD_USES_HDMI)
    lcd_width   = dev->lcd_info.xres;
    lcd_height  = dev->lcd_info.yres;
#endif

    /* initialize the window context */
    for (int i = 0; i < NUM_OF_WIN; i++) {
        win = &dev->win[i];
        memcpy(&win->lcd_info, &dev->lcd_info, sizeof(struct fb_var_screeninfo));
        memcpy(&win->var_info, &dev->lcd_info, sizeof(struct fb_var_screeninfo));

        win->rect_info.x = 0;
        win->rect_info.y = 0;
        win->rect_info.w = win->var_info.xres;
        win->rect_info.h = win->var_info.yres;

        if (window_set_pos(win) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::window_set_pos is failed : %s", __func__, strerror(errno));
            status = -EINVAL;
            goto err;
        }

        if (window_get_info(win) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::window_get_info is failed : %s", __func__, strerror(errno));
            status = -EINVAL;
            goto err;
        }

        win->size = win->fix_info.line_length * win->var_info.yres;

        for (int j = 0; j < NUM_OF_WIN_BUF; j++) {
            win->addr[j] = win->fix_info.smem_start + (win->size * j);
            SEC_HWC_Log(HWC_LOG_DEBUG, "%s::win-%d add[%d]  %x ", __func__, i, j,  win->addr[j]);
        }
    }

#ifdef USE_HW_PMEM
    if (createPmem(&dev->sec_pmem, PMEM_SIZE) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::initPmem(%d) fail", __func__, PMEM_SIZE);
    }
#endif

    if (createMem(&dev->s3c_mem, 0, 0) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::createMem() fail (size=0)", __func__);
        status = -EINVAL;
        goto err;
    }

    //create PP
    if (createFimc(&dev->fimc) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::creatFimc() fail", __func__);
        status = -EINVAL;
        goto err;
    }

#if defined(S5P_BOARD_USES_HDMI)
    LOGD(">>> Run service");
    android::SecTVOutService::instantiate();

    if (!pthread_blit2Hdmi_created) {
        if (pthread_create(&pthread_blit2Hdmi, NULL, &blit2Hdmi, dev)) {
            pthread_blit2Hdmi_created = false;
            LOGE("%s:: pthread_create() failed!", __func__);
        } else {
            pthread_blit2Hdmi_created = true;
            LOGD("%s:: Thread for blit2Hdmi is started\n", __func__);
            pthread_cond_init(&sync_condition, NULL);
            pthread_mutex_init(&sync_mutex, NULL);
        }

        char const * const device_template[] = {
            "/dev/graphics/fb%u",
            "/dev/fb%u",
            0 };

        int i = 0;
        char name[64];

        while ((LCDFd==-1) && device_template[i]) {
            snprintf(name, 64, device_template[i], 0);
            LCDFd = open(name, O_RDWR, 0);
            i++;
        }
    }
#endif

    SEC_HWC_Log(HWC_LOG_DEBUG, "%s:: hwc_device_open: SUCCESS \n", __func__);

    return 0;

err:
    if (destroyFimc(&dev->fimc) < 0)
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::destroyFimc() fail", __func__);

    if (destroyMem(&dev->s3c_mem) < 0)
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::destroyMem() fail", __func__);

#ifdef USE_HW_PMEM
    if (destroyPmem(&dev->sec_pmem) < 0)
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::destroyPmem() fail", __func__);
#endif

    for (int i = 0; i < NUM_OF_WIN; i++) {
        if (window_close(&dev->win[i]) < 0)
            SEC_HWC_Log(HWC_LOG_DEBUG, "%s::window_close() fail", __func__);
    }

    return status;
}

