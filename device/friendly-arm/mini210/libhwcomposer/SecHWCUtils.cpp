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

#include "SecHWCUtils.h"

struct yuv_fmt_list yuv_list[] = {
    { "V4L2_PIX_FMT_NV12",      "YUV420/2P/LSB_CBCR",   V4L2_PIX_FMT_NV12,     12, 2 },
    { "V4L2_PIX_FMT_NV12T",     "YUV420/2P/LSB_CBCR",   V4L2_PIX_FMT_NV12T,    12, 2 },
    { "V4L2_PIX_FMT_NV21",      "YUV420/2P/LSB_CRCB",   V4L2_PIX_FMT_NV21,     12, 2 },
    { "V4L2_PIX_FMT_NV21X",     "YUV420/2P/MSB_CBCR",   V4L2_PIX_FMT_NV21X,    12, 2 },
    { "V4L2_PIX_FMT_NV12X",     "YUV420/2P/MSB_CRCB",   V4L2_PIX_FMT_NV12X,    12, 2 },
    { "V4L2_PIX_FMT_YUV420",    "YUV420/3P",            V4L2_PIX_FMT_YUV420,   12, 3 },
    { "V4L2_PIX_FMT_YUYV",      "YUV422/1P/YCBYCR",     V4L2_PIX_FMT_YUYV,     16, 1 },
    { "V4L2_PIX_FMT_YVYU",      "YUV422/1P/YCRYCB",     V4L2_PIX_FMT_YVYU,     16, 1 },
    { "V4L2_PIX_FMT_UYVY",      "YUV422/1P/CBYCRY",     V4L2_PIX_FMT_UYVY,     16, 1 },
    { "V4L2_PIX_FMT_VYUY",      "YUV422/1P/CRYCBY",     V4L2_PIX_FMT_VYUY,     16, 1 },
    { "V4L2_PIX_FMT_UV12",      "YUV422/2P/LSB_CBCR",   V4L2_PIX_FMT_NV16,     16, 2 },
    { "V4L2_PIX_FMT_UV21",      "YUV422/2P/LSB_CRCB",   V4L2_PIX_FMT_NV61,     16, 2 },
    { "V4L2_PIX_FMT_UV12X",     "YUV422/2P/MSB_CBCR",   V4L2_PIX_FMT_NV16X,    16, 2 },
    { "V4L2_PIX_FMT_UV21X",     "YUV422/2P/MSB_CRCB",   V4L2_PIX_FMT_NV61X,    16, 2 },
    { "V4L2_PIX_FMT_YUV422P",   "YUV422/3P",            V4L2_PIX_FMT_YUV422P,  16, 3 },
};

int window_open(struct hwc_win_info_t *win, int id)
{
    int fd = 0;
    char name[64];
    int vsync = 1;

    char const * const device_template = "/dev/graphics/fb%u";
    // window & FB maping
    // fb0 -> win-id : 2
    // fb1 -> win-id : 3
    // fb2 -> win-id : 4
    // fb3 -> win-id : 0
    // fb4 -> win_id : 1
    // it is pre assumed that ...win0 or win1 is used here..
    switch (id) {
    case 0:
    case 1:
        break;
    default:
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::id(%d) is weird", __func__, id);
        goto error;
    }

    snprintf(name, 64, device_template, id + 3);

    win->fd = open(name, O_RDWR);
    if (win->fd <= 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::Failed to open window device (%s) : %s", __func__, strerror(errno), device_template);
        goto error;
    }

#ifdef ENABLE_FIMD_VSYNC
    if (ioctl(win->fd, S3CFB_SET_VSYNC_INT, &vsync) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::S3CFB_SET_VSYNC_INT fail", __func__);
        goto error;
    }
#endif

    return 0;

error:
    if (0 < win->fd)
        close(win->fd);
    win->fd = 0;

    return -1;
}

int window_close(struct hwc_win_info_t *win)
{
    int ret = 0;

    if (0 < win->fd) {
#ifdef ENABLE_FIMD_VSYNC
        int vsync = 0;
        if (ioctl(win->fd, S3CFB_SET_VSYNC_INT, &vsync) < 0)
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::S3CFB_SET_VSYNC_INT fail", __func__);
#endif
        ret = close(win->fd);
    }
    win->fd = 0;

    return ret;
}

int window_set_pos(struct hwc_win_info_t *win)
{
    struct s3cfb_user_window window;

    //before changing the screen configuration...powerdown the window
    if (window_hide(win) != 0)
        return -1;

    win->var_info.xres = win->rect_info.w;
    win->var_info.yres = win->rect_info.h;

    win->var_info.activate &= ~FB_ACTIVATE_MASK;
    win->var_info.activate |= FB_ACTIVATE_FORCE;

    if (ioctl(win->fd, FBIOPUT_VSCREENINFO, &(win->var_info)) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::FBIOPUT_VSCREENINFO(%d, %d) fail",
          __func__, win->rect_info.w, win->rect_info.h);
        return -1;
    }

    window.x = win->rect_info.x;
    window.y = win->rect_info.y;

    if (ioctl(win->fd, S3CFB_WIN_POSITION, &window) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::S3CFB_WIN_POSITION(%d, %d) fail",
            __func__, window.x, window.y);
      return -1;
    }

    return 0;
}

int window_get_info(struct hwc_win_info_t *win)
{
    if (ioctl(win->fd, FBIOGET_FSCREENINFO, &win->fix_info) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "FBIOGET_FSCREENINFO failed : %s", strerror(errno));
        goto error;
    }

    return 0;

error:
    win->fix_info.smem_start = 0;

    return -1;
}

int window_pan_display(struct hwc_win_info_t *win)
{
    struct fb_var_screeninfo *lcd_info = &(win->lcd_info);

#ifdef ENABLE_FIMD_VSYNC
    if (ioctl(win->fd, FBIO_WAITFORVSYNC, 0) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::FBIO_WAITFORVSYNC fail(%s)", __func__, strerror(errno));
    }
#endif

    lcd_info->yoffset = lcd_info->yres * win->buf_index;

    if (ioctl(win->fd, FBIOPAN_DISPLAY, lcd_info) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::FBIOPAN_DISPLAY(%d / %d / %d) fail(%s)",
            __func__,
            lcd_info->yres,
            win->buf_index, lcd_info->yres_virtual,
            strerror(errno));
        return -1;
    }
    return 0;
}

int window_show(struct hwc_win_info_t *win)
{
    if (win->power_state == 0) {
        if (ioctl(win->fd, FBIOBLANK, FB_BLANK_UNBLANK) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s: FBIOBLANK failed : (%d:%s)",
                __func__, win->fd, strerror(errno));
            return -1;
        }
        win->power_state = 1;
    }
    return 0;
}

int window_hide(struct hwc_win_info_t *win)
{
    if (win->power_state == 1) {
        if (ioctl(win->fd, FBIOBLANK, FB_BLANK_POWERDOWN) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::FBIOBLANK failed : (%d:%s)",
             __func__, win->fd, strerror(errno));
            return -1;
        }
        win->power_state = 0;
    }
    return 0;
}

