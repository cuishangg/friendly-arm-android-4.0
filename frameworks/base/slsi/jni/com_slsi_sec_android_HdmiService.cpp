#define LOG_TAG "HDMIStatusService"

#include "jni.h"
#include "JNIHelp.h"
#include <cutils/log.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#if defined(S5P_BOARD_USES_HDMI)
#include "ISecTVOut.h"
#endif

#define INFO(...) \
    do { \
        printf(__VA_ARGS__); \
        printf("\n"); \
        LOGD(__VA_ARGS__); \
    } while(0)


void assert_fail(const char *file, int line, const char *func, const char *expr) {
    INFO("assertion failed at file %s, line %d, function %s:",
        file, line, func);
    INFO("%s", expr);
    abort();
}

#define ASSERT(e) \
    do { \
        if (!(e)) \
        assert_fail(__FILE__, __LINE__, __func__, #e); \
    } while(0)


namespace android {

#define GETSERVICETIMEOUT (5)
static sp<ISecTVOut> g_SecTVOutService = 0;
sp<ISecTVOut> m_getSecTVOutService(void)
{
    int ret = 0;

    if (g_SecTVOutService == 0) {
        sp<IBinder> binder;
        sp<ISecTVOut> sc;
        sp<IServiceManager> sm = defaultServiceManager();
        int getSvcTimes = 0;
        for(getSvcTimes = 0; getSvcTimes < GETSERVICETIMEOUT; getSvcTimes++) {
            binder = sm->getService(String16("SecTVOutService"));
            if (binder == 0) {
                LOGW("SecTVOutService not published, waiting...");
                usleep(500000); // 0.5 s
            } else {
                break;
            }
        }
        // grab the lock again for updating g_surfaceFlinger
        if (getSvcTimes < GETSERVICETIMEOUT) {
            sc = interface_cast<ISecTVOut>(binder);
            g_SecTVOutService = sc;

        } else {
            LOGW("Failed to get SecTVOutService... SecHdmiClient will get it later..");
        }
    }
    return g_SecTVOutService;
}

/*
 * Class:     com_slsi_sec_android_HdmiService
 * Method:    setHdmiCableStatus
 * Signature: (I)V
 */
static void com_slsi_sec_android_HdmiService_setHdmiCableStatus
  (JNIEnv *env, jobject obj, jint i)
{
    int result = 0;

#if defined(S5P_BOARD_USES_HDMI)
    //LOGD("%s HDMI status: %d", __func__, i);
    if (g_SecTVOutService != 0) 
        g_SecTVOutService->setHdmiCableStatus(i);
#else
    return;
#endif

    //return result;
}

/*
 * Class:     com_slsi_sec_android_setHdmiMode
 * Method:    setHdmiMode
 * Signature: (I)V
 */
static void com_slsi_sec_android_HdmiService_setHdmiMode
  (JNIEnv *env, jobject obj, jint i)
{
    int result = 0;

#if defined(S5P_BOARD_USES_HDMI)
    //LOGD("%s HDMI mode: %d", __func__, i);
    if (g_SecTVOutService != 0)
        g_SecTVOutService->setHdmiMode(i);
#else
    return;
#endif
}

/*
 * Class:     com_slsi_sec_android_setHdmiResolution
 * Method:    setHdmiResolution
 * Signature: (I)V
 */
static void com_slsi_sec_android_HdmiService_setHdmiResolution
(JNIEnv *env, jobject obj, jint i)
{
    int result = 0;

#if defined(S5P_BOARD_USES_HDMI)
    //LOGD("%s HDMI Resolution: %d", __func__, i);
    if (g_SecTVOutService != 0)
        g_SecTVOutService->setHdmiResolution(i);
#else
    return;
#endif
}

/*
 * Class:     com_slsi_sec_android_setHdmiHdcp
 * Method:    setHdmiHdcp
 * Signature: (I)V
 */
static void com_slsi_sec_android_HdmiService_setHdmiHdcp
(JNIEnv *env, jobject obj, jint i)
{
    int result = 0;

#if defined(S5P_BOARD_USES_HDMI)
    //LOGD("%s HDMI Hdcp: %d", __func__, i);
    if (g_SecTVOutService != 0)
        g_SecTVOutService->setHdmiHdcp(i);
#else
    return;
#endif
}

/*
 * Class:     com_slsi_sec_android_HdmiService
 * Method:    initHdmiService
 * Signature: ()V
 */
static void com_slsi_sec_android_HdmiService_initHdmiService
  (JNIEnv *env, jobject obj)
{
#if defined(S5P_BOARD_USES_HDMI)
    LOGI("%s ", __func__);
    g_SecTVOutService = m_getSecTVOutService();
#else
    return;
#endif
    //return result;
}

static JNINativeMethod gMethods[] = {
    {"setHdmiCableStatus", "(I)V", (void*)com_slsi_sec_android_HdmiService_setHdmiCableStatus},
    {"setHdmiMode", "(I)V", (void*)com_slsi_sec_android_HdmiService_setHdmiMode},
    {"setHdmiResolution", "(I)V", (void*)com_slsi_sec_android_HdmiService_setHdmiResolution},
    {"setHdmiHdcp", "(I)V", (void*)com_slsi_sec_android_HdmiService_setHdmiHdcp},
    {"initHdmiService", "()V", (void*)com_slsi_sec_android_HdmiService_initHdmiService},
};

int register_com_samsung_sec_android_HdmiService(JNIEnv* env)
{
     jclass clazz = env->FindClass("com/slsi/sec/android/HdmiService");

     if (clazz == NULL)
     {
         LOGE("Can't find com/slsi/sec/android/HdmiService");
         return -1;
     }

     return jniRegisterNativeMethods(env, "com/slsi/sec/android/HdmiService",
                                     gMethods, NELEM(gMethods));
}

} /* namespace android */

