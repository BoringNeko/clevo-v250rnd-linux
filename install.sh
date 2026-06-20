#!/bin/bash
# install.sh — 安装 perf_mode 和 tuxedo-drivers 到系统
# 用法: sudo ./install.sh [tuxedo-drivers源码目录]

set -e

TUXEDO_SRC="${1:-/tmp/tuxedo-drivers-test}"
MODDIR="/lib/modules/$(uname -r)/extra"
BINDIR="/usr/local/bin"
MODLOAD="/etc/modules-load.d/tuxedo-drivers.conf"

echo "=== 1. 编译并安装 tuxedo-drivers 内核模块 ==="
if [ -f "$TUXEDO_SRC/src/tuxedo_io/tuxedo_io.ko" ]; then
    echo "  模块已编译, 跳过编译"
else
    echo "  编译中..."
    make -C "$TUXEDO_SRC" -j"$(nproc)"
fi

echo "  安装到 $MODDIR"
mkdir -p "$MODDIR"
cp -v "$TUXEDO_SRC/src/"*.ko "$MODDIR/" 2>/dev/null || true
cp -v "$TUXEDO_SRC/src/"*/*.ko "$MODDIR/" 2>/dev/null || true
depmod -a

echo ""
echo "=== 2. 设置开机自动加载 ==="
cat > "$MODLOAD" << 'EOF'
# TUXEDO drivers for Clevo V250RND performance mode switching
tuxedo_compatibility_check
tuxedo_keyboard
clevo_acpi
clevo_wmi
tuxedo_io
EOF
echo "  已写入 $MODLOAD"

echo ""
echo "=== 3. 立即加载模块 ==="
modprobe tuxedo_compatibility_check 2>/dev/null || true
modprobe tuxedo_keyboard         2>/dev/null || true
modprobe clevo_acpi              2>/dev/null || true
modprobe clevo_wmi               2>/dev/null || true
modprobe tuxedo_io               2>/dev/null || true

echo ""
echo "=== 4. 编译并安装 perf_mode ==="
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
make -C "$SCRIPT_DIR"
install -m 755 "$SCRIPT_DIR/perf_mode" "$BINDIR/"

echo ""
echo "=== 验证 ==="
ls -la /dev/tuxedo_io && echo "  /dev/tuxedo_io 就绪"
echo ""
echo "安装完成! 用法:"
echo "  sudo perf_mode status"
echo "  sudo perf_mode 0|1|2|3"
echo ""
echo "内核升级后需重新编译模块:"
echo "  cd $(readlink -f "$TUXEDO_SRC") && make clean && make && sudo make install"
