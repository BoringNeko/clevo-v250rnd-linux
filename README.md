# perf_mode — Clevo V250RND 性能模式切换工具

本项目使用 OpenCode + Deepseek-V4-Pro 制作。

因为七彩虹/蓝天公模笔记本没有 Linux 下的控制中心，导致 4060L 显卡只能跑在 85W 而不是满血的 140W。此工具通过 TUXEDO ioctl 接口与 EC 通信，实现 4 种性能模式切换。

目前已在 **七彩虹隐星 P15 23** (12650H + 4060L, Fedora 44 / Arch Linux ) 上测试通过。理论同型号不同配置和 Clevo V250RND 系列通用。

支持 4 种模式: 安静 (Quiet)、省电 (Power Saving)、性能 (Performance)、娱乐 (Entertainment)。

不同发行版/内核需要自己编译

已解决 Arch Linux 无法使用的问题

## 安装

### 一键安装

```bash
# 1. 编译 tuxedo-drivers (内核模块)
git clone https://github.com/tuxedocomputers/tuxedo-drivers.git /tmp/tuxedo-drivers
cd /tmp/tuxedo-drivers && make -j$(nproc)

# 2. 一键安装 (模块 + 自启 + perf_mode)
cd /path/to/Mode
sudo ./install.sh /tmp/tuxedo-drivers
```

`install.sh` 会自动:
- 安装内核模块到 `/lib/modules/$(uname -r)/extra/`
- 创建 `/etc/modules-load.d/tuxedo-drivers.conf` (开机自动加载)
- 立即加载所有模块
- 编译并安装 `perf_mode` 到 `/usr/local/bin/`

### Arch Linux 一键安装

```
cd /path/to/Mode
chmod +x install-arch.sh
./install_arch.sh
```

### 手动安装

```
# 编译 tuxedo-drivers (内核模块)
git clone https://github.com/tuxedocomputers/tuxedo-drivers.git /tmp/tuxedo-drivers
cd /tmp/tuxedo-drivers && make -j$(nproc)

# Arch Linux 可以使用 AUR 上的 tuxedo-drivers-dkms 动态内核或自行编译 dkms 内核
yay -S tuxedo-drivers-dkms
```

```bash
# 编译
make

# 安装到系统
sudo make install
```

### 卸载

```bash
sudo ./uninstall.sh
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

### 内核升级后

内核升级后需重新编译 tuxedo-drivers 模块:

```bash
cd /tmp/tuxedo-drivers
make clean && make
sudo make install
```

Arch Linux 安装脚本自带 DKMS 自动处理

推荐使用 DKMS 自动处理内核升级 (参考 [tuxedo-drivers](https://github.com/tuxedocomputers/tuxedo-drivers) DKMS 安装文档)。

### 开启 NVIDIA 动态功耗守护进程

Arch Linux 需要开启此项才能解锁 140W 功耗墙

其他发行版自行测试

```
sudo systemctl enable --now nvidia-powerd
```

## 依赖

- **TUXEDO 驱动** (`tuxedo_io.ko`, `clevo_wmi.ko`, `clevo_acpi.ko`, `tuxedo_keyboard.ko`, `tuxedo_compatibility_check.ko`)
  - 提供 `/dev/tuxedo_io` ioctl 接口
  - 来源: [tuxedo-drivers](https://github.com/tuxedocomputers/tuxedo-drivers) (GPL-2.0+)

## 文件说明

| 文件 | 说明 |
|------|------|
| `perf_mode.c` | 主程序源码 |
| `Makefile` | 编译 (`make` / `make clean` / `make install`) |
| `install.sh` | 一键安装脚本 (tuxedo-drivers + perf_mode) |
| `install_arch.sh` | Arch Linux的一键安装脚本 (tuxedo-drivers-dkms + perf_mode) |
| `uninstall.sh` | 一键卸载脚本 |
| `LICENSE` | GPL-3.0 许可证 |

## 工作原理

通过 TUXEDO `/dev/tuxedo_io` ioctl 接口 (Clevo 路径 `W_CL_PERF_PROFILE`, ioctl `0xEE` cmd `0x15`) 向 EC 写入性能模式，与 **TUXEDO Control Center** 使用相同的通信路径。

后备方案: 通过 `ec_sys` debugfs 或 `/dev/ec_io` 直接读写 EC RAM (Uniwill 协议, EC 偏移 `0x0751`)。

## 适配机型

| 机型 | 代工厂 | 主板 |
|------|--------|------|
| 七彩虹隐星 P15 23 | Clevo | V250RND (Uniwill) |

其他使用 Clevo WMI 接口 (GUID `ABBC0F6D`) 的笔记本也可能兼容。

## 许可证

本项目使用 **GPL-3.0-or-later** 许可证。详见 [LICENSE](LICENSE)。

## 使用的开源项目

| 项目 | 用途 | 许可证 |
|------|------|--------|
| [tuxedo-drivers](https://github.com/tuxedocomputers/tuxedo-drivers) | 内核驱动 (`tuxedo_io`, `clevo_wmi` 等), ioctl 定义 | GPL-2.0+ |
| [tuxedo-control-center](https://github.com/tuxedocomputers/tuxedo-control-center) | ioctl 通信协议参考 (ClevoDevice 类) | GPL-3.0+ |
| [acpi_call](https://github.com/nix-community/acpi_call) | ACPI 方法调试 (开发阶段) | GPL-3.0+ |
