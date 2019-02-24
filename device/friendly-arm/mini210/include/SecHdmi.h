/*
 * Copyright@ Samsung Electronics Co. LTD
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
**
** @author Taikyung, Yu(taikyung.yu@samsung.com)
** @date   2010-09-10
**
*/


#ifndef __SEC_HDMI_H__
#define __SEC_HDMI_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

#include <linux/fb.h>
#include <hardware/hardware.h>
#include <utils/threads.h>

#include "sec_format.h"
#include "sec_utils.h"
#include "s3c_lcd.h"
#include "SecFimc.h"
#if defined(S5P_BOARD_USES_HDMI_SUBTITLES)
#include "SecG2d.h"
#endif
#include "libedid.h"
#include "libcec.h"

namespace android {

//#define DEBUG_HDMI_HW_LEVEL

#define HDMI_FIMC_OUTPUT_BUF_NUM 3

#define TVOUT_DEV       "/dev/video14"
#define TVOUT_DEV_G0    "/dev/video21"
#define TVOUT_DEV_G1    "/dev/video22"

#define ALIGN(x, a)    (((x) + (a) - 1) & ~((a) - 1))

#if defined(STD_1080P)
    #define DEFAULT_HDMI_RESOLUTION_VALUE (1080930) // 1080P_30
    #define DEFAULT_HDMI_STD_ID           (V4L2_STD_1080P_30)
#elif defined(STD_720P)
    #define DEFAULT_HDMI_RESOLUTION_VALUE (720960) // 720P_60
    #define DEFAULT_HDMI_STD_ID           (V4L2_STD_720P_60)
#elif defined(STD_480P)
    #define DEFAULT_HDMI_RESOLUTION_VALUE (4809601) // 480P_60_4_3
    #define DEFAULT_HDMI_STD_ID           (V4L2_STD_480P_60_16_9)
#else
    #define DEFAULT_HDMI_RESOLUTION_VALUE (4809602) // 480P_60_4_3
    #define DEFAULT_HDMI_STD_ID           (V4L2_STD_480P_60_4_3)
#endif

// this is for testing... (very basic setting.)
//#define DEFAULT_HDMI_RESOLUTION_VALUE (4809602) // 480P_60_4_3
//#define DEFAULT_HDMI_STD_ID           (V4L2_STD_480P_60_4_3)

enum state {
	OFF = 0,
	ON = 1,
	NOT_SUPPORT = 2,
};

// video processor
typedef enum {
	VPROC_SRC_COLOR_NV12  = 0,
	VPROC_SRC_COLOR_NV12IW  = 1,
	VPROC_SRC_COLOR_TILE_NV12  = 2,
	VPROC_SRC_COLOR_TILE_NV12IW  = 3
}s5p_vp_src_color;

enum tv_mode{
    HDMI_OUTPUT_MODE_YCBCR = 0,
    HDMI_OUTPUT_MODE_RGB = 1,
    HDMI_OUTPUT_MODE_DVI = 2
};

enum hdmi_layer
{
    HDMI_LAYER_BASE   = 0,
    HDMI_LAYER_VIDEO,
    HDMI_LAYER_GRAPHIC_0,
    HDMI_LAYER_GRAPHIC_1,
    HDMI_LAYER_MAX,
};

struct v4l2_s5p_video_param
{
	unsigned short b_win_blending;
	unsigned int ui_alpha;
	unsigned int ui_priority;
	unsigned int ui_top_y_address;
	unsigned int ui_top_c_address;
	struct v4l2_rect rect_src;
	unsigned short src_img_endian;
};

struct tvout_param {
	struct v4l2_pix_format_s5p_tvout    tvout_src;
	struct v4l2_window_s5p_tvout        tvout_rect;
	struct v4l2_rect            tvout_dst;
};

struct overlay_param {
	struct v4l2_framebuffer 		overlay_frame;
	struct v4l2_window_s5p_tvout 	overlay_rect;
	struct v4l2_rect				overlay_dst;
};



int tvout_v4l2_querycap(int fp);
int tvout_v4l2_g_std(int fp, v4l2_std_id *std_id);
int tvout_v4l2_s_std(int fp, v4l2_std_id std_id);
int tvout_v4l2_enum_std(int fp, struct v4l2_standard *std, v4l2_std_id std_id);
int tvout_v4l2_enum_output(int fp, struct v4l2_output *output);
int tvout_v4l2_s_output(int fp, int index);
int tvout_v4l2_g_output(int fp, int *index);
int tvout_v4l2_enum_fmt(int fp, struct v4l2_fmtdesc *desc);
int tvout_v4l2_g_fmt(int fp, int buf_type, void* ptr);
int tvout_v4l2_s_fmt(int fp, int buf_type, void *ptr);
int tvout_v4l2_g_parm(int fp, int buf_type, void *ptr);
int tvout_v4l2_s_parm(int fp, int buf_type, void *ptr);
int tvout_v4l2_g_fbuf(int fp, struct v4l2_framebuffer *frame);
int tvout_v4l2_s_fbuf(int fp, struct v4l2_framebuffer *frame);
int tvout_v4l2_g_crop(int fp, unsigned int type, struct v4l2_rect *rect);
int tvout_v4l2_s_crop(int fp, unsigned int type, struct v4l2_rect *rect);
int tvout_v4l2_streamon(int fp, unsigned int type);
int tvout_v4l2_streamoff(int fp, unsigned int type);
int tvout_v4l2_start_overlay(int fp);
int tvout_v4l2_stop_overlay(int fp);
int tvout_v4l2_cropcap(int fp, struct v4l2_cropcap *a);

class SecHdmi
{
public :
    enum HDMI_LAYER
    {
        HDMI_LAYER_BASE   = 0,
        HDMI_LAYER_VIDEO,
        HDMI_LAYER_GRAPHIC_0,
        HDMI_LAYER_GRAPHIC_1,
        HDMI_LAYER_MAX,
    };

private:
    class HdmiThread : public Thread {
        SecHdmi * mHardware;

