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

#ifndef __SEC_G2D_C110_H__
#define __SEC_G2D_C110_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>

#include <linux/android_pmem.h>
#include <utils/threads.h>
#include <utils/StopWatch.h>

#include "SecG2d.h"
#include "sec_g2d.h"

namespace android
{

//#define CHECK_G2D_C110_PERFORMANCE
//#define CHECK_G2D_C110_CRITICAL_PERFORMANCE
#define NUMBER_G2D_LIST           (1)  // kcoolsw : because of pmem
#define G2D_NONE_BLOCKING_MODE

class SecG2dC110 : public SecG2d
{
private :
    int            m_fd;
    char          *m_g2dVirtAddr;
    unsigned int   m_g2dPhysAddr;
    unsigned int   m_g2dSize;
    char          *m_srcVirtAddr;
    unsigned int   m_srcPhysAddr;
    unsigned int   m_srcSize;
    char          *m_dstVirtAddr;
    unsigned int   m_dstPhysAddr;
    unsigned int   m_dstSize;
    struct pollfd  m_poll;

    Mutex         *m_lock;

    static Mutex   m_instanceLock;
    static int     m_curSecG2dC110Index;
    static int     m_numOfInstance;

    static SecG2d *m_ptrG2dList[NUMBER_G2D_LIST];

protected :
    SecG2dC110();
    virtual ~SecG2dC110();

public:
    static SecG2d *createInstance(void);
    static void    destroyInstance(SecG2d *g2d);
    static void    destroyAllInstance(void);

protected:
    virtual bool   t_create(void);
    virtual bool   t_destroy(void);
    virtual bool   t_stretch(G2dRect *src, G2dRect *dst, G2dClip *clip, G2dFlag *flag);
    virtual bool   t_lock(void);
    virtual bool   t_unLock(void);

private:
    bool           m_createG2d(void);
    bool           m_destroyG2d(void);
    bool           m_doG2d(G2dRect *src, G2dRect *dst, G2dFlag *flag);
    inline bool    m_pollG2d(struct pollfd *events);
    inline bool    m_cleanG2d(unsigned int physAddr, unsigned int size);
    inline bool    m_invalidG2d(unsigned int physAddr, unsigned int size);
    inline bool    m_flushG2d(unsigned int physAddr, unsigned int size);
    inline int     m_colorFormatSecG2d2G2dHw(int colorFormat);
    inline int     m_rotateValueSecG2d2G2dHw(int rotateValue);
    bool           m_getCalibratedNum(unsigned int *num1, unsigned int *num2);

#ifdef CHECK_G2D_C110_PERFORMANCE
    void           m_printSecG2dC110Performance(G2dRect    *src,
                                                G2dRect    *dst,
                                                int         stopWatchIndex,
                                                const char *stopWatchName[],
                                                nsecs_t     stopWatchTime[]);
#endif // CHECK_G2D_C110_PERFORMANCE
};

class SecG2dAutoFreeThread;

static sp<SecG2dAutoFreeThread> secG2dAutoFreeThread = 0;

class SecG2dAutoFreeThread : public Thread
{
    private:
        bool m_oneMoreSleep;
        bool m_destroyed;

    public:
        SecG2dAutoFreeThread(void):
                    //Thread(true),
                    Thread(false),
                    m_oneMoreSleep(true),
                    m_destroyed(false)
                    { }
        ~SecG2dAutoFreeThread(void)
        {
            if (m_destroyed == false) {
                SecG2dC110::destroyAllInstance();
                m_destroyed = true;
            }
        }

        virtual void onFirstRef()
        {
            run("SecG2dAutoFreeThread", PRIORITY_BACKGROUND);
        }

        virtual bool threadLoop()
        {
            //#define SLEEP_TIME (10000000) // 10 sec
            #define SLEEP_TIME (3000000) // 3 sec
            //#define SLEEP_TIME (1000000) // 1 sec

            if (m_oneMoreSleep == true) {
                m_oneMoreSleep = false;
                usleep(SLEEP_TIME);
                return true;
            } else {
                if (m_destroyed == false) {
                    SecG2dC110::destroyAllInstance();
                    m_destroyed = true;
                }
                secG2dAutoFreeThread = 0;
                return false;
            }
        }

        void setOneMoreSleep(void)
        {
            m_oneMoreSleep = true;
        }
};

}; // namespace android

#endif // __SEC_G2D_C110_H__