int window_get_global_lcd_info(int fd, struct fb_var_screeninfo *lcd_info)
{
    if (ioctl(fd, FBIOGET_VSCREENINFO, lcd_info) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "FBIOGET_VSCREENINFO failed : %s", strerror(errno));
        return -1;
    }

    if (lcd_info->xres == 0) {
        lcd_info->xres = DEFAULT_LCD_WIDTH;
        lcd_info->xres_virtual = DEFAULT_LCD_WIDTH;
    }

    if (lcd_info->yres == 0) {
        lcd_info->yres = DEFAULT_LCD_HEIGHT;
        lcd_info->yres_virtual = DEFAULT_LCD_HEIGHT * NUM_OF_WIN_BUF;
    }

    if (lcd_info->bits_per_pixel == 0)
        lcd_info->bits_per_pixel = DEFAULT_LCD_BPP;

    return 0;
}

int fimc_v4l2_set_src(int fd, unsigned int hw_ver, s5p_fimc_img_info *src)
{
    struct v4l2_format  fmt;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop    crop;
    struct v4l2_requestbuffers req;

    /*
     * To set size & format for source image (DMA-INPUT)
     */
    fmt.type                = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    fmt.fmt.pix.width       = src->full_width;
    fmt.fmt.pix.height      = src->full_height;
    fmt.fmt.pix.pixelformat = src->color_space;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE;

    if (ioctl (fd, VIDIOC_S_FMT, &fmt) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "VIDIOC_S_FMT failed : errno=%d (%s) : fd=%d", errno, strerror(errno), fd);
        return -1;
    }

    /*
     * crop input size
     */
    crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    if (0x50 == hw_ver) {
        crop.c.left   = src->start_x;
        crop.c.top    = src->start_y;
    } else {
        crop.c.left   = 0;
        crop.c.top    = 0;
    }
    crop.c.width  = src->width;
    crop.c.height = src->height;
    if (ioctl(fd, VIDIOC_S_CROP, &crop) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "Error in video VIDIOC_S_CROP (%d, %d, %d, %d)",
            crop.c.left, crop.c.top, crop.c.width, crop.c.height);
        return -1;
    }

    /*
     * input buffer type
     */
    req.count       = 1;
    req.type        = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    req.memory      = V4L2_MEMORY_USERPTR;

    if (ioctl (fd, VIDIOC_REQBUFS, &req) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "Error in VIDIOC_REQBUFS");
        return -1;
    }

    return 0;
}

int fimc_v4l2_set_dst(int fd,
                      s5p_fimc_img_info *dst,
                      int rotation,
                      int flag_h_flip,
                      int flag_v_flip,
                      unsigned int addr)
{
    struct v4l2_format      sFormat;
    struct v4l2_control     vc;
    struct v4l2_framebuffer fbuf;

    /*
     * set rotation configuration
     */
    vc.id = V4L2_CID_HFLIP;
    vc.value = flag_h_flip;
    if (ioctl(fd, VIDIOC_S_CTRL, &vc) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "Error in video VIDIOC_S_CTRL - flag_h_flip (%d)", flag_h_flip);
        return -1;
    }

    vc.id = V4L2_CID_VFLIP;
    vc.value = flag_v_flip;
    if (ioctl(fd, VIDIOC_S_CTRL, &vc) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "Error in video VIDIOC_S_CTRL - flag_v_flip (%d)", flag_v_flip);
        return -1;
    }

    vc.id = V4L2_CID_ROTATION;
    vc.value = rotation;
    if (ioctl(fd, VIDIOC_S_CTRL, &vc) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "Error in video VIDIOC_S_CTRL - rotation (%d)", rotation);
        return -1;
    }

    /*
     * set size, format & address for destination image (DMA-OUTPUT)
     */
    if (ioctl (fd, VIDIOC_G_FBUF, &fbuf) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "Error in video VIDIOC_G_FBUF");
        return -1;
    }

    fbuf.base            = (void *)addr;
    fbuf.fmt.width       = dst->full_width;
    fbuf.fmt.height      = dst->full_height;
    fbuf.fmt.pixelformat = dst->color_space;
    if (ioctl (fd, VIDIOC_S_FBUF, &fbuf) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "Error in video VIDIOC_S_FBUF 0x%x %d %d %d",
            (void *)addr, dst->full_width, dst->full_height, dst->color_space);
        return -1;
    }

    /*
     * set destination window
     */
    sFormat.type             = V4L2_BUF_TYPE_VIDEO_OVERLAY;
    sFormat.fmt.win.w.left   = dst->start_x;
    sFormat.fmt.win.w.top    = dst->start_y;
    sFormat.fmt.win.w.width  = dst->width;
    sFormat.fmt.win.w.height = dst->height;
    if (ioctl(fd, VIDIOC_S_FMT, &sFormat) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "Error in video VIDIOC_S_FMT %d %d %d %d",
            dst->start_x, dst->start_y, dst->width, dst->height);
        return -1;
    }

    return 0;
}

int fimc_v4l2_stream_on(int fd, enum v4l2_buf_type type)
{
    if (ioctl (fd, VIDIOC_STREAMON, &type) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "Error in VIDIOC_STREAMON");
        return -1;
    }

    return 0;
}

int fimc_v4l2_queue(int fd, struct fimc_buf *fimc_buf)
{
    struct v4l2_buffer buf;

    buf.type        = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    buf.memory      = V4L2_MEMORY_USERPTR;
    buf.m.userptr   = (unsigned long)fimc_buf;
    buf.length      = 0;
    buf.index       = 0;

    if (ioctl (fd, VIDIOC_QBUF, &buf) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "Error in VIDIOC_QBUF");
        return -1;
    }

    return 0;
}

int fimc_v4l2_dequeue(int fd)
{
    struct v4l2_buffer buf;
    buf.type   = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    buf.memory = V4L2_MEMORY_USERPTR;

    if (ioctl (fd, VIDIOC_DQBUF, &buf) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "Error in VIDIOC_DQBUF");
        return -1;
    }

    return buf.index;
}

int fimc_v4l2_stream_off(int fd)
{
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    if (ioctl (fd, VIDIOC_STREAMOFF, &type) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "Error in VIDIOC_STREAMOFF");
        return -1;
    }

    return 0;
}

int fimc_v4l2_clr_buf(int fd)
{
    struct v4l2_requestbuffers req;

    req.count   = 0;
    req.type    = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    req.memory  = V4L2_MEMORY_USERPTR;

    if (ioctl (fd, VIDIOC_REQBUFS, &req) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "Error in VIDIOC_REQBUFS");
        return -1;
    }

    return 0;
}

