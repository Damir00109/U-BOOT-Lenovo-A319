# U-Boot for Lenovo A319

Mainline U-Boot port for the Lenovo A319 smartphone (MediaTek MT6572).

The bootloader runs on real hardware, is loaded into RAM through the stock
preloader and never touches the eMMC unless you ask it to.

## Hardware

| | |
|---|---|
| SoC | MediaTek MT6572, dual Cortex-A7, ARMv7 |
| RAM | 512 MiB |
| Storage | Hynix H4G2a eMMC, 3.6 GiB user area + 2 x 4 MiB boot areas |
| Display | OTM8019A, DSI 480x800, 2 lanes, RGB888, video sync-pulse mode |
| Backlight | PWM1 on GPIO128, 32 kHz |
| PMIC | MT6323 |

## Status

Working on hardware:

- boot from the stock preloader, DRAM init, watchdog, UART console at 921600
- display: panel init, DSI video output, backlight and brightness control
- both MMC controllers; eMMC read (verified against a full dump by CRC32) and write
- DOS/EBR partition table parsing
- USB fastboot: `getvar`, `flash`, `erase`, `boot`, `oem run`, `oem console`
- booting a `zImage` with a raw initrd via `bootz`, ATAGS and FDT support

Known limitations:

- the stock Android 3.4 kernel does not boot under U-Boot: it decompresses and then
  resets, because it expects the MediaTek boot tag that LK normally provides
- the environment lives in RAM (`ENV_IS_NOWHERE`), nothing is persisted
- `oem board` is not implemented
- no mainline kernel support yet

## Building

```sh
make lenovo_a319_defconfig
make CROSS_COMPILE=arm-none-eabi- -j$(nproc)
```

The tree builds with the bare-metal `arm-none-eabi-` toolchain.

## Running without flashing

The image is uploaded into RAM by the stock preloader. Any preloader payload
loader works; this project uses `da-boot`:

```sh
da-boot --preloader preloader.bin -i u-boot.bin@0x80020000 -j 0x80020000 preloader
```

Start the loader first, then power-cycle the phone. It must report the MTK
hardware code `0x6572` before uploading anything.

**Do not flash this U-Boot over the stock LK until you have a verified full eMMC
backup.** The preloader, `seccfg`, `nvram` and `protect_f/s` are what make the
device recoverable.

## Display notes

Two details are board specific and easy to get wrong:

1. **Backlight is PWM1 on GPIO128**, not PWM2 on GPIO30. This was confirmed from
   the stock Android GPIO table: pin 128 runs in mode 1 (`pwm1_1`), pin 30 is a
   plain GPIO. The backlight runs at 32 kHz.
2. **The DSI PHY timings must match the values logged by the stock LK.** With the
   generic timings computed from the data rate every line is shifted by a few
   pixels and the picture is sheared. They are passed through the
   `mediatek,phy-timings` property of the DSI node:

   ```dts
   mediatek,phy-timings = <3 3 7 13 12 4 15 7 19 14 3 11 28>;
   ```

   Order: `lpx, da_hs_prepare, da_hs_zero, da_hs_trail, ta_go, ta_sure, ta_get,
   da_hs_exit, clk_hs_zero, clk_hs_trail, clk_hs_prepare, clk_hs_post,
   clk_hs_exit`.

## Fastboot

The device identifies itself as `18d1:4ee0`, product `mt6572`, with a serial
number derived from the eMMC CID.

Partitions are exposed under their MediaTek names through
`fastboot_raw_partition_*` environment variables in
`board/mediatek/mt6572/default.env`:

```
fastboot_raw_partition_uboot=0xe900 0x300
fastboot_raw_partition_bootimg=0xec00 0x3000
fastboot_raw_partition_recovery=0x11c00 0x3000
fastboot_raw_partition_preloader=0x0 0x6000 mmcpart 1
```

Examples:

```sh
fastboot getvar all
fastboot flash bootimg boot.img
fastboot oem "run:mmc dev 0; mmc read 0x90000000 0xec00 0x3000; crc32 0x90000000 0x600000"
```

### Flashing caveats

Two different address views exist and mixing them up writes to the wrong place:

- `/dev/block/mmcblk0` and a raw dump use user-area offsets: boot at `0x1D80000`
  (block `0xEC00`), recovery at `0x2380000` (`0x11C00`), logo at `0x2A40000`
  (`0x15200`), MBR at block 0;
- the MediaTek scatter file and LK's own partition table use those offsets plus
  `0xC00000`, because they count the preloader region (which physically lives in
  the eMMC boot0 area) as part of the user area.

The `fastboot_raw_partition_*` values must use the user-area offsets.

## Booting a kernel from RAM

The stock Android boot image is wrapped twice: inside `ANDROID!` the kernel and
ramdisk are MediaTek containers (`magic 0x58881688`, names `KERNEL`/`ROOTFS`).
U-Boot jumps to the wrapper and dies, so the payloads have to be extracted
first. A working recipe for the extracted `zImage` and initrd:

```
setenv bootargs 'console=ttyMT0,921600n1 root=/dev/ram lcm=1-otm8019a_dsi_vdo_dijing fps=5976 vram=11534336'
setenv fastboot_bootcmd 'bootz 0x90000a00 0x9034a200:0x151ff0'
run do_fastboot
```

then, on the host:

```sh
fastboot boot boot.img
```

`0x90000a00` is the fastboot buffer plus the 0x800 Android header and the 0x200
MediaTek header; `0x9034a200` is the ramdisk after 1683 kernel pages. A raw
initrd has to be given as `address:length`.

The watchdog is stopped before the handoff (`arch_preboot_os()`), otherwise the
board resets 15 seconds into the kernel.

## Credits

This port builds on the MediaTek MT6572 work done by others:

- Max Shevchenko — MT6572 port, device trees, mainline kernel support
- Roman Vivchar — MT6572 MMSYS routing
- Dr. clk120m — MUSB speed improvements, MediaTek keypad driver

The MT6572 U-Boot and Linux trees live at
[mt6572-mainline](https://github.com/mt6572-mainline).
