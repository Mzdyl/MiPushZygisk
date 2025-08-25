#include <cstdlib>
#include <unistd.h>
#include <string>
#include <vector>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include "zygisk.hpp"
#include "logging.h"
#include "hook.h"
#include "util.h"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

using namespace std;

#define MODULE_DIR "/data/adb/modules/zygisk_mipushfake"
#define WHITELIST_PATH MODULE_DIR "/package.list"

static std::unordered_set<std::string> target_packages;
static bool list_loaded = false;

class MiPushZygisk : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;

        if (!list_loaded) {
            loadWhitelist();
            list_loaded = true;
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        string package_name = parsePackageName(jstringToStdString(env, args->app_data_dir).c_str());
        if (package_name.empty() || target_packages.find(package_name) == target_packages.end()) {
            return;
        }

        string process_name = jstringToStdString(env, args->nice_name);
        if (!isTargetProcess(process_name)) {
            return;
        }

        LOGI("Package [%s] is whitelisted and process [%s] is a target type. Applying hooks.",
             package_name.c_str(), process_name.c_str());
        Hook(api, env).hook();
    }

    void preServerSpecialize(ServerSpecializeArgs *args) override {
        api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
    }

private:
    Api *api;
    JNIEnv *env;

    static bool isTargetProcess(const string &processName) {
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

    void loadWhitelist() {
        std::ifstream file(WHITELIST_PATH);
        if (!file.is_open()) {
            LOGW("Failed to open whitelist file: %s", WHITELIST_PATH);
            return;
        }

        LOGD("Loading whitelist from %s", WHITELIST_PATH);
        string line;
        int count = 0;
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (!line.empty()) {
                target_packages.insert(line);
                count++;
            }
        }
        file.close();
    }

    static string parsePackageName(const char *app_data_dir) {
        if (app_data_dir && *app_data_dir) {
            char package_name[256] = {0};
            if (sscanf(app_data_dir, "/data/user/%*d/%s", package_name) == 1 ||
                sscanf(app_data_dir, "/data/data/%s", package_name) == 1) {
                char *suffix = strchr(package_name, '/');
                if (suffix) {
                    *suffix = '\0';
                }
                return package_name;
            }
        }
        return "";
    }
};

REGISTER_ZYGISK_MODULE(MiPushZygisk)