int fimc_handle_oneshot(int fd, struct fimc_buf *fimc_buf)
{
    if (fimc_v4l2_stream_on(fd, V4L2_BUF_TYPE_VIDEO_OUTPUT) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "Fail : v4l2_stream_on()");
        return -1;
    }

    if (fimc_v4l2_queue(fd, fimc_buf) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "Fail : v4l2_queue()");
        return -1;
    }

    if (fimc_v4l2_dequeue(fd) < 0) {
        LOGE("Fail : v4l2_dequeue()");
        return -1;
    }

    if (fimc_v4l2_stream_off(fd) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "Fail : v4l2_stream_off()");
        return -1;
    }

    if (fimc_v4l2_clr_buf(fd) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "Fail : v4l2_clr_buf()");
        return -1;
    }

    return 0;
}

/* memcpy_rect function could handle only YV12 format & YUV420P format.
 * use the default memcpy function for other formats */
static int memcpy_rect(void *dst, void *src, int fullW, int fullH, int realW, int realH, int format)
{
    unsigned char *srcCb, *srcCr;
    unsigned char *dstCb, *dstCr;
    unsigned char *srcY, *dstY;
    int srcCbOffset, srcCrOffset;
    int dstFrameOffset, dstCrOffset;
    int cbFullW, cbRealW;
    int cbSrcFW, ySrcFW;
    int i;

    if(format == HAL_PIXEL_FORMAT_YV12){ //yv12
        cbFullW = fullW >> 1;
        cbRealW = realW >> 1;
        dstFrameOffset = fullW * fullH;
        dstCrOffset = dstFrameOffset + (dstFrameOffset >> 2);
        dstY = (unsigned char *)dst;
        dstCb = (unsigned char *)dst + dstFrameOffset;
        dstCr = (unsigned char *)dst + dstCrOffset;
        ySrcFW = fullW;
        cbSrcFW = ySrcFW >> 1;
        //as defined in hardware.h, cb & cr full_width should be aligned to 16. ALIGN(y_stride/2, 16).
        //Alignment is hard coded to 16.
        //for example...check frameworks/media/libvideoeditor/lvpp/VideoEditorTools.cpp file for UV stride calculation.
        cbSrcFW = (cbSrcFW + 15) & (~15);
        srcCbOffset = ySrcFW * fullH;
        srcCrOffset = srcCbOffset + ((cbSrcFW * fullH) >> 1);
        srcY =  (unsigned char *)src;
        srcCr = (unsigned char *)src + srcCbOffset;
        srcCb = (unsigned char *)src + srcCrOffset;
    } else if(format == HAL_PIXEL_FORMAT_YCbCr_420_P){
        cbFullW = fullW >> 1;
        cbRealW = realW >> 1;
        dstFrameOffset = fullW * fullH;
        dstCrOffset = dstFrameOffset + (dstFrameOffset >> 2);
        dstY = (unsigned char *)dst;
        dstCb = (unsigned char *)dst + dstFrameOffset;
        dstCr = (unsigned char *)dst + dstCrOffset;
        if(fullW == realW){
            memcpy(dst, src, (dstFrameOffset * 3)>> 1);
            return 0;
        }
        cbSrcFW = cbRealW;
        ySrcFW = realW;
        srcCbOffset = realW * realH;
        srcCrOffset = srcCbOffset + (srcCbOffset >> 2);
        srcY =  (unsigned char *)src;
        srcCb = (unsigned char *)src + srcCbOffset;
        srcCr = (unsigned char *)src + srcCrOffset;
    } else {
        SEC_HWC_Log(HWC_LOG_ERROR, "use default memcpy instead of memcpy_rect");
        return -1;
    }

    for(i=0; i < realH; i++)
        memcpy(dstY + fullW * i, srcY + ySrcFW * i, realW);

    for(i=0; i < (realH >> 1); i++)
        memcpy(dstCb + cbFullW * i, srcCb + cbSrcFW * i, cbRealW);

    for(i=0; i < (realH >> 1); i++)
        memcpy(dstCr + cbFullW * i, srcCr + cbSrcFW * i, cbRealW);

    return 0;

}

static int get_src_phys_addr(struct hwc_context_t  *ctx, sec_img *src_img, sec_rect *src_rect)
{
    s5p_fimc_t *fimc = &ctx->fimc;
    struct s3c_mem_alloc *ptr_mem_alloc = &ctx->s3c_mem.mem_alloc[0];
#ifdef USE_HW_PMEM
    sec_pmem_alloc_t *pm_alloc = &ctx->sec_pmem.sec_pmem_alloc[0];
#endif

    unsigned int src_virt_addr  = 0;
    unsigned int src_phys_addr  = 0;
    unsigned int src_frame_size = 0;

    struct pmem_region region;
    ADDRS * addr;

    // error check routine
    if (0 == src_img->base) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s invalid src image base", __func__);
        return 0;
    }

    switch (src_img->mem_type) {
    case HWC_PHYS_MEM_TYPE:
        src_phys_addr = src_img->base + src_img->offset;
        break;

    case HWC_VIRT_MEM_TYPE:
    case HWC_UNKNOWN_MEM_TYPE:
        switch (src_img->format) {
        case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP:
        case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_420_SP:
        case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP_TILED:
        case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_SP:
        case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_SP:
            addr = (ADDRS *)(src_img->base);
            fimc->params.src.buf_addr_phy_rgb_y = addr->addr_y;
            fimc->params.src.buf_addr_phy_cb    = addr->addr_cbcr;

            src_phys_addr = fimc->params.src.buf_addr_phy_rgb_y;
            if (0 == src_phys_addr) {
                SEC_HWC_Log(HWC_LOG_ERROR, "%s address error (format=CUSTOM_YCbCr/YCrCb_420_SP Y-addr=0x%x CbCr-Addr=0x%x)",
                    __func__, fimc->params.src.buf_addr_phy_rgb_y, fimc->params.src.buf_addr_phy_cb);
                return 0;
            }
            break;
        case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_I:
        case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_I:
        case HAL_PIXEL_FORMAT_CUSTOM_CbYCrY_422_I:
        case HAL_PIXEL_FORMAT_CUSTOM_CrYCbY_422_I:
            addr = (ADDRS *)(src_img->base + src_img->offset);
            fimc->params.src.buf_addr_phy_rgb_y = addr->addr_y;
            src_phys_addr = fimc->params.src.buf_addr_phy_rgb_y;
            if (0 == src_phys_addr) {
                SEC_HWC_Log(HWC_LOG_ERROR, "%s address error (format=CUSTOM_YCbCr/CbYCrY_422_I Y-addr=0x%x)",
                    __func__, fimc->params.src.buf_addr_phy_rgb_y);
                return 0;
            }
            break;
        default:
            // copy
            src_frame_size = FRAME_SIZE(src_img->format, src_img->w, src_img->h);
            if (src_frame_size == 0) {
                SEC_HWC_Log(HWC_LOG_ERROR, "%s::FRAME_SIZE fail ", __func__);
                return 0;
            }

#ifdef USE_HW_PMEM
            if (0 <= checkPmem(&ctx->sec_pmem, 0, src_frame_size)) {
                src_virt_addr   = pm_alloc->virt_addr;
                src_phys_addr   = pm_alloc->phys_addr;
                pm_alloc->size  = src_frame_size;
            } else
#endif
            if (0 <= checkMem(&ctx->s3c_mem, 0, src_frame_size)) {
                src_virt_addr       = ptr_mem_alloc->vir_addr;
                src_phys_addr       = ptr_mem_alloc->phy_addr;
                ptr_mem_alloc->size = src_frame_size;
            } else {
                SEC_HWC_Log(HWC_LOG_ERROR, "%s::check_mem fail ", __func__);
                return 0;
            }

            if((src_img->format == HAL_PIXEL_FORMAT_YCbCr_420_P) ||
                (src_img->format == HAL_PIXEL_FORMAT_YV12)){
               if(memcpy_rect((void *)src_virt_addr, (void*)((unsigned int)src_img->base), src_img->w,
                              src_img->h, src_rect->w, src_rect->h, src_img->format) != 0)
                   return 0;
            } else
                memcpy((void *)src_virt_addr, (void*)((unsigned int)src_img->base), src_frame_size);

#ifdef USE_HW_PMEM
              if(pm_alloc->size  == src_frame_size){
                  region.offset = 0;
                  region.len = src_frame_size;
                  if (ioctl(ctx->sec_pmem.pmem_master_fd, PMEM_CACHE_FLUSH, &region) < 0)
                      SEC_HWC_Log(HWC_LOG_ERROR, "%s::pmem cache flush fail ", __func__);
              }
#endif
            break;
        }
    }

    return src_phys_addr;
}

