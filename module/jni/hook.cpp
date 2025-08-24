#include "hook.h"
#include "logging.h"
#include <string>
#include <unordered_map>
#include <fstream> // Added for file operations

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

void hookBuild(JNIEnv *env) {
    LOGD("hook Build\n");
    jclass build_class = env->FindClass("android/os/Build");
    jstring new_xiaomi_str = env->NewStringUTF("Xiaomi");

    if (new_xiaomi_str == nullptr) {
        LOGE("Failed to create Xiaomi string");
        return;
    }

    jfieldID brand_id = env->GetStaticFieldID(build_class, "BRAND", "Ljava/lang/String;");
    if (brand_id != nullptr) {
        env->SetStaticObjectField(build_class, brand_id, new_xiaomi_str);
    }

    jfieldID manufacturer_id = env->GetStaticFieldID(build_class, "MANUFACTURER", "Ljava/lang/String;");
    if (manufacturer_id != nullptr) {
        env->SetStaticObjectField(build_class, manufacturer_id, new_xiaomi_str);
    }

    env->DeleteLocalRef(new_xiaomi_str);

    LOGD("hook Build done");
}

void hookSystemProperties(JNIEnv *env, zygisk::Api *api) {
    LOGD("hook SystemProperties\n");

    JNINativeMethod targetHookMethods[] = {
            {"native_get", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
             (void *) my_native_get},
    };

    api->hookJniNativeMethods(env, "android/os/SystemProperties", targetHookMethods, 1);

    *(void **) &orig_native_get = targetHookMethods[0].fnPtr;

    LOGD("hook SystemProperties done: %p\n", orig_native_get);
}

void Hook::hook() {
    hookBuild(env);
    hookSystemProperties(env, api);
}
