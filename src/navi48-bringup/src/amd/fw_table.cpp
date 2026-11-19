//
//  fw_table.cpp — implementation of ../fw/fw_table.h.
//
//  Lives in src/amd/ (not src/fw/, despite the header living there) on
//  purpose: the Makefile globs `src/fw/*.c` and `src/amd/*.cpp`, but NOT
//  `src/fw/*.cpp` (see the Makefile's CXX_SRCS/C_SRCS lines, and
//  PORTING.md rule 8 — the Makefile is not to be edited). A same-
//  directory src/fw/fw_table.cpp would therefore never be added to
//  CXX_SRCS and would silently never compile or link. Putting the C++
//  translation unit here, picked up by `$(wildcard src/amd/*.cpp)`,
//  keeps the build's existing glob untouched while still building this
//  file. fw_table.h itself is a header (not compiled directly), so its
//  location in src/fw/ has no effect on the build.
//
//  No allocation, no STL, no exceptions/RTTI. Table build strategy:
//
//  Each generated src/fw/fw_<blob>.c defines `fw_<blob>_size` as an
//  extern `const size_t` in ITS OWN translation unit — this TU only sees
//  its *declaration*, not its value, so `fw_<blob>_size` cannot be used
//  as a constant-expression initializer here (tried `constexpr FwBlob
//  kFwTable[] = { { fw_x, fw_x_size, ... }, ... };` first: clang rejects
//  it — "constexpr variable must be initialized by a constant
//  expression" / "initializer of 'fw_x_size' is unknown" — because
//  copying an extern object's *value* across translation units is not a
//  link-time-resolvable relocation the way taking its *address* is).
//  The general fix for a POD table like this is a C++ dynamic
//  initializer (global constructor), but that needs the kext's C++
//  runtime to run `__mod_init_func`-style initializers before anything
//  calls fw_get()/fw_find_by_name() — an ordering guarantee this file
//  shouldn't assume. Instead, g_table[]/g_built below are plain
//  zero-initialized globals (.bss, no initializer at all) and
//  build_table() fills them in with an ordinary function call the first
//  time either public entry point runs — ordinary runtime assignments,
//  no allocation, no static-initialization-order dependency.
//
#include "../fw/fw_table.h"
#include "asic_profile.h"