static int get_dst_phys_addr(struct hwc_context_t *ctx, sec_img *dst_img, sec_rect *dst_rect, int *dst_memcpy_flag)
{
    unsigned int dst_phys_addr  = 0;

    if (HWC_PHYS_MEM_TYPE == dst_img->mem_type && 0 != dst_img->base)
        dst_phys_addr = dst_img->base;
    else {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::get_dst_phys_addr fail ", __func__);
        dst_phys_addr = 0;
    }
    return dst_phys_addr;
}

static inline int rotateValueHAL2PP(unsigned char transform, int *flag_h_flip, int *flag_v_flip)
{
    int rotate_result = 0;
    int rotate_flag = transform & 0x7;

    switch (rotate_flag) {
    case HAL_TRANSFORM_ROT_90:
        rotate_result = 90;
        break;
    case HAL_TRANSFORM_ROT_180:
        rotate_result = 180;
        break;
    case HAL_TRANSFORM_ROT_270:
        rotate_result = 270;
        break;
    }

    switch (rotate_flag) {
    case HAL_TRANSFORM_FLIP_H:
        *flag_h_flip = 1;
        *flag_v_flip = 0;
        break;
    case HAL_TRANSFORM_FLIP_V:
        *flag_h_flip = 0;
        *flag_v_flip = 1;
        break;
    default:
        *flag_h_flip = 0;
        *flag_v_flip = 0;
        break;
    }

    return rotate_result;
}

static inline int multipleOfN(int number, int N)
{
    int result = number;
    switch (N) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
    case 32:
    case 64:
    case 128:
    case 256:
        result = (number - (number & (N-1)));
        break;
    default:
        result = number - (number % N);
        break;
    }
    return result;
}

static inline int widthOfPP(unsigned int ver, int pp_color_format, int number)
{
    int result = number;

    if (0x50 == ver) {
        switch (pp_color_format) {
        /* 422 1/2/3 plane */
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_UYVY:
        case V4L2_PIX_FMT_NV61:
        case V4L2_PIX_FMT_NV16:
        case V4L2_PIX_FMT_YUV422P:
        /* 420 2/3 plane */
        case V4L2_PIX_FMT_NV21:
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_NV12T:
        case V4L2_PIX_FMT_YUV420:
            result = multipleOfN(number, 2);
            break;
        default :
            break;
        }
    } else {
        switch (pp_color_format) {
        case V4L2_PIX_FMT_RGB565:
            result = multipleOfN(number, 8);
            break;
        case V4L2_PIX_FMT_RGB32:
            result = multipleOfN(number, 4);
            break;
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_UYVY:
            result = multipleOfN(number, 4);
            break;
        case V4L2_PIX_FMT_NV61:
        case V4L2_PIX_FMT_NV16:
            result = multipleOfN(number, 8);
            break;
        case V4L2_PIX_FMT_YUV422P:
            result = multipleOfN(number, 16);
            break;
        case V4L2_PIX_FMT_NV21:
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_NV12T:
            result = multipleOfN(number, 8);
            break;
        case V4L2_PIX_FMT_YUV420:
            result = multipleOfN(number, 16);
            break;
        default :
            break;
        }
    }
    return result;
}

static inline int heightOfPP(int pp_color_format, int number)
{
    int result = number;

    switch (pp_color_format) {
    case V4L2_PIX_FMT_NV21:
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV12T:
    case V4L2_PIX_FMT_YUV420:
        result = multipleOfN(number, 2);
        break;
    default :
        break;
    }

    return result;
}

static unsigned int get_yuv_bpp(unsigned int fmt)
{
    int i, sel = -1;
    int yuv_list_size     = sizeof(yuv_list);
    int yuv_fmt_list_size = sizeof(struct yuv_fmt_list);
    int list_size         = (int)(yuv_list_size / yuv_fmt_list_size);

    for (i = 0; i < list_size; i++) {
        if (yuv_list[i].fmt == fmt) {
            sel = i;
            break;
        }
    }

    if (sel == -1)
        return sel;
    else
        return yuv_list[sel].bpp;
}

static unsigned int get_yuv_planes(unsigned int fmt)
{
    int i, sel = -1;
    int yuv_list_size     = sizeof(yuv_list);
    int yuv_fmt_list_size = sizeof(struct yuv_fmt_list);
    int list_size         = (int)(yuv_list_size / yuv_fmt_list_size);

    for (i = 0; i < list_size; i++) {
        if (yuv_list[i].fmt == fmt) {
            sel = i;
            break;
        }
    }

    if (sel == -1)
        return sel;
    else
        return yuv_list[sel].planes;
}