    public:
        HdmiThread(SecHdmi *hw):
#ifdef SINGLE_PROCESS
        // In single process mode this thread needs to be a java thread,
        // since we won't be calling through the binder.
        Thread(true),
#else
        Thread(false),
#endif
        mHardware(hw) { }

        virtual void onFirstRef() {
            run("HdmiThread", PRIORITY_NORMAL);
        }
        virtual bool threadLoop() {
            return mHardware->runCEC();
        }
    };


private :
    bool         mFlagCreate;
    bool         mFirstRef;

    Mutex        mLock;

    bool         mFlagConnected;
    bool         mFlagHdmiStart[HDMI_LAYER_MAX];

    int          mSrcWidth[HDMI_LAYER_MAX];
    int          mSrcHeight[HDMI_LAYER_MAX];
    int          mSrcColorFormat[HDMI_LAYER_MAX];
    int          mHdmiSrcWidth[HDMI_LAYER_MAX];
    int          mHdmiSrcHeight[HDMI_LAYER_MAX];

    int          mHdmiDstWidth;
    int          mHdmiDstHeight;
    unsigned int mHdmiSrcYAddr;
    unsigned int mHdmiSrcCbCrAddr;

    int          mHdmiOutputMode;
    unsigned int mHdmiResolutionValue;
    v4l2_std_id  mHdmiStdId;
    bool         mHdcpMode;
    int          mAudioMode;

    int          mCurrentHdmiOutputMode;
    unsigned int mCurrentHdmiResolutionValue;
    bool         mCurrentHdcpMode;
    int          mCurrentAudioMode;
    bool         mHdmiInfoChange;

