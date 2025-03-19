#include "hook.h"
#include "logging.h"
#include <string>
#include <unordered_map>

using namespace std;

jstring (*orig_native_get)(JNIEnv *env, jclass clazz, jstring keyJ, jstring defJ);

static unordered_map<string, string> spoofedProperties = {
    {"ro.product.brand", "Xiaomi"},
    {"ro.product.manufacturer", "Xiaomi"},
    {"ro.miui.ui.version.name", "V130"},
    {"ro.miui.ui.version.code", "13"},
    {"ro.miui.version.code_time", "1658851200"},
    {"ro.miui.internal.storage", "/sdcard/"},
    {"ro.miui.region", "CN"},
    {"ro.miui.cust_variant", "cn"},
    {"ro.vendor.miui.region", "CN"}
};

jstring my_native_get(JNIEnv *env, jclass clazz, jstring keyJ, jstring defJ) {
    const char *keyChars = env->GetStringUTFChars(keyJ, nullptr);
    string keyStr(keyChars);
    env->ReleaseStringUTFChars(keyJ, keyChars);
    auto it = spoofedProperties.find(keyStr);
    if (it != spoofedProperties.end()) {
        return env->NewStringUTF(it->second.c_str());
    }
    return orig_native_get(env, clazz, keyJ, defJ);
}