static int runcFimcCore(struct hwc_context_t *ctx,
                unsigned int src_phys_addr, sec_img *src_img, sec_rect *src_rect, uint32_t src_color_space,
                unsigned int dst_phys_addr, sec_img *dst_img, sec_rect *dst_rect, uint32_t dst_color_space,
                int transform)
{
    s5p_fimc_t        * fimc = &ctx->fimc;
    s5p_fimc_params_t * params = &(fimc->params);

    unsigned int    frame_size = 0;
    struct fimc_buf fimc_src_buf;

    int src_bpp, src_planes;
    int flag_h_flip = 0;
    int flag_v_flip = 0;
    int rotate_value = rotateValueHAL2PP(transform, &flag_h_flip, &flag_v_flip);

    // set post processor configuration
    params->src.full_width  = src_img->w;
    params->src.full_height = src_img->h;
    params->src.start_x     = src_rect->x;
    params->src.start_y     = src_rect->y;
    params->src.width       = widthOfPP(fimc->hw_ver, src_color_space, src_rect->w);
    params->src.height      = heightOfPP(src_color_space, src_rect->h);
    params->src.color_space = src_color_space;
    params->src.buf_addr_phy_rgb_y = src_phys_addr;

    // check minimum
    if (src_rect->w < 16 || src_rect->h < 8) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s src size is not supported by fimc : f_w=%d f_h=%d x=%d y=%d w=%d h=%d (ow=%d oh=%d) format=0x%x", __func__,
         params->src.full_width, params->src.full_height, params->src.start_x, params->src.start_y,
         params->src.width, params->src.height, src_rect->w, src_rect->h, params->src.color_space);
        return -1;
    }

    switch (rotate_value) {
    case 0:
        params->dst.full_width  = dst_img->w;
        params->dst.full_height = dst_img->h;

        params->dst.start_x     = dst_rect->x;
        params->dst.start_y     = dst_rect->y;

        params->dst.width       =
            widthOfPP(fimc->hw_ver, dst_color_space, dst_rect->w);
        params->dst.height      = heightOfPP(dst_color_space, dst_rect->h);
        break;
    case 90:
        params->dst.full_width  = dst_img->h;
        params->dst.full_height = dst_img->w;

        params->dst.start_x     = dst_rect->y;
        params->dst.start_y     = dst_img->w - (dst_rect->x + dst_rect->w);

        params->dst.width       =
            widthOfPP(fimc->hw_ver, dst_color_space, dst_rect->h);
        params->dst.height      =
            widthOfPP(fimc->hw_ver, dst_color_space, dst_rect->w);

        if (0x50 > fimc->hw_ver)
            params->dst.start_y     += (dst_rect->w - params->dst.height);
        break;
    case 180:
        params->dst.full_width  = dst_img->w;
        params->dst.full_height = dst_img->h;

        params->dst.start_x     = dst_img->w - (dst_rect->x + dst_rect->w);
        params->dst.start_y     = dst_img->h - (dst_rect->y + dst_rect->h);

        params->dst.width       =
            widthOfPP(fimc->hw_ver, dst_color_space, dst_rect->w);
        params->dst.height      = heightOfPP(dst_color_space, dst_rect->h);
        break;
    case 270:
        params->dst.full_width  = dst_img->h;
        params->dst.full_height = dst_img->w;

        params->dst.start_x     = dst_img->h - (dst_rect->y + dst_rect->h);
        params->dst.start_y     = dst_rect->x;

        params->dst.width       =
            widthOfPP(fimc->hw_ver, dst_color_space, dst_rect->h);
        params->dst.height      =
            widthOfPP(fimc->hw_ver, dst_color_space, dst_rect->w);

        if (0x50 > fimc->hw_ver)
            params->dst.start_y += (dst_rect->w - params->dst.height);
        break;
    }

    params->dst.color_space = dst_color_space;

    // check minimum
    if (dst_rect->w  < 8 || dst_rect->h < 4) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s dst size is not supported by fimc : f_w=%d f_h=%d x=%d y=%d w=%d h=%d (ow=%d oh=%d) format=0x%x", __func__,
            params->dst.full_width, params->dst.full_height, params->dst.start_x, params->dst.start_y,
            params->dst.width, params->dst.height, dst_rect->w, dst_rect->h, params->dst.color_space);
        return -1;
    }

    /* check scaling limit
     * the scaling limie must not be more than MAX_RESIZING_RATIO_LIMIT
     */
    if (((src_rect->w > dst_rect->w) && ((src_rect->w / dst_rect->w) > MAX_RESIZING_RATIO_LIMIT)) ||
        ((dst_rect->w > src_rect->w) && ((dst_rect->w / src_rect->w) > MAX_RESIZING_RATIO_LIMIT))) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s over scaling limit : src.w=%d dst.w=%d (limit=%d)", __func__, src_rect->w, dst_rect->w, MAX_RESIZING_RATIO_LIMIT);
        return -1;
    }

#if 0
    LOGD("%s::src(addr=0x%x w=%d h=%d) dst(addr=0x%x w=%d h=%d)",
    __func__,
    src_phys_addr, src_img->width, src_img->height, dst_phys_addr, params->dst.full_width, params->dst.full_height);
#endif

   /* set configuration related to destination (DMA-OUT)
     *   - set input format & size
     *   - crop input size
     *   - set input buffer
     *   - set buffer type (V4L2_MEMORY_USERPTR)
     */
    if (fimc_v4l2_set_dst(fimc->dev_fd,
                          &params->dst,
                          rotate_value,
                          flag_h_flip,
                          flag_v_flip,
                          dst_phys_addr) < 0) {
        return -1;
    }

   /* set configuration related to source (DMA-INPUT)
     *   - set input format & size
     *   - crop input size
     *   - set input buffer
     *   - set buffer type (V4L2_MEMORY_USERPTR)
     */
    if (fimc_v4l2_set_src(fimc->dev_fd, fimc->hw_ver, &params->src) < 0)
        return -1;

    // set input dma address (Y/RGB, Cb, Cr)
    switch (src_img->format) {
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP:
    case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP_TILED:
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_SP:
    case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_SP:
        // for video contents zero copy case
        fimc_src_buf.base[0] = params->src.buf_addr_phy_rgb_y;
        fimc_src_buf.base[1] = params->src.buf_addr_phy_cb;
        break;

    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_I:
    case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_I:
    case HAL_PIXEL_FORMAT_CUSTOM_CbYCrY_422_I:
    case HAL_PIXEL_FORMAT_CUSTOM_CrYCbY_422_I:
    case HAL_PIXEL_FORMAT_RGB_565:
        // for camera capture zero copy case
        fimc_src_buf.base[0] = params->src.buf_addr_phy_rgb_y;
        break;

    default:
        // set source image
        src_bpp    = get_yuv_bpp(src_color_space);
        src_planes = get_yuv_planes(src_color_space);

        fimc_src_buf.base[0] = params->src.buf_addr_phy_rgb_y;

        if (2 == src_planes) {
            frame_size = params->src.full_width * params->src.full_height;
            params->src.buf_addr_phy_cb = params->src.buf_addr_phy_rgb_y + frame_size;

            fimc_src_buf.base[1] = params->src.buf_addr_phy_cb;
        } else if (3 == src_planes) {
            frame_size = params->src.full_width * params->src.full_height;
            params->src.buf_addr_phy_cb = params->src.buf_addr_phy_rgb_y + frame_size;

            if (12 == src_bpp)
                 params->src.buf_addr_phy_cr = params->src.buf_addr_phy_cb + (frame_size >> 2);
            else
                 params->src.buf_addr_phy_cr = params->src.buf_addr_phy_cb + (frame_size >> 1);

            fimc_src_buf.base[1] = params->src.buf_addr_phy_cb;
            fimc_src_buf.base[2] = params->src.buf_addr_phy_cr;
        }
    }

    if (fimc_handle_oneshot(fimc->dev_fd, &fimc_src_buf) < 0) {
        fimc_v4l2_clr_buf(fimc->dev_fd);
        return -1;
    }

    return 0;
}

