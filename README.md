# perf_mode — Clevo V250RND 性能模式切换工具

本项目使用 OpenCode + Deepseek-V4-Flash-Free + Deepseek-V4-Pro 制作
因为七彩虹/蓝天公模的笔记本没有 Linux 下的控制中心
导致 4060L 显卡只能跑在 85W 而不是满血的 140W
所以~~鞭策~~使用 AI 做了个工具实现切换模式
目前仅在 七彩虹隐星 P15 23 12650H + 4060L Fedora 上测试通过
理论同型号不同配置和 V250RND 系列通用

适用于 **七彩虹隐星 P15 23** (Colorful Xuying P15 23) / **Clevo V250RND** 笔记本的性能模式切换命令行工具。

支持 4 种模式: 安静 (Quiet)、省电 (Power Saving)、性能 (Performance)、娱乐 (Entertainment)。

## 依赖

- **TUXEDO 驱动** (`tuxedo_io.ko`, `clevo_wmi.ko`, `clevo_acpi.ko` 等) — 提供 `/dev/tuxedo_io` ioctl 接口
  - 来源: [tuxedo-drivers](https://github.com/tuxedocomputers/tuxedo-drivers)
  - 许可证: GPL-2.0+

内核模块可按需直接编译:

```bash
git clone https://github.com/tuxedocomputers/tuxedo-drivers.git
cd tuxedo-drivers
make
sudo make install
```

## 编译

```bash
make
```

## 安装

```bash
sudo make install
```

## 使用

```bash
# 查看当前状态
sudo perf_mode status

# 切换模式
sudo perf_mode 0    # 安静 (Quiet)
sudo perf_mode 1    # 省电 (Power Saving)
sudo perf_mode 2    # 性能 (Performance)
sudo perf_mode 3    # 娱乐 (Entertainment)
```

## 工作原理

通过 TUXEDO `/dev/tuxedo_io` ioctl 接口 (Clevo 路径 `W_CL_PERF_PROFILE`, ioctl `0xEE` cmd `0x15`) 向 EC 写入性能模式，与 **TUXEDO Control Center** 使用相同的通信路径。

后备方案: 通过 `ec_sys` debugfs (`/sys/kernel/debug/ec/ec0/io`) 或 `/dev/ec_io` 直接读写 EC RAM (Uniwill 协议, EC 偏移 `0x0751`)。

## 许可证

本项目使用 **GPL-3.0-or-later** 许可证。详见 [LICENSE](LICENSE)。

## 使用的开源项目

| 项目 | 用途 | 许可证 | 链接 |
|------|------|--------|------|
| [tuxedo-drivers](https://github.com/tuxedocomputers/tuxedo-drivers) | 内核驱动 (`tuxedo_io`, `clevo_wmi` 等), ioctl 定义 | GPL-2.0+ | <https://github.com/tuxedocomputers/tuxedo-drivers> |
| [tuxedo-control-center](https://github.com/tuxedocomputers/tuxedo-control-center) | ioctl 通信协议参考 (ClevoDevice/UniwillDevice) | GPL-3.0+ | <https://github.com/tuxedocomputers/tuxedo-control-center> |
| [acpi_call](https://github.com/nix-community/acpi_call) | ACPI 方法调试调用 (开发阶段参考) | GPL-3.0+ | <https://github.com/nix-community/acpi_call> |

## 适配机型

| 机型 | 代工厂 | 主板 |
|------|--------|------|
| 七彩虹隐星 P15 23 | Clevo | V250RND (Uniwill) |

其他使用 Clevo WMI 接口 (GUID `ABBC0F6D`) 的笔记本也可能兼容。