namespace amdgpu {

// ---- extern "C" symbols from the generated src/fw/fw_*.c files --------
//
// Names/format match tools/embed-firmware.py's output and the existing
// hand-written src/fw/fw_psp_sos.c: `const uint8_t <sym>[]` (64-byte
// aligned) + `const size_t <sym>_size`.
extern "C" {

// Symbol names come from asic_profile.h (PSP 14.0.3 / GC 12.0.1 blobs on Navi 48, 14.0.2 / 12.0.0 on Navi 44).
extern const uint8_t COBALT_FW_PSP_SYM(sos)[];
extern const size_t  COBALT_FW_PSP_SIZE(sos);

extern const uint8_t COBALT_FW_PSP_SYM(ta)[];
extern const size_t  COBALT_FW_PSP_SIZE(ta);

extern const uint8_t COBALT_FW_GC_SYM(imu)[];
extern const size_t  COBALT_FW_GC_SIZE(imu);

extern const uint8_t COBALT_FW_GC_SYM(pfp)[];
extern const size_t  COBALT_FW_GC_SIZE(pfp);

extern const uint8_t COBALT_FW_GC_SYM(me)[];
extern const size_t  COBALT_FW_GC_SIZE(me);

extern const uint8_t COBALT_FW_GC_SYM(mec)[];
extern const size_t  COBALT_FW_GC_SIZE(mec);

extern const uint8_t COBALT_FW_GC_SYM(uni_mes)[];
extern const size_t  COBALT_FW_GC_SIZE(uni_mes);

extern const uint8_t COBALT_FW_GC_SYM(rlc)[];
extern const size_t  COBALT_FW_GC_SIZE(rlc);

extern const uint8_t COBALT_FW_SMU_SYM[];
extern const size_t  COBALT_FW_SMU_SIZE;

extern const uint8_t COBALT_FW_SDMA_SYM[];
extern const size_t  COBALT_FW_SDMA_SIZE;

} // extern "C"

namespace {

constexpr uint32_t kCount = static_cast<uint32_t>(FwId::Count);

// Plain zero-initialized statics — no constructor, lands in .bss.
FwBlob g_table[kCount];
bool   g_built = false;

// Fills g_table[] with ordinary runtime assignments (reads each extern
// fw_<blob>_size's actual value — valid the moment this kext's segments
// are mapped, well before any bring-up code could call fw_get()). Safe
// to call repeatedly; only the first call does anything. Bring-up is
// single-threaded through this point, so no guard/lock is used — see
// the file header.
//
// GC_MES, GC_MES1 and GC_TOC have no embedded blob on this card's
// non-kicker psp_14_0_3/gfx1201 bring-up ladder (uni_mes supersedes the
// legacy split MES pair; TOC is only loaded for psp_14_0_5), so those
// rows keep data=nullptr, size=0 — see fw_table.h.
void build_table() {
    if (g_built) return;

    g_table[static_cast<uint32_t>(FwId::PSP_SOS)] =
        FwBlob{ COBALT_FW_PSP_SYM(sos), COBALT_FW_PSP_SIZE(sos), COBALT_FW_PSP_NAME(sos), FwId::PSP_SOS };
    g_table[static_cast<uint32_t>(FwId::PSP_TA)] =
        FwBlob{ COBALT_FW_PSP_SYM(ta), COBALT_FW_PSP_SIZE(ta), COBALT_FW_PSP_NAME(ta), FwId::PSP_TA };
    g_table[static_cast<uint32_t>(FwId::GC_IMU)] =
        FwBlob{ COBALT_FW_GC_SYM(imu), COBALT_FW_GC_SIZE(imu), COBALT_FW_GC_NAME(imu), FwId::GC_IMU };
    g_table[static_cast<uint32_t>(FwId::GC_PFP)] =
        FwBlob{ COBALT_FW_GC_SYM(pfp), COBALT_FW_GC_SIZE(pfp), COBALT_FW_GC_NAME(pfp), FwId::GC_PFP };
    g_table[static_cast<uint32_t>(FwId::GC_ME)] =
        FwBlob{ COBALT_FW_GC_SYM(me), COBALT_FW_GC_SIZE(me), COBALT_FW_GC_NAME(me), FwId::GC_ME };
    g_table[static_cast<uint32_t>(FwId::GC_MEC)] =
        FwBlob{ COBALT_FW_GC_SYM(mec), COBALT_FW_GC_SIZE(mec), COBALT_FW_GC_NAME(mec), FwId::GC_MEC };
    g_table[static_cast<uint32_t>(FwId::GC_MES)] =
        FwBlob{ nullptr, 0, COBALT_FW_GC_NAME(mes), FwId::GC_MES };
    g_table[static_cast<uint32_t>(FwId::GC_MES1)] =
        FwBlob{ nullptr, 0, COBALT_FW_GC_NAME(mes1), FwId::GC_MES1 };
    g_table[static_cast<uint32_t>(FwId::GC_UNI_MES)] =
        FwBlob{ COBALT_FW_GC_SYM(uni_mes), COBALT_FW_GC_SIZE(uni_mes), COBALT_FW_GC_NAME(uni_mes), FwId::GC_UNI_MES };
    g_table[static_cast<uint32_t>(FwId::GC_RLC)] =
        FwBlob{ COBALT_FW_GC_SYM(rlc), COBALT_FW_GC_SIZE(rlc), COBALT_FW_GC_NAME(rlc), FwId::GC_RLC };
    g_table[static_cast<uint32_t>(FwId::GC_TOC)] =
        FwBlob{ nullptr, 0, COBALT_FW_GC_NAME(toc), FwId::GC_TOC };
    g_table[static_cast<uint32_t>(FwId::SMU)] =
        FwBlob{ COBALT_FW_SMU_SYM, COBALT_FW_SMU_SIZE, COBALT_FW_SMU_NAME, FwId::SMU };
    g_table[static_cast<uint32_t>(FwId::SDMA)] =
        FwBlob{ COBALT_FW_SDMA_SYM, COBALT_FW_SDMA_SIZE, COBALT_FW_SDMA_NAME, FwId::SDMA };

    g_built = true;
}

// Manual equality check — deliberately not strcmp/strncmp. PORTING.md /
// the task brief only guarantee memcpy/memset/strlen/strncmp out of
// <string.h> in this kernel; strcmp isn't in that guaranteed set, and a
// bounded strncmp would need an arbitrary length cap for what is really
// an unbounded exact-match. Blob names are short, static, NUL-terminated
// C strings on both sides, so a plain loop is simplest and dependency-free.
bool str_eq(const char *a, const char *b) {
    while (*a != '\0' && *b != '\0') {
        if (*a != *b) return false;
        ++a;
        ++b;
    }
    return *a == *b; // both must be at NUL simultaneously
}

} // anonymous namespace

const FwBlob *fw_get(FwId id) {
    build_table();
    uint32_t idx = static_cast<uint32_t>(id);
    if (idx >= kCount) return nullptr;
    return &g_table[idx];
}

const FwBlob *fw_find_by_name(const char *name) {
    if (name == nullptr) return nullptr;
    build_table();
    for (uint32_t i = 0; i < kCount; ++i) {
        if (str_eq(g_table[i].name, name)) return &g_table[i];
    }
    return nullptr;
}

} // namespace amdgpu