int createFimc(s5p_fimc_t *fimc)
{
    struct v4l2_capability cap;
    struct v4l2_format fmt;
    struct v4l2_control vc;

    #define  PP_DEVICE_DEV_NAME  "/dev/video1"

    // open device file
    if (fimc->dev_fd <= 0)
        fimc->dev_fd = open(PP_DEVICE_DEV_NAME, O_RDWR);

    if (fimc->dev_fd <= 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::Post processor open error (%d)", __func__, errno);
        goto err;
    }

    // check capability
    if (ioctl(fimc->dev_fd, VIDIOC_QUERYCAP, &cap) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "VIDIOC_QUERYCAP failed");
        goto err;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%d has no streaming support", fimc->dev_fd);
        goto err;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%d is no video output", fimc->dev_fd);
        goto err;
    }

    /*
     * malloc fimc_outinfo structure
     */
    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    if (ioctl(fimc->dev_fd, VIDIOC_G_FMT, &fmt) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::Error in video VIDIOC_G_FMT", __func__);
        goto err;
    }

    vc.id = V4L2_CID_FIMC_VERSION;
    vc.value = 0;

    if (ioctl(fimc->dev_fd, VIDIOC_G_CTRL, &vc) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::Error in video VIDIOC_G_CTRL", __func__);
        goto err;
    }
    fimc->hw_ver = vc.value;

    return 0;

err:
    if (0 < fimc->dev_fd)
        close(fimc->dev_fd);
    fimc->dev_fd =0;

    return -1;
}

int destroyFimc(s5p_fimc_t *fimc)
{
    if (fimc->out_buf.virt_addr != NULL) {
        fimc->out_buf.virt_addr = NULL;
        fimc->out_buf.length = 0;
    }

    // close
    if (0 < fimc->dev_fd)
        close(fimc->dev_fd);
    fimc->dev_fd = 0;

    return 0;
}

int runFimc(struct hwc_context_t *ctx,
            struct sec_img *src_img, struct sec_rect *src_rect,
            struct sec_img *dst_img, struct sec_rect *dst_rect,
            uint32_t transform)
{
    s5p_fimc_t *  fimc = &ctx->fimc;

    unsigned int src_phys_addr  = 0;
    unsigned int dst_phys_addr  = 0;
    int          rotate_value   = 0;
    int          flag_force_memcpy = 0;
    int32_t      src_color_space;
    int32_t      dst_color_space;

    // 1 : source address and size
    if (0 == (src_phys_addr = get_src_phys_addr(ctx, src_img, src_rect)))
        return -1;

    // 2 : destination address and size
    if (0 == (dst_phys_addr = get_dst_phys_addr(ctx, dst_img, dst_rect, &flag_force_memcpy)))
        return -2;

    // check whether fimc supports the src format
    if (0 > (src_color_space = HAL_PIXEL_FORMAT_2_V4L2_PIX(src_img->format)))
        return -3;

   if (0 > (dst_color_space = HAL_PIXEL_FORMAT_2_V4L2_PIX(dst_img->format)))
        return -4;

   if (runcFimcCore(ctx, src_phys_addr, src_img, src_rect, (uint32_t)src_color_space,
                         dst_phys_addr, dst_img, dst_rect, (uint32_t)dst_color_space,
                         transform) < 0)
        return -5;

    if (flag_force_memcpy == 1) {
#ifdef USE_HW_PMEM
        if (0 != ctx->sec_pmem.sec_pmem_alloc[1].size) {
            struct s3c_mem_dma_param s3c_mem_dma;

            s3c_mem_dma.src_addr = (unsigned long)(ctx->sec_pmem.sec_pmem_alloc[1].virt_addr);
            s3c_mem_dma.size     = ctx->sec_pmem.sec_pmem_alloc[1].size;
            ioctl(ctx->s3c_mem.fd, S3C_MEM_CACHE_INV, &s3c_mem_dma);

            memcpy((void*)((unsigned int)dst_img->base), (void *)(ctx->sec_pmem.sec_pmem_alloc[1].virt_addr), ctx->sec_pmem.sec_pmem_alloc[1].size);
        } else
#endif
        {
            struct s3c_mem_alloc *ptr_mem_alloc = &ctx->s3c_mem.mem_alloc[1];
            struct s3c_mem_dma_param s3c_mem_dma;

            s3c_mem_dma.src_addr = (unsigned long)ptr_mem_alloc->vir_addr;
            s3c_mem_dma.size     = ptr_mem_alloc->size;
            ioctl(ctx->s3c_mem.fd, S3C_MEM_CACHE_INV, &s3c_mem_dma);

            memcpy((void*)((unsigned int)dst_img->base), (void *)ptr_mem_alloc->vir_addr, ptr_mem_alloc->size);
        }
    }

    return 0;
}

#ifdef SUB_TITLES_HWC
static int get_g2d_src_phys_addr(struct hwc_context_t  *ctx, G2dRect *src_rect)
{
    struct s3c_mem_alloc *ptr_mem_alloc = &ctx->s3c_mem.mem_alloc[0];
#ifdef USE_HW_PMEM
    sec_pmem_alloc_t *pm_alloc = &ctx->sec_pmem.sec_pmem_alloc[0];
#endif

    unsigned int src_virt_addr  = 0;
    unsigned int src_phys_addr  = 0;
    unsigned int src_frame_size = 0;

    struct pmem_region region;

    // error check routine
    if (0 == src_rect->virt_addr) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s invalid src address", __func__);
        return 0;
    }

    src_frame_size = FRAME_SIZE(src_rect->color_format, src_rect->full_w, src_rect->full_h);
    if (src_frame_size == 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::FRAME_SIZE fail ", __func__);
        return 0;
    }

#ifdef USE_HW_PMEM
    if (0 <= checkPmem(&ctx->sec_pmem, 0, src_frame_size)) {
        src_virt_addr   = pm_alloc->virt_addr;
        src_phys_addr   = pm_alloc->phys_addr;
        pm_alloc->size  = src_frame_size;
    } else
#endif
    if (0 <= checkMem(&ctx->s3c_mem, 0, src_frame_size)) {
        src_virt_addr       = ptr_mem_alloc->vir_addr;
        src_phys_addr       = ptr_mem_alloc->phy_addr;
        ptr_mem_alloc->size = src_frame_size;
    } else {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::check_mem fail ", __func__);
        return 0;
    }
    memcpy((void *)src_virt_addr, (void*)((unsigned int)src_rect->virt_addr), src_frame_size);

    return src_phys_addr;
}

