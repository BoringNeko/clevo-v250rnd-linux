/*
 * perf_mode.c — Clevo V250RND (Colorful Xuying P15 23) performance mode switcher
 *
 * Communicates with EC via TUXEDO ioctl interface (/dev/tuxedo_io).
 * Supports 4 performance modes: Quiet, Power Saving, Performance, Entertainment.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

/* ========== TUXEDO ioctl definitions (from tuxedo_io_ioctl.h) ========== */

#define IOCTL_MAGIC      0xEC
#define MAGIC_READ_CL    (IOCTL_MAGIC + 1)
#define MAGIC_WRITE_CL   (IOCTL_MAGIC + 2)
#define MAGIC_READ_UW    (IOCTL_MAGIC + 3)
#define MAGIC_WRITE_UW   (IOCTL_MAGIC + 4)

/* Clevo interface — used by TUXEDO Control Center on Clevo laptops */
#define W_CL_PERF_PROFILE  _IOW(MAGIC_WRITE_CL, 0x15, int32_t*)
#define R_CL_HW_IF_STR     _IOR(MAGIC_READ_CL,  0x00, char*)
#define R_CL_FANINFO1      _IOR(MAGIC_READ_CL,  0x10, int32_t*)
#define R_HWCHECK_CL       _IOR(IOCTL_MAGIC,    0x05, int32_t*)
#define R_MOD_VERSION      _IOR(IOCTL_MAGIC,    0x00, char*)

/* Uniwill interface — fallback for Uniwill-based laptops */
#define W_UW_PERF_PROF  _IOW(MAGIC_WRITE_UW, 0x18, int32_t*)
#define R_UW_MODE       _IOR(MAGIC_READ_UW,  0x14, int32_t*)

/* ========== Direct EC fallback (requires ec_sys or ec_io module) ========== */

#define EC_IO      "/sys/kernel/debug/ec/ec0/io"
#define EC_IO_DEV  "/dev/ec_io"
#define UW_EC_DEV  "/dev/uw_ec"
#define EC_FLAGS   0x8c
#define EC_LDAT    0x8a
#define EC_HDAT    0x8b
#define EC_CMDL    0x8d
#define EC_CMDH    0x8e
#define BIT_BFLG   2
#define BIT_RFLG   0
#define BIT_WFLG   1
#define BIT_DRDY   7
#define PERF_ADDR  0x0751

/* ========== Mode names ========== */

static const char *mode_name[] = {
    [0] = "安静 (Quiet)",
    [1] = "省电 (Power Saving)",
    [2] = "性能 (Performance)",
    [3] = "娱乐 (Entertainment)"
};

static int to_uw_profile(int mode) {
    if (mode <= 1) return 1;
    return mode;
}

/* ========== TUXEDO /dev/tuxedo_io backend ========== */

static int tuxedo_available(void) {
    return access("/dev/tuxedo_io", F_OK) == 0;
}

static int tuxedo_set_mode(int mode) {
    int fd = open("/dev/tuxedo_io", O_RDWR);
    if (fd < 0) return -1;
    int ret;
    int32_t val;

    /* Try Clevo path first — used by TCC on Clevo laptops */
    val = mode;
    ret = ioctl(fd, W_CL_PERF_PROFILE, &val);
    if (ret >= 0) {
        int32_t finfo;
        if (ioctl(fd, R_CL_FANINFO1, &finfo) >= 0) {
            close(fd);
            return 0;
        }
    }

    /* Fallback: Uniwill path */
    int32_t profile = to_uw_profile(mode);
    ret = ioctl(fd, W_UW_PERF_PROF, &profile);
    close(fd);
    return ret;
}

/* ========== Direct EC backend (Uniwill EC protocol) ========== */

static int ec_write8(int fd, uint16_t off, uint8_t val) {
    return pwrite(fd, &val, 1, off);
}

static int ec_read8(int fd, uint16_t off, uint8_t *val) {
    return pread(fd, val, 1, off);
}

static int uw_wait_drdy(int fd) {
    for (int i = 0; i < 100; i++) {
        uint8_t flags;
        ec_read8(fd, EC_FLAGS, &flags);
        if (flags & (1 << BIT_DRDY)) return 0;
        usleep(1000);
    }
    return -1;
}

static int uw_ec_read(int fd, uint16_t addr, uint8_t *data) {
    uint8_t addr_low = addr & 0xff, addr_high = (addr >> 8) & 0xff;
    uint8_t flags;
    ec_read8(fd, EC_FLAGS, &flags);
    ec_write8(fd, EC_FLAGS, flags | (1 << BIT_BFLG));
    ec_write8(fd, EC_LDAT, addr_low);
    ec_write8(fd, EC_HDAT, addr_high);
    flags &= ~(1 << BIT_DRDY);
    flags |= (1 << BIT_RFLG);
    ec_write8(fd, EC_FLAGS, flags);
    if (uw_wait_drdy(fd) < 0) { ec_write8(fd, EC_FLAGS, 0); return -1; }
    ec_read8(fd, EC_CMDL, data);
    ec_write8(fd, EC_FLAGS, 0);
    return 0;
}

