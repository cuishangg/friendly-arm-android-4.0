#include "JNIHelp.h"
#include "jni.h"
#include "utils/Log.h"
#include "utils/misc.h"

namespace android {
int register_com_samsung_sec_android_HdmiService(JNIEnv* env);
};

using namespace android;

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        LOGE("GetEnv failed!");
        return result;
    }
    LOG_ASSERT(env, "Could not retrieve the env!");

    if(register_com_samsung_sec_android_HdmiService(env) == -1)
        LOGE("%s register_com_samsung_sec_android_HdmiStatus is failed", __func__);

    return JNI_VERSION_1_4;
}