static inline int get_HAL_2_G2D_FORMAT(int format, int flag)
{
    int result = -1;
    if(flag){
        switch (format) {
        case HAL_PIXEL_FORMAT_RGBA_8888: result = SecG2d::COLOR_FORMAT_ABGR_8888;      break;
        case HAL_PIXEL_FORMAT_RGBX_8888: result = SecG2d::COLOR_FORMAT_XBGR_8888;      break;
        case HAL_PIXEL_FORMAT_BGRA_8888: result = SecG2d::COLOR_FORMAT_ARGB_8888;      break;
        case HAL_PIXEL_FORMAT_RGB_888:   result = SecG2d::COLOR_FORMAT_PACKED_BGR_888; break;
        case HAL_PIXEL_FORMAT_RGB_565:   result = SecG2d::COLOR_FORMAT_RGB_565;        break;
        case HAL_PIXEL_FORMAT_RGBA_5551: result = SecG2d::COLOR_FORMAT_RGBA_5551;      break;
        case HAL_PIXEL_FORMAT_RGBA_4444: result = SecG2d::COLOR_FORMAT_RGBA_4444;      break;
        default: break;
    }
    } else {
        switch (format){
        case HAL_PIXEL_FORMAT_RGBA_8888: result = SecG2d::COLOR_FORMAT_ARGB_8888;      break;
        case HAL_PIXEL_FORMAT_RGBX_8888: result = SecG2d::COLOR_FORMAT_XRGB_8888;      break;
        case HAL_PIXEL_FORMAT_BGRA_8888: result = SecG2d::COLOR_FORMAT_ABGR_8888;      break;
        case HAL_PIXEL_FORMAT_RGB_888:   result = SecG2d::COLOR_FORMAT_PACKED_RGB_888; break;
        case HAL_PIXEL_FORMAT_RGB_565:   result = SecG2d::COLOR_FORMAT_RGB_565;        break;
        case HAL_PIXEL_FORMAT_RGBA_5551: result = SecG2d::COLOR_FORMAT_ARGB_1555;      break;
        case HAL_PIXEL_FORMAT_RGBA_4444: result = SecG2d::COLOR_FORMAT_ARGB_4444;      break;
        default: break;
        }
   }
    return result;
}

static inline int rotateValueHAL2G2D(unsigned char transform)
{
    int result = SecG2d::ROTATE_0;
    int rotate_flag = transform & 0x7;

    switch (rotate_flag) {
    case HAL_TRANSFORM_ROT_90:  result = SecG2d::ROTATE_90;  break;
    case HAL_TRANSFORM_ROT_180: result = SecG2d::ROTATE_180; break;
    case HAL_TRANSFORM_ROT_270: result = SecG2d::ROTATE_270; break;
    default: break;
    }
    return result;
}

int runG2d(struct hwc_context_t *ctx,
            G2dRect *src_rect,
            G2dRect *dst_rect,
            uint32_t transform)
{
    G2dFlag flag = {SecG2d::ROTATE_0,
                    SecG2d::ALPHA_OPAQUE,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int          tmp   = 0;

    // 1 : source address and size
    if (0 == (src_rect->phys_addr = get_g2d_src_phys_addr(ctx, src_rect)))
        return -1;

    // 2 : destination address and size
    if (0 == dst_rect->phys_addr)
        return -2;

    // check whether g2d supports the src format
    if (0 > (src_rect->color_format = get_HAL_2_G2D_FORMAT(src_rect->color_format, 1)))
        return -3;

    if (0 > (dst_rect->color_format = get_HAL_2_G2D_FORMAT(dst_rect->color_format, 0)))
        return -4;

    flag.rotate_val = rotateValueHAL2G2D(transform);

    if((flag.rotate_val == SecG2d::ROTATE_90) ||
        (flag.rotate_val == SecG2d::ROTATE_270)){
            tmp = dst_rect->full_w;
            dst_rect->full_w = dst_rect->full_h;
            dst_rect->full_h = tmp;

            tmp = dst_rect->w;
            dst_rect->w = dst_rect->h;
            dst_rect->h = tmp;
        }

   // scale and rotate and alpha with FIMG
    if (stretchSecG2d(src_rect, dst_rect, NULL, &flag) < 0)
        return -5;

    return 0;
}
#endif

int createMem(struct s3c_mem_t *mem, unsigned int index, unsigned int size)
{
    #define S3C_MEM_DEV_NAME "/dev/s3c-mem"

    struct s3c_mem_alloc *ptr_mem_alloc;
    struct s3c_mem_alloc mem_alloc_info;

    if (index >= NUM_OF_MEM_OBJ) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::invalid index (%d >= %d)", __func__, index, NUM_OF_MEM_OBJ);
        goto err;
    }

    ptr_mem_alloc = &mem->mem_alloc[index];

    if (mem->fd <= 0) {
        mem->fd = open(S3C_MEM_DEV_NAME, O_RDWR);
        if (mem->fd <= 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::open(%s) fail(%s)", __func__, S3C_MEM_DEV_NAME, strerror(errno));
            goto err;
        }
    }

    if (0 == size)
        return 0;

    mem_alloc_info.size = size;

    if (ioctl(mem->fd, S3C_MEM_CACHEABLE_ALLOC, &mem_alloc_info) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::S3C_MEM_ALLOC(size : %d)  fail", __func__, mem_alloc_info.size);
        goto err;
    }

    ptr_mem_alloc->phy_addr = mem_alloc_info.phy_addr;
    ptr_mem_alloc->vir_addr = mem_alloc_info.vir_addr;
    ptr_mem_alloc->size     = mem_alloc_info.size;

    return 0;

err:
    if (0 < mem->fd)
        close(mem->fd);
    mem->fd = 0;

    return 0;
}

int destroyMem(struct s3c_mem_t *mem)
{
    int i;
    struct s3c_mem_alloc *ptr_mem_alloc;

    if (mem->fd <= 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::invalied fd(%d) fail", __func__, mem->fd);
        return -1;
    }

    for (i = 0; i < NUM_OF_MEM_OBJ; i++) {
        ptr_mem_alloc = &mem->mem_alloc[i];

        if (0 != ptr_mem_alloc->vir_addr) {
            if (ioctl(mem->fd, S3C_MEM_FREE, ptr_mem_alloc) < 0) {
                SEC_HWC_Log(HWC_LOG_ERROR, "%s::S3C_MEM_FREE fail", __func__);
                return -1;
            }

            ptr_mem_alloc->phy_addr = 0;
            ptr_mem_alloc->vir_addr = 0;
            ptr_mem_alloc->size     = 0;
        }
    }

    close(mem->fd);
    mem->fd = 0;

    return 0;
}

