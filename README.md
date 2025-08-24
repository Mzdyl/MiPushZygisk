# MiPush Zygisk

一个用于为应用伪装小米设备的 Magisk/Zygisk 模块，以便于使用 [MiPush](https://github.com/NihilityT/MiPushFramework.git)

本模块基于 Zygisk，实现了无感、高效、全自动的解决方案。

### 特性

- **全自动检测**: 无需任何手动配置，模块会自动检测哪些应用集成了小米推送SDK。
- **精确注入**: 只对目标应用的推送相关进程进行注入，最大程度减少对系统的影响。
- **高效稳定**: 运行时性能开销极低，不影响应用或系统稳定性。

### 下载

前往 [**Release**](https://github.com/Mzdyl/MiPushZygisk/) 页面下载最新的模块刷机包。



### 构建

确保您已配置好 Android NDK 的环境路径，然后在项目根目录执行：

```shell
./build.sh
```

构建产物（zip刷机包）将生成在 `build` 目录下。

### 致谢
感谢[HMSPush](https://github.com/fei-ke/HmsPushZygisk.git)，这个项目99%的代码都是从那边抄的

### License
[GNU General Public License v3 (GPL-3)](http://www.gnu.org/copyleft/gpl.html)。