static int uw_ec_write(int fd, uint16_t addr, uint8_t data) {
    uint8_t addr_low = addr & 0xff, addr_high = (addr >> 8) & 0xff;
    uint8_t flags;
    ec_read8(fd, EC_FLAGS, &flags);
    ec_write8(fd, EC_FLAGS, flags | (1 << BIT_BFLG));
    ec_write8(fd, EC_LDAT, addr_low);
    ec_write8(fd, EC_HDAT, addr_high);
    ec_write8(fd, EC_CMDL, data);
    ec_write8(fd, EC_CMDH, 0);
    flags &= ~(1 << BIT_DRDY);
    flags |= (1 << BIT_WFLG);
    ec_write8(fd, EC_FLAGS, flags);
    if (uw_wait_drdy(fd) < 0) { ec_write8(fd, EC_FLAGS, 0); return -1; }
    ec_write8(fd, EC_FLAGS, 0);
    return 0;
}

static const char *ec_path;

static int ec_sys_available(void) {
    if (access(EC_IO, F_OK) == 0)      { ec_path = EC_IO;     return 1; }
    if (access(EC_IO_DEV, F_OK) == 0)  { ec_path = EC_IO_DEV; return 1; }
    if (access(UW_EC_DEV, F_OK) == 0)  { ec_path = UW_EC_DEV; return 1; }
    return 0;
}

static int ec_sys_set_mode(int mode) {
    int fd = open(ec_path, O_RDWR);
    if (fd < 0) return -1;
    uint8_t val;
    if (uw_ec_read(fd, PERF_ADDR, &val) < 0) { close(fd); return -1; }
    val &= ~(0xa0 | 0x10);
    if (mode <= 1)      val |= 0xa0;
    else if (mode == 2) val |= 0x00;
    else                val |= 0x10;
    int ret = uw_ec_write(fd, PERF_ADDR, val);
    close(fd);
    return ret;
}

/* ========== main ========== */

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "用法: sudo %s <0|1|2|3|status>\n", argv[0]);
        fprintf(stderr, "  0=安静  1=省电  2=性能  3=娱乐\n");
        fprintf(stderr, "  依赖: TUXEDO 驱动 (tuxedo_io.ko) 或 ec_sys/ec_io 模块\n");
        return 1;
    }

    int backend = 0;
    if (tuxedo_available() && access("/dev/tuxedo_io", W_OK) == 0)
        backend = 1;
    else if (ec_sys_available())
        backend = 2;

    if (backend == 0) {
        fprintf(stderr, "找不到可用后端。请确保以下之一可用:\n");
        fprintf(stderr, "  - TUXEDO 驱动: /dev/tuxedo_io\n");
        fprintf(stderr, "  - EC 模块:     sudo modprobe ec_sys write_support=1\n");
        return 1;
    }

    /* ---- status ---- */
    if (strcmp(argv[1], "status") == 0) {
        if (backend == 1) {
            int fd = open("/dev/tuxedo_io", O_RDWR);
            if (fd >= 0) {
                char s[64] = {0}, v[64] = {0};
                int32_t m = 0, hw = 0;
                printf("后端:     TUXEDO ioctl\n");
                if (ioctl(fd, R_MOD_VERSION, v) == 0)
                    printf("模块版本: %s\n", v);
                if (ioctl(fd, R_HWCHECK_CL, &hw) == 0)
                    printf("Clevo 检测: %s\n", hw ? "是" : "否");
                if (ioctl(fd, R_CL_HW_IF_STR, s) == 0 && s[0])
                    printf("Clevo 接口: %s\n", s);
                if (ioctl(fd, R_UW_MODE, &m) == 0)
                    printf("EC 0x0751:  0x%02x\n", m);
                close(fd);
            }
        } else {
            int fd = open(ec_path, O_RDWR);
            if (fd >= 0) {
                uint8_t v;
                printf("后端:     直接 EC (%s)\n", ec_path);
                if (uw_ec_read(fd, PERF_ADDR, &v) == 0)
                    printf("EC 0x0751: 0x%02x\n", v);
                else
                    printf("EC 0x0751: 读取失败\n");
                close(fd);
            }
        }
        return 0;
    }

    /* ---- set mode ---- */
    int mode = atoi(argv[1]);
    if (mode < 0 || mode > 3) {
        fprintf(stderr, "模式: 0=安静 1=省电 2=性能 3=娱乐\n");
        return 1;
    }

    int ret = (backend == 1) ? tuxedo_set_mode(mode) : ec_sys_set_mode(mode);
    if (ret < 0) {
        perror("设置失败");
        return 1;
    }

    printf("已切换到: %s\n", mode_name[mode]);
    return 0;
}