int checkMem(struct s3c_mem_t *mem, unsigned int index, unsigned int size)
{
    int ret;
    struct s3c_mem_alloc *ptr_mem_alloc;
    struct s3c_mem_alloc mem_alloc_info;

    if (index >= NUM_OF_MEM_OBJ) {
        SEC_HWC_Log(HWC_LOG_ERROR, "%s::invalid index (%d >= %d)", __func__, index, NUM_OF_MEM_OBJ);
        return -1;
    }

    if (mem->fd <= 0) {
        ret = createMem(mem, index, size);
        return ret;
    }

    ptr_mem_alloc = &mem->mem_alloc[index];

    if (ptr_mem_alloc->size < (int)size) {
        if (0 < ptr_mem_alloc->size) {
            // free allocated mem
            if (ioctl(mem->fd, S3C_MEM_FREE, ptr_mem_alloc) < 0) {
                SEC_HWC_Log(HWC_LOG_ERROR, "%s::S3C_MEM_FREE fail", __func__);
                return -1;
            }
        }

        // allocate mem with requested size
        mem_alloc_info.size = size;
        if (ioctl(mem->fd, S3C_MEM_CACHEABLE_ALLOC, &mem_alloc_info) < 0) {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::S3C_MEM_ALLOC(size : %d)  fail", __func__, mem_alloc_info.size);
            return -1;
        }

        ptr_mem_alloc->phy_addr = mem_alloc_info.phy_addr;
        ptr_mem_alloc->vir_addr = mem_alloc_info.vir_addr;
        ptr_mem_alloc->size     = mem_alloc_info.size;
    }

    return 0;
}

#ifdef USE_HW_PMEM
int createPmem(sec_pmem_t *pm, unsigned int buf_size)
{
    int    master_fd, err = 0, i;
    void  *base;
    unsigned int phys_base;
    size_t size, sub_size[NUM_OF_MEM_OBJ];
    struct pmem_region region;

#define PMEM_DEVICE_DEV_NAME "/dev/pmem_gpu1"

    master_fd = open(PMEM_DEVICE_DEV_NAME, O_RDWR, 0);
    if (master_fd < 0) {
        pm->pmem_master_fd = -1;
        if (EACCES == errno)
            return 0;
        else {
            SEC_HWC_Log(HWC_LOG_ERROR, "%s::open(%s) fail(%s)", __func__, PMEM_DEVICE_DEV_NAME, strerror(errno));
            return -errno;
        }
    }

    if (ioctl(master_fd, PMEM_GET_TOTAL_SIZE, &region) < 0)
    {
        SEC_HWC_Log(HWC_LOG_ERROR, "PMEM_GET_TOTAL_SIZE failed, default mode");
        size = 8<<20;   // 8 MiB
    } else
        size = region.len;

    base = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, master_fd, 0);
    if (base == MAP_FAILED) {
        SEC_HWC_Log(HWC_LOG_ERROR, "[%s] mmap failed : %d (%s)", __func__, errno, strerror(errno));
        base = 0;
        close(master_fd);
        master_fd = -1;
        return -errno;
    }

    if (ioctl(master_fd, PMEM_GET_PHYS, &region) < 0) {
        SEC_HWC_Log(HWC_LOG_ERROR, "PMEM_GET_PHYS failed, limp mode");
        region.offset = 0;
    }

    pm->pmem_master_fd   = master_fd;
    pm->pmem_master_base = base;
    pm->pmem_total_size  = size;
    //pm->pmem_master_phys_base = region.offset;
    phys_base = region.offset;

    // sec_pmem_alloc[0] for temporary buffer for source
    sub_size[0] = buf_size;
    sub_size[0] = roundUpToPageSize(sub_size[0]);

    for (i=0; i<NUM_OF_MEM_OBJ; i++) {
        sec_pmem_alloc_t *pm_alloc = &(pm->sec_pmem_alloc[i]);
        int fd, ret;
        int offset = i ? sub_size[i-1] : 0;
        struct pmem_region sub = { offset, sub_size[i] };

        // create the "sub-heap"
        if (0 > (fd = open(PMEM_DEVICE_DEV_NAME, O_RDWR, 0))) {
            SEC_HWC_Log(HWC_LOG_ERROR, "[%s][index=%d] open failed (%dL) : %d (%s)", __func__, i, __LINE__, errno, strerror(errno));
            return -errno;
        }

        // connect to it
        if (0 != (ret = ioctl(fd, PMEM_CONNECT, pm->pmem_master_fd))) {
            SEC_HWC_Log(HWC_LOG_ERROR, "[%s][index=%d] ioctl(PMEM_CONNECT) failed : %d (%s)", __func__, i, errno, strerror(errno));
            close(fd);
            return -errno;
        }

        // make it available to the client process
        if (0 != (ret = ioctl(fd, PMEM_MAP, &sub))) {
            SEC_HWC_Log(HWC_LOG_ERROR, "[%s][index=%d] ioctl(PMEM_MAP) failed : %d (%s)", __func__, i, errno, strerror(errno));
            close(fd);
            return -errno;
        }

        pm_alloc->fd         = fd;
        pm_alloc->total_size = sub_size[i];
        pm_alloc->offset     = offset;
        pm_alloc->virt_addr  = (unsigned int)base + (unsigned int)offset;
        pm_alloc->phys_addr  = (unsigned int)phys_base + (unsigned int)offset;

#if defined (PMEM_DEBUG)
        SEC_HWC_Log(HWC_LOG_DEBUG, "[%s] pm_alloc[%d] fd=%d total_size=%d offset=0x%x virt_addr=0x%x phys_addr=0x%x",
            __func__, i, pm_alloc->fd, pm_alloc->total_size,  pm_alloc->offset, pm_alloc->virt_addr, pm_alloc->phys_addr);
#endif
    }

    return err;
}

int destroyPmem(sec_pmem_t *pm)
{
    int i, err;

    for (i=0; i<NUM_OF_MEM_OBJ; i++) {
        sec_pmem_alloc_t *pm_alloc = &(pm->sec_pmem_alloc[i]);

        if (0 <= pm_alloc->fd) {
            struct pmem_region sub = { pm_alloc->offset, pm_alloc->total_size };

            if (0 > (err = ioctl(pm_alloc->fd, PMEM_UNMAP, &sub)))
                SEC_HWC_Log(HWC_LOG_ERROR, "[%s][index=%d] ioctl(PMEM_UNMAP) failed : %d (%s)", __func__, i, errno, strerror(errno));
#if defined (PMEM_DEBUG)
            else {
                SEC_HWC_Log(HWC_LOG_DEBUG, "[%s] pm_alloc[%d] unmap fd=%d total_size=%d offset=0x%x",
                    __func__, i, pm_alloc->fd, pm_alloc->total_size,  pm_alloc->offset);
            }
#endif

            close(pm_alloc->fd);

            pm_alloc->fd         = -1;
            pm_alloc->total_size = 0;
            pm_alloc->offset     = 0;
            pm_alloc->virt_addr  = 0;
            pm_alloc->phys_addr  = 0;
        }
    }

    if (0 <= pm->pmem_master_fd) {
        munmap(pm->pmem_master_base, pm->pmem_total_size);
        close(pm->pmem_master_fd);
        pm->pmem_master_fd = -1;
    }

    pm->pmem_master_base = 0;
    pm->pmem_total_size  = 0;

    return 0;
}

int checkPmem(sec_pmem_t *pm, unsigned int index, unsigned int requested_size)
{
    sec_pmem_alloc_t *pm_alloc = &(pm->sec_pmem_alloc[index]);

    if (0 < pm_alloc->virt_addr && requested_size <= (unsigned int)(pm_alloc->total_size))
        return 0;

    pm_alloc->size = 0;
    return -1;
}
#endif