    bool         mFlagFimcStart;
    int          mFimcDstColorFormat;
    unsigned int mFimcReservedMem[HDMI_FIMC_OUTPUT_BUF_NUM];
    unsigned int mFimcCurrentOutBufIndex;
    SecFimc      mSecFimc;

    unsigned int mHdmiResolutionValueList[11];
    int          mHdmiSizeOfResolutionValueList;

    int          mLCDFd;
    int          mLCDWidth;
    int          mLCDHeight;

    tvout_param mTvoutParam;


    sp<HdmiThread> mHdmiThread;
    bool           mFlagHdmiThreadRun;
    mutable Mutex  mHdmiThreadLock;

    unsigned int   mV4LOutputType;

    enum CECDeviceType mCecDeviceType;
    unsigned char mCecBuffer[CEC_MAX_FRAME_SIZE];
    int mCecLAddr;
    int mCecPAddr;

    int mFdTvout;
    int mFdTvoutG0;
    int mFdTvoutG1;

    unsigned int cur_phy_g2d_addr;
public :

    SecHdmi();
    virtual ~SecHdmi();
    bool create(void);
    bool destroy(void);
    inline bool flagCreate(void) { return mFlagCreate; }

    bool connect(void);

    bool disconnect(void);

    bool flagConnected(void);

    bool flush(int w, int h, int colorFormat,
               int hdmiLayer,
               unsigned int physYAddr, unsigned int physCbAddr,
               int num_of_hwc_layer);

    bool clear(int hdmiLayer);

    bool setHdmiOutputMode(int hdmiOutputMode, bool forceRun = false);

    bool setHdmiResolution(unsigned int hdmiResolutionValue, bool forceRun = true);

    bool setHdcpMode(bool hdcpMode, bool forceRun = false);

private:

    bool m_reset(int w, int h, int colorFormat, int hdmiLayer);

    bool m_startHdmi(int hdmiLayer);
    bool m_stopHdmi(int hdmiLayer);

    bool m_setHdmiOutputMode(int hdmiOutputMode);
    bool m_setHdmiResolution(unsigned int hdmiResolutionValue);
    bool m_setHdcpMode(bool hdcpMode);
    bool m_setAudioMode(int audioMode);

    int  m_resolutionValueIndex(unsigned int ResolutionValue);

    bool m_flagHWConnected(void);

    int hdmi_initialize(int layer, v4l2_std_id std_id);
    int hdmi_deinitialize(int layer);
    int hdmi_set_v_param(int layer,
                         int src_w, int src_h, int dst_w, int dst_h,
                         unsigned int ui_top_y_address, unsigned int ui_top_c_address);
    int hdmi_gl_set_param(int layer,
                          int src_w, int src_h, int dst_w, int dst_h,
                          unsigned int ui_base_address, int num_of_hwc_layer);
    int hdmi_streamon  (int layer);
    int hdmi_streamoff(int layer);

    int hdmi_cable_status();
    int hdmi_set_mode(unsigned int mode);
    int hdmi_outputmode_2_v4l2_output_type(int output_mode);
    int hdmi_v4l2_output_type_2_outputmode(int output_mode);
    int hdmi_check_output_mode(int v4l2_output_type);

    int hdmi_set_resolution(v4l2_std_id std_id);
    int hdmi_check_resolution(v4l2_std_id std_id);
    double hdmi_resolution_2_zoom_factor(unsigned int resolution);
    int hdmi_resolution_2_std_id(unsigned int resolution, int * w, int * h, v4l2_std_id * std_id);
    int hdmi_set_overlay_param(int layer, void *param);

    int hdmi_enable_hdcp(unsigned int hdcp_en);

    int hdmi_check_audio(void);

    int hdmi_tvout_init(v4l2_std_id std_id);

    void hdmi_show_info(void);

    bool m_runEdid(bool flagInsert);
    bool initCEC(void);
    bool deinitCEC(void);
    bool runCEC(void);

};

}; // namespace android

#endif //__SEC_HDMI_H__


