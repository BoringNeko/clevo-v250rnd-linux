#!/bin/bash
# install-arch.sh — 安装 perf_mode 和 tuxedo-drivers 到 Arch Linux
# 用法: ./install-arch.sh (注意：请勿使用 sudo 运行)

set -e

# Arch Linux 严禁使用 root 执行 makepkg
if [ "$EUID" -eq 0 ]; then
  echo "❌ 错误: 请不要使用 sudo 运行此脚本！"
  echo "Arch Linux 的 makepkg 需要普通用户权限，脚本会在需要时自动请求 sudo。"
  exit 1
fi

echo "=== 1. 安装必要的构建依赖 (base-devel, dkms, 内核头文件) ==="
# 智能检测当前运行的内核包名 (精准读取 pkgbase 文件)
if [ -f "/usr/lib/modules/$(uname -r)/pkgbase" ]; then
    KERNEL_PKG=$(cat "/usr/lib/modules/$(uname -r)/pkgbase")
else
    KERNEL_PKG="linux" # 默认回退
fi
HEADER_PKG="${KERNEL_PKG}-headers"

echo "  当前内核包: $KERNEL_PKG"
echo "  需要头文件: $HEADER_PKG"
sudo pacman -S --needed --noconfirm base-devel git dkms "$HEADER_PKG"

echo ""
echo "=== 2. 从 AUR 编译并安装 tuxedo-drivers-dkms ==="
if pacman -Qs tuxedo-drivers-dkms > /dev/null; then
    echo "  tuxedo-drivers-dkms 已安装, 跳过 AUR 编译"
else
    BUILD_DIR=$(mktemp -d)
    echo "  克隆 AUR 仓库到 $BUILD_DIR..."
    git clone https://aur.archlinux.org/tuxedo-drivers-dkms.git "$BUILD_DIR"

    cd "$BUILD_DIR"
    echo "  开始编译 DKMS 包..."
    makepkg -si --noconfirm
    cd - > /dev/null

    # 清理临时目录
    rm -rf "$BUILD_DIR"
fi

echo ""
echo "=== 3. 设置开机自动加载 ==="
MODLOAD="/etc/modules-load.d/tuxedo-drivers.conf"
sudo bash -c "cat > $MODLOAD << 'EOF'
# TUXEDO drivers for Clevo V250RND performance mode switching
tuxedo_compatibility_check
tuxedo_keyboard
clevo_acpi
clevo_wmi
tuxedo_io
EOF"
echo "  已写入 $MODLOAD"

echo ""
echo "=== 4. 立即加载内核模块 ==="
sudo modprobe tuxedo_compatibility_check 2>/dev/null || true
sudo modprobe tuxedo_keyboard         2>/dev/null || true
sudo modprobe clevo_acpi              2>/dev/null || true
sudo modprobe clevo_wmi               2>/dev/null || true
sudo modprobe tuxedo_io               2>/dev/null || true

echo ""
echo "=== 5. 编译并安装 perf_mode ==="
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"
make
sudo install -m 755 perf_mode /usr/local/bin/

echo ""
echo "=== 验证 ==="
if [ -c /dev/tuxedo_io ]; then
    ls -la /dev/tuxedo_io
    echo "  /dev/tuxedo_io 就绪"
else
    echo "  警告: /dev/tuxedo_io 未找到，可能需要重启系统使 DKMS 模块生效。"
fi

echo ""
echo "✅ 安装完成! 用法:"
echo "  sudo perf_mode status"
echo "  sudo perf_mode 0|1|2|3"
echo ""
echo "🎉 核心优势: 由于使用了 DKMS，未来通过 pacman 更新内核时，驱动会自动重新编译，无需手动干预！"
