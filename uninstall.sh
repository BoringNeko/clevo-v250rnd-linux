#!/bin/bash
# uninstall.sh — 卸载 perf_mode 和 tuxedo-drivers
# 用法: sudo ./uninstall.sh

set -e

BINDIR="/usr/local/bin"
MODDIR="/lib/modules/$(uname -r)/extra"
MODLOAD="/etc/modules-load.d/tuxedo-drivers.conf"

echo "=== 1. 卸载内核模块 ==="
for mod in tuxedo_io clevo_wmi clevo_acpi tuxedo_keyboard tuxedo_compatibility_check; do
    if lsmod | grep -q "^${mod} "; then
        rmmod "$mod" 2>/dev/null && echo "  已卸载: $mod" || echo "  跳过(占用): $mod"
    fi
done

echo ""
echo "=== 2. 删除模块文件 ==="
rm -fv "$MODDIR"/tuxedo_*.ko "$MODDIR"/clevo_*.ko "$MODDIR"/uniwill_*.ko \
        "$MODDIR"/ite_*.ko "$MODDIR"/stk*.ko "$MODDIR"/gxtp*.ko \
        "$MODDIR"/tuxi_*.ko 2>/dev/null || true
depmod -a

echo ""
echo "=== 3. 删除开机加载配置 ==="
rm -fv "$MODLOAD"

echo ""
echo "=== 4. 删除 perf_mode ==="
rm -fv "$BINDIR/perf_mode"

echo ""
echo "卸载完成"
