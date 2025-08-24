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
#include <sys/stat.h> 

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
        if (!isPushProcess(process_name)) {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        string app_data_dir = jstringToStdString(env, args->app_data_dir);
        if (app_data_dir.empty()) {
            return;
        }

        if (hasMiPushFiles(app_data_dir)) {
            string process_name = jstringToStdString(env, args->nice_name);
            string package_name = parsePackageName(app_data_dir.c_str());
            LOGI("MiPush SDK detected (via files). Hooking process: %s for package: %s", process_name.c_str(), package_name.c_str());
            Hook(api, env).hook();
        } else {
            string process_name = jstringToStdString(env, args->nice_name);
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
        return false;
    }

    static bool hasMiPushFiles(const string& app_data_dir) {
        const vector<string> files_to_check = {
            app_data_dir + "/shared_prefs/mipush.xml",
            app_data_dir + "/shared_prefs/mipush_extra.xml"
        };

        for (const auto& file_path : files_to_check) {
            if (access(file_path.c_str(), F_OK) == 0) {
                LOGD("Found MiPush SDK file: %s", file_path.c_str());
                return true;
            }
        }

        return false;
    }

    static string parsePackageName(const char *app_data_dir) {
        if (app_data_dir && *app_data_dir) {
            char package_name[256] = {0};
            // /data/user/<user_id>/<package>
            if (sscanf(app_data_dir, "/data/user/%*d/%s", package_name) == 1) {
                return package_name;
            }
            // /mnt/expand/<id>/user/<user_id>/<package>
            if (sscanf(app_data_dir, "/mnt/expand/%*[^/]/user/%*d/%s", package_name) == 1) {
                return package_name;
            }
            // /data/data/<package>
            if (sscanf(app_data_dir, "/data/data/%s", package_name) == 1) {
                return package_name;
            }
        }
        return "";
    }
};

// Register our module class
REGISTER_ZYGISK_MODULE(MiPushZygisk)
