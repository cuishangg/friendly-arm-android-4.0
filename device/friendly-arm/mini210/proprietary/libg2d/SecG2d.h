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
 * Sangwoo, Park(sw5771.park@samsung.com)
 * Date    : 02 May 2010
 * Purpose : Initial version
 */

#ifndef __SEC_G2D_H__
#define __SEC_G2D_H__

#include <utils/Log.h>
#include "sec_g2d.h"

struct G2dRect {
    int x;
    int y;
    int w;
    int h;
    int full_w;
    int full_h;
    int color_format;
    unsigned int phys_addr;
    char        *virt_addr;
};

struct G2dFlag {
    int rotate_val;
    int alpha_val;

    int blue_screen_mode;     //true : enable, false : disable
    int color_key_val;        //screen color value
    int color_switch_val;     //one color

    int src_color;            // when set one color on SRC
    
    int third_op_mode;
    int rop_mode;
    int mask_mode;
    int render_mode;    
    int potterduff_mode;
    int memory_type;
};

struct G2dClip {
    int t;
    int b;
    int l;
    int r;
};

#ifdef __cplusplus
class SecG2d
{
public:
#endif
    enum COLOR_FORMAT {
        COLOR_FORMAT_BASE = 0,

        COLOR_FORMAT_RGB_565,

        COLOR_FORMAT_RGBA_8888,
        COLOR_FORMAT_ARGB_8888,
        COLOR_FORMAT_BGRA_8888,
        COLOR_FORMAT_ABGR_8888,

        COLOR_FORMAT_RGBX_8888,
        COLOR_FORMAT_XRGB_8888,
        COLOR_FORMAT_BGRX_8888,
        COLOR_FORMAT_XBGR_8888,

        COLOR_FORMAT_RGBA_5551,
        COLOR_FORMAT_ARGB_1555,
        COLOR_FORMAT_BGRA_5551,
        COLOR_FORMAT_ABGR_1555,

        COLOR_FORMAT_RGBX_5551,
        COLOR_FORMAT_XRGB_1555,
        COLOR_FORMAT_BGRX_5551,
        COLOR_FORMAT_XBGR_1555,

        COLOR_FORMAT_RGBA_4444,
        COLOR_FORMAT_ARGB_4444,
        COLOR_FORMAT_BGRA_4444,
        COLOR_FORMAT_ABGR_4444,

        COLOR_FORMAT_RGBX_4444,
        COLOR_FORMAT_XRGB_4444,
        COLOR_FORMAT_BGRX_4444,
        COLOR_FORMAT_XBGR_4444,

        COLOR_FORMAT_PACKED_RGB_888,
        COLOR_FORMAT_PACKED_BGR_888,

        COLOR_FORMAT_YUV_420SP,
        COLOR_FORMAT_YUV_420P,
        COLOR_FORMAT_YUV_420I,
        COLOR_FORMAT_YUV_422SP,	
        COLOR_FORMAT_YUV_422P,
        COLOR_FORMAT_YUV_422I,
        COLOR_FORMAT_YUYV,	

        COLOR_FORMAT_MAX,
    };

    enum ROTATE {
        ROTATE_BASE = 0,
        ROTATE_0,
        ROTATE_90,
        ROTATE_180,
        ROTATE_270,
        ROTATE_X_FLIP,
        ROTATE_Y_FLIP,
        ROTATE_MAX,
    };   

    enum ALPHA {
        ALPHA_MIN    = 0, // wholly transparent
        ALPHA_MAX    = 255,
        ALPHA_OPAQUE = 256,
    };

