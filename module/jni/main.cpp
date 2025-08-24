#include <cstdlib>
#include <unistd.h>
#include <string>
#include <vector>
#include <fcntl.h>

#include "zygisk.hpp"
#include "logging.h"
#include "hook.h"
#include "util.h"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

using namespace std;

// 核心组件的“指纹”
// 这些是在 AndroidManifest.xml 中声明的，不会被混淆
const vector<const char*> MIPUSH_COMPONENTS = {
    "com.xiaomi.push.service.XMPushService",
    "com.xiaomi.push.service.XMJobService",
    "com.xiaomi.mipush.sdk.PushMessageHandler",
    "com.xiaomi.mipush.sdk.MessageHandleService",
};

class MiPushZygisk : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
        LOGD("MiPushZygisk module loaded successfully.");
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        string process_name = jstringToStdString(env, args->nice_name);
        if (!isPotentialTargetProcess(process_name)) {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        string app_data_dir = jstringToStdString(env, args->app_data_dir);
        if (app_data_dir.empty()) {
            return;
        }

        string package_name = parsePackageName(app_data_dir.c_str());
        if (package_name.empty()) {
            LOGW("Could not parse package name from app_data_dir: %s", app_data_dir.c_str());
            return;
        }
        
        // 最终方案：通过 PackageManager 检查静态的 Manifest 组件
        if (hasMiPushComponents(package_name.c_str())) {
            string process_name = jstringToStdString(env, args->nice_name);
            LOGI("MiPush components found in Manifest for package [%s]. Applying device hooks to process [%s].",
                 package_name.c_str(), process_name.c_str());
            Hook(api, env).hook();
        } else {
             // LOGD("Package [%s] does not contain MiPush components in its Manifest.", package_name.c_str());
        }
    }

    void preServerSpecialize(ServerSpecializeArgs *args) override {
        api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
    }

private:
    Api *api;
    JEnv *env;

    // 新的检查方法：检查应用的 Manifest 是否声明了 MiPush 的核心组件
    bool hasMiPushComponents(const char* package_name) {
        // JNI 调用 Android API 来获取 PackageManager 和 PackageInfo
        jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
        if (!activityThreadClass) {
            env->ExceptionClear();
            return false;
        }

        jmethodID currentApplicationMethod = env->GetStaticMethodID(activityThreadClass, "currentApplication", "()Landroid/app/Application;");
        if (!currentApplicationMethod) {
            env->ExceptionClear();
            return false;
        }

        jobject application = env->CallStaticObjectMethod(activityThreadClass, currentApplicationMethod);
        if (!application) {
            env->ExceptionClear();
            return false;
        }

        jclass contextClass = env->GetObjectClass(application);
        jmethodID getPackageManagerMethod = env->GetMethodID(contextClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
        jobject packageManager = env->CallObjectMethod(application, getPackageManagerMethod);

        jstring packageNameJString = env->NewStringUTF(package_name);
        jclass packageManagerClass = env->GetObjectClass(packageManager);
        
        // 我们需要检查 Services
        jmethodID getPackageInfoMethod = env->GetMethodID(packageManagerClass, "getPackageInfo", "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
        // PackageManager.GET_SERVICES = 4
        jobject packageInfo = env->CallObjectMethod(packageManager, getPackageInfoMethod, packageNameJString, 4);

        env->DeleteLocalRef(packageNameJString);
        if (env->ExceptionCheck() || !packageInfo) {
            env->ExceptionClear();
            return false;
        }

        jclass packageInfoClass = env->GetObjectClass(packageInfo);
        jfieldID servicesField = env->GetFieldID(packageInfoClass, "services", "[Landroid/content/pm/ServiceInfo;");
        jobjectArray services = (jobjectArray) env->GetObjectField(packageInfo, servicesField);

        bool found = false;
        if (services) {
            jsize serviceCount = env->GetArrayLength(services);
            for (int i = 0; i < serviceCount; ++i) {
                jobject serviceInfo = env->GetObjectArrayElement(services, i);
                jclass serviceInfoClass = env->GetObjectClass(serviceInfo);
                jfieldID nameField = env->GetFieldID(serviceInfoClass, "name", "Ljava/lang/String;");
                jstring serviceNameJString = (jstring) env->GetObjectField(serviceInfo, nameField);
                const char* serviceName = env->GetStringUTFChars(serviceNameJString, nullptr);

                // 与我们的指纹列表进行比对
                for (const char* component : MIPUSH_COMPONENTS) {
                    if (strcmp(serviceName, component) == 0) {
                        found = true;
                        break;
                    }
                }

                env->ReleaseStringUTFChars(serviceNameJString, serviceName);
                env->DeleteLocalRef(serviceInfo);
                env->DeleteLocalRef(serviceInfoClass);
                env->DeleteLocalRef(serviceNameJString);
                if (found) break;
            }
            env->DeleteLocalRef(services);
        }
        
        // 清理所有JNI局部引用
        env->DeleteLocalRef(activityThreadClass);
        env->DeleteLocalRef(application);
        env->DeleteLocalRef(contextClass);
        env->DeleteLocalRef(packageManager);
        env->DeleteLocalRef(packageManagerClass);
        env->DeleteLocalRef(packageInfo);
        env->DeleteLocalRef(packageInfoClass);

        return found;
    }

    static bool isPotentialTargetProcess(const string &processName) {
        if (processName.empty()) return false;
        if (processName.find(':') == string::npos) return true;
        if (processName.length() > 5 && processName.substr(processName.length() - 5) == ":push") return true;
        if (processName.length() > 12 && processName.substr(processName.length() - 12) == ":pushservice") return true;
        return false;
    }

    static string parsePackageName(const char *app_data_dir) {
        if (app_data_dir && *app_data_dir) {
            char package_name[256] = {0};
            if (sscanf(app_data_dir, "/data/user/%*d/%s", package_name) == 1 ||
                sscanf(app_data_dir, "/data/data/%s", package_name) == 1) {
                return package_name;
            }
        }
        return "";
    }
};

REGISTER_ZYGISK_MODULE(MiPushZygisk)