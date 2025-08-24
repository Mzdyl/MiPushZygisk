/* Copyright 2022-2023 John "topjohnwu" Wu
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

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

class MiPushZygisk : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
        LOGD("MiPushZygisk module loaded successfully.");
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        string process_name = jstringToStdString(env, args->nice_name);
        LOGD("preAppSpecialize called for process: [%s]", process_name.c_str());

        if (!isPushProcess(process_name)) {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        string process_name = jstringToStdString(env, args->nice_name);
        LOGD("postAppSpecialize called for process: [%s]", process_name.c_str());

        if (hasMiPushSDK()) {
            string app_data_dir = jstringToStdString(env, args->app_data_dir);
            string package_name = parsePackageName(app_data_dir.c_str());
            LOGI("MiPush SDK found. Hooking process: %s for package: %s", process_name.c_str(), package_name.c_str());
            Hook(api, env).hook();
        } else {
            LOGI("MiPush SDK not found for process: %s. Module will not hook this process.", process_name.c_str());
        }
    }

    void preServerSpecialize(ServerSpecializeArgs *args) override {
        api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
    }

private:
    Api *api;
    JNIEnv *env;

    // Check if the process name ends with a common push service suffix
    static bool isPushProcess(const string &processName) {
        if (processName.empty()) {
            return false;
        }
        if (processName.find(':') == string::npos) {
            return true;
        }
        if (processName.length() > 5 && processName.substr(processName.length() - 5) == ":push") {
            return true;
        }
        if (processName.length() > 12 && processName.substr(processName.length() - 12) == ":pushservice") {
            return true;
        }
        // Add other common names if needed
        return false;
    }

    // Check for the existence of a core MiPush SDK class
    bool hasMiPushSDK() {
        const char* classes_to_check[] = {
            "com/xiaomi/push/service/receivers/PingReceiver",
            "com/xiaomi/mipush/sdk/MiPushClient",
            "com/xiaomi/push/service/XMPushService",
            "com/xiaomi/push/service/XMJobService",
            "com/xiaomi/mipush/sdk/NotificationClickedActivity",
            "com/xiaomi/mipush/sdk/PushMessageHandler",
            "com/xiaomi/mipush/sdk/MessageHandleService",
            "com/xiaomi/mipush/sdk/PushMessageReceiver",
            "com/xiaomi/push/service/NetworkStatusReceiver",
        };

        for (const char* class_name : classes_to_check) {
            jclass clazz = env->FindClass(class_name);
            if (clazz != nullptr) {
                env->DeleteLocalRef(clazz);
                LOGD("Found MiPush SDK class: %s", class_name);
                return true;
            }
            env->ExceptionClear();
        }

        return false;
    }

    static string parsePackageName(const char *app_data_dir) {
        if (app_data_dir && *app_data_dir) {
            char package_name[256] = {0};
            // /data/user/<user_id>/<package>
            if (sscanf(app_data_dir, "/data/%*[^/]/%*[^/]/%s", package_name) == 1) {
                return package_name;
            }

            // /mnt/expand/<id>/user/<user_id>/<package>
            if (sscanf(app_data_dir, "/mnt/expand/%*[^/]/%*[^/]/%*[^/]/%s", package_name) == 1) {
                return package_name;
            }

            // /data/data/<package>
            if (sscanf(app_data_dir, "/data/%*[^/]/%s", package_name) == 1) {
                return package_name;
            }
        }
        return "";
    }
};

// Register our module class
REGISTER_ZYGISK_MODULE(MiPushZygisk)
