# Building the bring-up kext

> **WARNING.** Navi48Bringup.kext writes GPU registers directly (PCIe BARs, PSP, SMU, RLC, SDMA, CP,
> MES, display engine). It is experimental, it has only been run on one x86_64 test PC with one
> RX 9070 XT, and a wrong build or a different card can hang or restart the machine or corrupt
> display state. Do not run it on a machine you cannot afford to restart or recover. You run
> it entirely at your own risk; there is no warranty (see LICENSE).

## Prerequisites

- macOS with the Xcode Command Line Tools (clang, make, codesign, python3).
- MacKernelSDK, taken from the RDNA4FB clone (not vendored here):

  ```
  git clone https://github.com/somestupidgirl/RDNA4FB
  ```

  and pass its MacKernelSDK directory as MKSDK below.
- AMD firmware from linux-firmware (not in this repo, AMD's own licence):

  ```
  git clone https://gitlab.com/kernel-firmware/linux-firmware.git
  tools/fetch-firmware.sh <path-to-that-checkout>
  ```

  This only copies files (no network) into `src/navi48-bringup/firmware/`, which is
  gitignored. The kext embeds exactly these ten files from linux-firmware's `amdgpu/`:

  ```
  gc_12_0_1_imu.bin  gc_12_0_1_me.bin  gc_12_0_1_mec.bin  gc_12_0_1_pfp.bin
  gc_12_0_1_rlc.bin  gc_12_0_1_uni_mes.bin
  psp_14_0_3_sos.bin  psp_14_0_3_ta.bin
  sdma_7_0_1.bin  smu_14_0_3.bin
  ```

  The Makefile stops with a list if any is missing. `tools/fetch-firmware.sh` prints the
  sha256 of each copy against the revision the project was developed with; a different
  linux-firmware revision may work, but only that one has been run.

## Build

```
make -C src/navi48-bringup MKSDK=/path/to/RDNA4FB/MacKernelSDK all
```

The result is `src/navi48-bringup/build/Navi48Bringup.kext` (x86_64, ad-hoc signed, min macOS
11 ABI). Check it with: `dwarfdump --uuid build/Navi48Bringup.kext/Contents/MacOS/Navi48Bringup`
and `nm -u -arch x86_64 <that binary>` (only kernel, IOKit and KPI symbols).

### RX 9060 XT (Navi 44) build — experimental, untested on hardware

The `navi44` branch of this fork adds an `ASIC` switch (see `src/navi48-bringup/src/asic_profile.h`):

```
tools/fetch-firmware.sh <path-to-linux-firmware> navi44     # gc_12_0_0_*, psp_14_0_2_*, sdma_7_0_0, smu_14_0_2
make -C src/navi48-bringup ASIC=navi44 MKSDK=/path/to/RDNA4FB/MacKernelSDK all
```

The result is `src/navi48-bringup/build-navi44/Navi48Bringup.kext`, matching PCI `0x7590` only. It keeps
the same bundle id, class names and N48N ABI, so Mesa (with `mesa-patches/0010`) and the tools work
unchanged; Mesa switches to its gfx1200 table on its own when the kext reports GC 12.0.0.
Remove any OpenCore `device-id` spoof for the card first, or the kext will not match it.

What changes per ASIC: PCI IDs, IP versions (GC 12.0.0 / PSP+SMU 14.0.2 / SDMA 7.0.0), firmware blob
names, and die geometry (2 SE x 2 SA x 8 CU = 32 CUs, 8 RBs). Linux amdgpu drives both dies through the
same code (gfx_v12_0.c, psp_v14_0.c, sdma_v7_0.c, smu_v14_0_2_ppt.c), which is why the rest is shared.
Not yet verified on a 9060 XT: the PSP/SMU bring-up, the DCN display path and the dcn41 register
allowlist ranges (derived on Navi 48).

A Linux/WSL cross build is possible with Apple's ld64 (cctools-port) and a macOS SDK:

```
make -C src/navi48-bringup ASIC=navi44 SDK=/path/MacOSX14.5.sdk MKSDK=/path/MacKernelSDK LDEXTRA=-fuse-ld=/path/to/x86_64-apple-darwin-ld NM_UNDEF="llvm-nm -u" XATTR=true CODESIGN="rcodesign sign"
```

## Optional pieces that are NOT in this repository

The shader cache (`src/shadercache/shadercache.c` is only its parser) and four gfx1201 blit
kernels serve the older route that substitutes shaders inside Apple's own AMDRadeonX6000
accelerator. They derive from Apple's shader binaries and captures and are therefore left
out. The native route (Mesa RADV plus the Metal bundle in `tools/native`) does not use them.
When `firmware/shadercache.bin` and `firmware/shader_blit_*.bin` are absent the Makefile embeds
zero-length blobs under the same symbols. The cache then refuses to open ("sc_open ...
REFUSED ... substitution stays OFF") and every blit-kernel substitution is refused, so
nothing is substituted or armed. Likewise the translator self-tests in `src/xlat12` need
captured Apple IB streams that are not shipped; without them the self-tests report a failure
and the render-translation arms refuse. Everything else, including the compute test shaders
(`src/navi48-bringup/shaders/store_magic*.s` with their assembled `.bin`), is in the tree.

Boot-args and loading are described in the source comments; this file only covers building.
