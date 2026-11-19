//
//  asic_profile.h — the one place that says which RDNA 4 die this kext is built for.
//
//  Select at build time with the Makefile's ASIC variable (default navi48):
//
//      make ASIC=navi48   ->  RX 9070 / 9070 XT / AI PRO R9700   (Navi 48, gfx1201, GC 12.0.1)
//      make ASIC=navi44   ->  RX 9060 / 9060 XT                  (Navi 44, gfx1200, GC 12.0.0)
//
//  which passes -DCOBALT_ASIC_NAVI44=1 for the Navi 44 build. Everything that used to hard-code
//  Navi 48 (PCI IDs, IP versions, firmware blob names, die geometry) reads it from here instead.
//
//  Sources for the Navi 44 values (Linux amdgpu, drivers/gpu/drm/amd):
//    - IP versions: amdgpu_discovery.c / soc24.c (GC 12.0.0, MP0/MP1 14.0.2, SDMA 7.0.0); the
//      firmware names follow from them (amdgpu_ucode.c naming: gc_12_0_0_*, psp_14_0_2_*,
//      smu_14_0_2, sdma_7_0_0), all present in linux-firmware's amdgpu/.
//    - Linux treats GC 12.0.0 and 12.0.1 identically in gfx_v12_0.c / imu_v12_0.c / mes_v12_0.c,
//      PSP 14.0.2 and 14.0.3 identically in psp_v14_0.c, SDMA 7.0.0 and 7.0.1 identically in
//      sdma_v7_0.c, and SMU 14.0.2 / 14.0.3 share smu_v14_0_2_ppt.c. The differences that remain
//      (soc24.c external_rev_id offset, PG flags, the DF ECC IH client id in gmc_v12_0.c) do not
//      touch this kext's bring-up path.
//    - Die geometry: Navi 44 is half a Navi 48: 2 SE x 2 SA/SE x 4 WGP/SA x 2 CU/WGP = 32 CUs,
//      4 RB/SE = 8 RBs, 128-bit GDDR6. The live GB_ADDR_CONFIG read cross-checks this at boot.
//
#pragma once

#include <stdint.h>

#define COBALT_XCAT2_(a, b) a##b
#define COBALT_XCAT2(a, b)  COBALT_XCAT2_(a, b)
#define COBALT_XCAT3_(a, b, c) a##b##c
#define COBALT_XCAT3(a, b, c)  COBALT_XCAT3_(a, b, c)

#if defined(COBALT_ASIC_NAVI44) && COBALT_ASIC_NAVI44

#define COBALT_ASIC_NAME        "Navi 44"
#define COBALT_ASIC_GFX_NAME    "gfx1200"
#define COBALT_ASIC_PCI_ID0     0x7590u      // RX 9060 XT
#define COBALT_ASIC_PCI_ID1     0x7590u      // (no second SKU wired up yet)
#define COBALT_ASIC_PCI_NAME0   "RX 9060 XT"
#define COBALT_ASIC_PCI_NAME1   "RX 9060 XT"

// IP versions
#define COBALT_IP_GFX   12, 0, 0
#define COBALT_IP_SDMA   7, 0, 0
#define COBALT_IP_PSP   14, 0, 2
#define COBALT_IP_SMU   14, 0, 2
#define COBALT_IP_MES   12, 0, 0

// Firmware blob symbol prefixes (tools/embed-firmware.py names them fw_<file stem>)
#define COBALT_FW_GC_SYM(part)     COBALT_XCAT2(fw_gc_12_0_0_, part)
#define COBALT_FW_GC_SIZE(part)    COBALT_XCAT3(fw_gc_12_0_0_, part, _size)
#define COBALT_FW_GC_NAME(part)    "gc_12_0_0_" #part
#define COBALT_FW_PSP_SYM(part)    COBALT_XCAT2(fw_psp_14_0_2_, part)
#define COBALT_FW_PSP_SIZE(part)   COBALT_XCAT3(fw_psp_14_0_2_, part, _size)
#define COBALT_FW_PSP_NAME(part)   "psp_14_0_2_" #part
#define COBALT_FW_SMU_SYM          fw_smu_14_0_2
#define COBALT_FW_SMU_SIZE         fw_smu_14_0_2_size
#define COBALT_FW_SMU_NAME         "smu_14_0_2"
#define COBALT_FW_SDMA_SYM         fw_sdma_7_0_0
#define COBALT_FW_SDMA_SIZE        fw_sdma_7_0_0_size
#define COBALT_FW_SDMA_NAME        "sdma_7_0_0"

// Die geometry (gfx_v12_0_gpu_early_init + discovery gc_info)
#define COBALT_MAX_SE            2u
#define COBALT_MAX_SH_PER_SE     2u
#define COBALT_MAX_RB_PER_SE     4u
#define COBALT_MAX_CU_PER_SH     8u
#define COBALT_MAX_HW_CONTEXTS   8u

#else // Navi 48 (default, the upstream Cobalt64 target)

#define COBALT_ASIC_NAVI48 1
#define COBALT_ASIC_NAME        "Navi 48"
#define COBALT_ASIC_GFX_NAME    "gfx1201"
#define COBALT_ASIC_PCI_ID0     0x7550u      // RX 9070 / 9070 XT
#define COBALT_ASIC_PCI_ID1     0x7551u      // Radeon AI PRO R9700
#define COBALT_ASIC_PCI_NAME0   "RX 9070/XT"
#define COBALT_ASIC_PCI_NAME1   "R9700"

#define COBALT_IP_GFX   12, 0, 1
#define COBALT_IP_SDMA   7, 0, 1
#define COBALT_IP_PSP   14, 0, 3
#define COBALT_IP_SMU   14, 0, 3
#define COBALT_IP_MES   12, 0, 1

#define COBALT_FW_GC_SYM(part)     COBALT_XCAT2(fw_gc_12_0_1_, part)
#define COBALT_FW_GC_SIZE(part)    COBALT_XCAT3(fw_gc_12_0_1_, part, _size)
#define COBALT_FW_GC_NAME(part)    "gc_12_0_1_" #part
#define COBALT_FW_PSP_SYM(part)    COBALT_XCAT2(fw_psp_14_0_3_, part)
#define COBALT_FW_PSP_SIZE(part)   COBALT_XCAT3(fw_psp_14_0_3_, part, _size)
#define COBALT_FW_PSP_NAME(part)   "psp_14_0_3_" #part
#define COBALT_FW_SMU_SYM          fw_smu_14_0_3
#define COBALT_FW_SMU_SIZE         fw_smu_14_0_3_size
#define COBALT_FW_SMU_NAME         "smu_14_0_3"
#define COBALT_FW_SDMA_SYM         fw_sdma_7_0_1
#define COBALT_FW_SDMA_SIZE        fw_sdma_7_0_1_size
#define COBALT_FW_SDMA_NAME        "sdma_7_0_1"

#define COBALT_MAX_SE            4u
#define COBALT_MAX_SH_PER_SE     2u
#define COBALT_MAX_RB_PER_SE     4u
#define COBALT_MAX_CU_PER_SH     8u
#define COBALT_MAX_HW_CONTEXTS   8u

#endif

// The 4x2 per-SA bitmaps in GFXConfig / Navi48Ttl are sized for the largest die (Navi 48).
#if COBALT_MAX_SE > 4 || COBALT_MAX_SH_PER_SE > 2
#error "asic_profile: die larger than the [4][2] per-SA arrays"
#endif