    enum DITHER {
        DITHER_BASE   = 0,
        DITHER_OFF    = 0,
        DITHER_ON     = 1,
        DITHER_MAX,
    };
    enum PORTTERDUFF {
        PORTTERDUFF_Src = 0,      //!< [Sa, Sc]
        PORTTERDUFF_Dst,      //!< [Da, Dc]
        PORTTERDUFF_Clear,    //!< [0, 0]
        PORTTERDUFF_SrcOver,  //!< [Sa + Da - Sa*Da, Rc = Sc + (1 - Sa)*Dc]
        PORTTERDUFF_DstOver,  //!< [Sa + Da - Sa*Da, Rc = Dc + (1 - Da)*Sc]
        PORTTERDUFF_SrcIn,    //!< [Sa * Da, Sc * Da]
        PORTTERDUFF_DstIn,    //!< [Sa * Da, Sa * Dc]
        PORTTERDUFF_SrcOut,   //!< [Sa * (1 - Da), Sc * (1 - Da)]
        PORTTERDUFF_DstOut,   //!< [Da * (1 - Sa), Dc * (1 - Sa)]
        PORTTERDUFF_SrcATop,  //!< [Da, Sc * Da + (1 - Sa) * Dc]
        PORTTERDUFF_DstATop,  //!< [Sa, Sa * Dc + Sc * (1 - Da)]
        PORTTERDUFF_Xor,      //!< [Sa + Da - 2 * Sa * Da, Sc * (1 - Da) + (1 - Sa) * Dc]
        // these modes are defined in the SVG Compositing standard
        // http://www.w3.org/TR/2009/WD-SVGCompositing-20090430/
        PORTTERDUFF_Plus,
        PORTTERDUFF_Multiply,
        PORTTERDUFF_Screen,
        PORTTERDUFF_Overlay,
        PORTTERDUFF_Darken,
        PORTTERDUFF_Lighten,
        PORTTERDUFF_ColorDodge,
        PORTTERDUFF_ColorBurn,
        PORTTERDUFF_HardLight,
        PORTTERDUFF_SoftLight,
        PORTTERDUFF_Difference,
        PORTTERDUFF_Exclusion,
        PORTTERDUFF_kLastMode = PORTTERDUFF_Exclusion
    };

    enum RENDER_MODE {
        RENDER_MODE_CACHE_OFF    = 0,
        RENDER_MODE_CACHE_ON     = (1 >> 1),
        RENDER_MODE_NON_BLOCKING = (1 >> 2),
    };

#ifdef __cplusplus
private :
    bool    m_flagCreate;
 
protected :
    SecG2d();
    SecG2d(const SecG2d& rhs) {}
    virtual ~SecG2d();

public:
    bool        create(void);
    bool        destroy(void);
    inline bool flagCreate(void) { return m_flagCreate; }
    bool        stretch(G2dRect *src, G2dRect *dst, G2dClip *clip, G2dFlag *flag);
    bool	    sync(void);

protected:
    virtual bool t_create(void);
    virtual bool t_destroy(void);
    virtual bool t_stretch(G2dRect *src, G2dRect *dst, G2dClip *clip, G2dFlag *flag);
    virtual bool t_sync(void);
    virtual bool t_lock(void);
    virtual bool t_unLock(void);

    unsigned int t_frameSize(G2dRect * rect);
    bool         t_copyFrame(G2dRect * src, G2dRect * dst);
};
#endif

//---------------------------------------------------------------------------//
// user api extern function 
//---------------------------------------------------------------------------//
// usage 1
// SecG2d * p = createG2d();
// p->stretch()
// destroyG2d(p);
//
// usage 2
// stretchG2d(src, dst, clip, flag);
//---------------------------------------------------------------------------//
#ifdef __cplusplus
extern "C" 
#endif
struct SecG2d* createSecG2d();

#ifdef __cplusplus
extern "C"
#endif
void           destroySecG2d(SecG2d *g2d);

#ifdef __cplusplus
extern "C"
#endif
int            stretchSecG2d(G2dRect *src,
                             G2dRect *dst,
                             G2dClip *clip,
                             G2dFlag *flag);
#ifdef __cplusplus
extern "C"
#endif
int            syncSecG2d(void);

#endif //__SEC_G2D_H__
