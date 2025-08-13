// This file is a part of Julia. License is MIT: https://julialang.org/license

// RISC-V 64 specific processor detection and dispatch

#include "llvm-version.h"
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/MathExtras.h>
#include <llvm/Support/raw_ostream.h>

#include "processor.h"
#include "julia.h"
#include "julia_internal.h"

#include <sys/stat.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <set>
#include <fstream>
#include <algorithm>
#include <map>

namespace RISCV64 {

enum class CPU : uint32_t {
    generic = 0,

    // Generic RISC-V 64 targets
    rv64gc,      // RV64G + compressed instructions
    rv64gcv,     // RV64GC + vector extension
    rv64imafdc,  // RV64IMAFDC (equivalent to RV64GC)
    rv64imafdcv, // RV64IMAFDC + vector extension

    // Vendor-specific targets (generic implementations)
    sifive_u74,  // SiFive U74-MC
    sifive_u84,  // SiFive U84-MC
    sifive_u87,  // SiFive U87-MC
    sifive_u89,  // SiFive U89-MC
    sifive_u9,   // SiFive U9-MC
    sifive_u9a,  // SiFive U9A-MC
    sifive_u9b,  // SiFive U9B-MC
    sifive_u9c,  // SiFive U9C-MC
    sifive_u9d,  // SiFive U9D-MC
    sifive_u9e,  // SiFive U9E-MC
    sifive_u9f,  // SiFive U9F-MC
    sifive_u9g,  // SiFive U9G-MC
    sifive_u9h,  // SiFive U9H-MC
    sifive_u9i,  // SiFive U9I-MC
    sifive_u9j,  // SiFive U9J-MC
    sifive_u9k,  // SiFive U9K-MC
    sifive_u9l,  // SiFive U9L-MC
    sifive_u9m,  // SiFive U9M-MC
    sifive_u9n,  // SiFive U9N-MC
    sifive_u9o,  // SiFive U9O-MC
    sifive_u9p,  // SiFive U9P-MC
    sifive_u9q,  // SiFive U9Q-MC
    sifive_u9r,  // SiFive U9R-MC
    sifive_u9s,  // SiFive U9S-MC
    sifive_u9t,  // SiFive U9T-MC
    sifive_u9u,  // SiFive U9U-MC
    sifive_u9v,  // SiFive U9V-MC
    sifive_u9w,  // SiFive U9W-MC
    sifive_u9x,  // SiFive U9X-MC
    sifive_u9y,  // SiFive U9Y-MC
    sifive_u9z,  // SiFive U9Z-MC
};

static constexpr size_t feature_sz = 4;
static constexpr FeatureName feature_names[] = {
#define JL_FEATURE_DEF(name, bit, llvmver) {#name, bit, llvmver},
#define JL_FEATURE_DEF_NAME(name, bit, llvmver, str) {str, bit, llvmver},
#include "features_riscv64.h"
#undef JL_FEATURE_DEF
#undef JL_FEATURE_DEF_NAME
};
static constexpr uint32_t nfeature_names = sizeof(feature_names) / sizeof(FeatureName);

template<typename... Args>
static inline constexpr FeatureList<feature_sz> get_feature_masks(Args... args)
{
    return ::get_feature_masks<feature_sz>(args...);
}

#define JL_FEATURE_DEF_NAME(name, bit, llvmver, str) JL_FEATURE_DEF(name, bit, llvmver)
static constexpr auto feature_masks = get_feature_masks(
#define JL_FEATURE_DEF(name, bit, llvmver) bit,
#include "features_riscv64.h"
#undef JL_FEATURE_DEF
    -1);

namespace Feature {
enum : uint32_t {
#define JL_FEATURE_DEF(name, bit, llvmver) name = bit,
#include "features_riscv64.h"
#undef JL_FEATURE_DEF
};
#undef JL_FEATURE_DEF_NAME

// Feature dependencies for RISC-V 64
static constexpr FeatureDep deps[] = {
    {d, f},                    // Double precision requires single precision
    {zfinx, f},                // Zfinx requires F extension
    {zdinx, d},                // Zdinx requires D extension
    {zhinx, f},                // Zhinx requires F extension
    {zhinxmin, f},             // Zhinxmin requires F extension
    {zve32f, zve32x},          // Zve32f requires Zve32x
    {zve64f, zve64x},          // Zve64f requires Zve64x
    {zve64d, zve64f},          // Zve64d requires Zve64f
    {zve64f, zve32f},          // Zve64f requires Zve32f
    {zve64x, zve32x},          // Zve64x requires Zve32x
    {zve32x, v},               // Zve32x requires V extension
    {zve32f, v},               // Zve32f requires V extension
    {zve64x, v},               // Zve64x requires V extension
    {zve64f, v},               // Zve64f requires V extension
    {zve64d, v},               // Zve64d requires V extension
    {zvbb, v},                 // Zvbb requires V extension
    {zvbc, v},                 // Zvbc requires V extension
    {zvfbfmin, v},             // Zvfbfmin requires V extension
    {zvfbfwma, v},             // Zvfbfwma requires V extension
    {zvkg, v},                 // Zvkg requires V extension
    {zvkned, v},               // Zvkned requires V extension
    {zvknha, v},               // Zvknha requires V extension
    {zvknhb, v},               // Zvknhb requires V extension
    {zvksed, v},               // Zvksed requires V extension
    {zvksh, v},                // Zvksh requires V extension
    {zvkb, v},                 // Zvkb requires V extension
    {zca, c},                  // Zca requires C extension
    {zcb, c},                  // Zcb requires C extension
    {zcd, c},                  // Zcd requires C extension
    {zcf, c},                  // Zcf requires C extension
    {zcmp, c},                 // Zcmp requires C extension
    {zcmt, c},                 // Zcmt requires C extension
    {zalrsc, a},               // Zalrsc requires A extension
    {zacas, a},                // Zacas requires A extension
    {zmmul, m},                // Zmmul requires M extension
    {zknd, aes},               // Zknd requires AES
    {zkne, aes},               // Zkne requires AES
    {zknh, sha2},              // Zknh requires SHA2
    {zksed, sm4},              // Zksed requires SM4
    {zksh, sm3},               // Zksh requires SM3
    {zk, zknd},                // Zk requires Zknd
    {zk, zkne},                // Zk requires Zkne
    {zk, zknh},                // Zk requires Zknh
    {zk, zksed},               // Zk requires Zksed
    {zk, zksh},                // Zk requires Zksh
    {zk, zkr},                 // Zk requires Zkr
    {zvknha, aes},             // Zvknha requires AES
    {zvknhb, sha2},            // Zvknhb requires SHA2
    {zvksed, sm4},             // Zvksed requires SM4
    {zvksh, sm3},              // Zvksh requires SM3
    {zfa, f},                  // Zfa requires F extension
    {zfh, f},                  // Zfh requires F extension
    {zfhmin, f},               // Zfhmin requires F extension
    {zfh, zfhmin},             // Zfh requires Zfhmin
    {zfinx, f},                // Zfinx requires F extension
    {zdinx, d},                // Zdinx requires D extension
    {zhinx, f},                // Zhinx requires F extension
    {zhinxmin, f},             // Zhinxmin requires F extension
    {zhinx, zhinxmin},         // Zhinx requires Zhinxmin
};

// Basic RISC-V 64 feature sets
constexpr auto generic = get_feature_masks();
constexpr auto rv64i = get_feature_masks(); // Base integer ISA
constexpr auto rv64im = get_feature_masks(m); // Integer + multiply/divide
constexpr auto rv64ima = rv64im | get_feature_masks(a); // + atomic operations
constexpr auto rv64imaf = rv64ima | get_feature_masks(f); // + single precision FP
constexpr auto rv64imafd = rv64imaf | get_feature_masks(d); // + double precision FP
constexpr auto rv64imafdc = rv64imafd | get_feature_masks(c); // + compressed instructions
constexpr auto rv64gc = rv64imafdc; // RV64GC (equivalent to RV64IMAFDC)
constexpr auto rv64gcv = rv64gc | get_feature_masks(v); // + vector extension

// SiFive U74-MC features (based on typical RISC-V 64 implementations)
constexpr auto sifive_u74 = rv64gc | get_feature_masks(zba, zbb, zbs, zicbom, zicbop, zicboz);

// SiFive U84-MC features (enhanced version)
constexpr auto sifive_u84 = sifive_u74 | get_feature_masks(zicond, zawrs, zfa, zfhmin);

// SiFive U87-MC features (further enhanced)
constexpr auto sifive_u87 = sifive_u84 | get_feature_masks(zfh, zicntr, zihpm);

// SiFive U89-MC features (latest features)
constexpr auto sifive_u89 = sifive_u87 | get_feature_masks(zicclsm, zicfilp, zicfiss, zihintntl, zihintpause, zihwa, zimop, ziselect, ztso);

// Generic SiFive U9 series (placeholder for future implementations)
constexpr auto sifive_u9 = sifive_u89;

}

static constexpr CPUSpec<CPU, feature_sz> cpus[] = {
    {"generic", CPU::generic, CPU::generic, 0, Feature::generic},
    {"rv64gc", CPU::rv64gc, CPU::generic, 0, Feature::rv64gc},
    {"rv64gcv", CPU::rv64gcv, CPU::rv64gc, 0, Feature::rv64gcv},
    {"rv64imafdc", CPU::rv64imafdc, CPU::generic, 0, Feature::rv64imafdc},
    {"rv64imafdcv", CPU::rv64imafdcv, CPU::rv64imafdc, 0, Feature::rv64gcv},
    {"sifive-u74-mc", CPU::sifive_u74, CPU::rv64gc, 0, Feature::sifive_u74},
    {"sifive-u84-mc", CPU::sifive_u84, CPU::sifive_u74, 0, Feature::sifive_u84},
    {"sifive-u87-mc", CPU::sifive_u87, CPU::sifive_u84, 0, Feature::sifive_u87},
    {"sifive-u89-mc", CPU::sifive_u89, CPU::sifive_u87, 0, Feature::sifive_u89},
    {"sifive-u9-mc", CPU::sifive_u9, CPU::sifive_u89, 0, Feature::sifive_u9},
};

static constexpr size_t ncpus = sizeof(cpus) / sizeof(CPUSpec<CPU, feature_sz>);

static const CPUSpec<CPU, feature_sz> *find_cpu(llvm::StringRef name)
{
    for (size_t i = 0; i < ncpus; i++) {
        if (name == cpus[i].name) {
            return &cpus[i];
        }
    }
    return nullptr;
}

static const char *find_cpu_name(uint32_t cpu)
{
    for (size_t i = 0; i < ncpus; i++) {
        if (cpu == (uint32_t)cpus[i].cpu) {
            return cpus[i].name;
        }
    }
    return "generic";
}

// RISC-V 64 CPU detection is primarily based on /proc/cpuinfo on Linux
// and sysctl on other systems
static std::pair<uint32_t, FeatureList<feature_sz>> _get_host_cpu()
{
    FeatureList<feature_sz> features = {};

    // Try to detect CPU features from /proc/cpuinfo on Linux
#ifdef _OS_LINUX_
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    std::string cpu_name = "generic";

    while (std::getline(cpuinfo, line)) {
        if (line.substr(0, 10) == "processor\t") {
            // Reset features for each processor (they should be the same)
            features = {};
        }
        else if (line.substr(0, 10) == "hart\t") {
            // RISC-V specific: hart ID
        }
        else if (line.substr(0, 10) == "isa\t") {
            // RISC-V ISA string - this is the most important field
            size_t pos = line.find('\t');
            if (pos != std::string::npos) {
                std::string isa = line.substr(pos + 1);

                // Parse ISA string to detect features
                if (isa.find("rv64") != std::string::npos) {
                    // RV64 base architecture
                }
                if (isa.find("m") != std::string::npos) {
                    set_bit(features, Feature::m, true);
                }
                if (isa.find("a") != std::string::npos) {
                    set_bit(features, Feature::a, true);
                }
                if (isa.find("f") != std::string::npos) {
                    set_bit(features, Feature::f, true);
                }
                if (isa.find("d") != std::string::npos) {
                    set_bit(features, Feature::d, true);
                }
                if (isa.find("c") != std::string::npos) {
                    set_bit(features, Feature::c, true);
                }
                if (isa.find("v") != std::string::npos) {
                    set_bit(features, Feature::v, true);
                }
                if (isa.find("zba") != std::string::npos) {
                    set_bit(features, Feature::zba, true);
                }
                if (isa.find("zbb") != std::string::npos) {
                    set_bit(features, Feature::zbb, true);
                }
                if (isa.find("zbs") != std::string::npos) {
                    set_bit(features, Feature::zbs, true);
                }
                if (isa.find("zicbom") != std::string::npos) {
                    set_bit(features, Feature::zicbom, true);
                }
                if (isa.find("zicbop") != std::string::npos) {
                    set_bit(features, Feature::zicbop, true);
                }
                if (isa.find("zicboz") != std::string::npos) {
                    set_bit(features, Feature::zicboz, true);
                }
                if (isa.find("zicond") != std::string::npos) {
                    set_bit(features, Feature::zicond, true);
                }
                if (isa.find("zawrs") != std::string::npos) {
                    set_bit(features, Feature::zawrs, true);
                }
                if (isa.find("zfa") != std::string::npos) {
                    set_bit(features, Feature::zfa, true);
                }
                if (isa.find("zfh") != std::string::npos) {
                    set_bit(features, Feature::zfh, true);
                }
                if (isa.find("zfhmin") != std::string::npos) {
                    set_bit(features, Feature::zfhmin, true);
                }
                if (isa.find("zfinx") != std::string::npos) {
                    set_bit(features, Feature::zfinx, true);
                }
                if (isa.find("zdinx") != std::string::npos) {
                    set_bit(features, Feature::zdinx, true);
                }
                if (isa.find("zhinx") != std::string::npos) {
                    set_bit(features, Feature::zhinx, true);
                }
                if (isa.find("zhinxmin") != std::string::npos) {
                    set_bit(features, Feature::zhinxmin, true);
                }
                if (isa.find("zicntr") != std::string::npos) {
                    set_bit(features, Feature::zicntr, true);
                }
                if (isa.find("zihpm") != std::string::npos) {
                    set_bit(features, Feature::zihpm, true);
                }
                if (isa.find("zicclsm") != std::string::npos) {
                    set_bit(features, Feature::zicclsm, true);
                }
                if (isa.find("zicfilp") != std::string::npos) {
                    set_bit(features, Feature::zicfilp, true);
                }
                if (isa.find("zicfiss") != std::string::npos) {
                    set_bit(features, Feature::zicfiss, true);
                }
                if (isa.find("zihintntl") != std::string::npos) {
                    set_bit(features, Feature::zihintntl, true);
                }
                if (isa.find("zihintpause") != std::string::npos) {
                    set_bit(features, Feature::zihintpause, true);
                }
                if (isa.find("zihwa") != std::string::npos) {
                    set_bit(features, Feature::zihwa, true);
                }
                if (isa.find("zimop") != std::string::npos) {
                    set_bit(features, Feature::zimop, true);
                }
                if (isa.find("ziselect") != std::string::npos) {
                    set_bit(features, Feature::ziselect, true);
                }
                if (isa.find("ztso") != std::string::npos) {
                    set_bit(features, Feature::ztso, true);
                }
            }
        }
        else if (line.substr(0, 10) == "uarch\t") {
            // RISC-V microarchitecture
            size_t pos = line.find('\t');
            if (pos != std::string::npos) {
                std::string uarch = line.substr(pos + 1);
                if (uarch.find("sifive") != std::string::npos) {
                    if (uarch.find("u74") != std::string::npos) {
                        cpu_name = "sifive-u74-mc";
                    }
                    else if (uarch.find("u84") != std::string::npos) {
                        cpu_name = "sifive-u84-mc";
                    }
                    else if (uarch.find("u87") != std::string::npos) {
                        cpu_name = "sifive-u87-mc";
                    }
                    else if (uarch.find("u89") != std::string::npos) {
                        cpu_name = "sifive-u89-mc";
                    }
                    else if (uarch.find("u9") != std::string::npos) {
                        cpu_name = "sifive-u9-mc";
                    }
                }
            }
        }
    }

    // Find the best matching CPU based on detected features
    uint32_t best_cpu = (uint32_t)CPU::generic;
    size_t best_score = 0;

    for (size_t i = 0; i < ncpus; i++) {
        size_t score = 0;
        for (size_t j = 0; j < feature_sz; j++) {
            score += llvm::popcount(features[j] & cpus[i].features[j]);
        }
        if (score > best_score) {
            best_score = score;
            best_cpu = (uint32_t)cpus[i].cpu;
        }
    }

    // If we found a specific CPU name, use it
    if (cpu_name != "generic") {
        auto spec = find_cpu(cpu_name);
        if (spec) {
            best_cpu = (uint32_t)spec->cpu;
        }
    }

    return std::make_pair(best_cpu, features);
#else
    // For non-Linux systems, fall back to generic
    return std::make_pair((uint32_t)CPU::generic, features);
#endif
}

static inline const std::pair<uint32_t, FeatureList<feature_sz>> &get_host_cpu()
{
    static auto host_cpu = _get_host_cpu();
    return host_cpu;
}

static bool is_generic_cpu_name(uint32_t cpu)
{
    switch ((CPU)cpu) {
    case CPU::generic:
    case CPU::rv64gc:
    case CPU::rv64gcv:
    case CPU::rv64imafdc:
    case CPU::rv64imafdcv:
        return true;
    default:
        return false;
    }
}

static inline const std::string &host_cpu_name()
{
    static std::string name = [] {
        if (is_generic_cpu_name(get_host_cpu().first)) {
            auto llvm_name = jl_get_cpu_name_llvm();
            if (llvm_name != "generic") {
                return llvm_name;
            }
        }
        return std::string(find_cpu_name(get_host_cpu().first));
    }();
    return name;
}

static inline const char *normalize_cpu_name(llvm::StringRef name)
{
    // Normalize common RISC-V CPU names
    if (name == "rv64gc")
        return "rv64gc";
    if (name == "rv64gcv")
        return "rv64gcv";
    if (name == "rv64imafdc")
        return "rv64imafdc";
    if (name == "rv64imafdcv")
        return "rv64imafdcv";
    return nullptr;
}

template<size_t n>
static inline void enable_depends(FeatureList<n> &features)
{
    // Enable feature dependencies
    ::enable_depends(features, Feature::deps, sizeof(Feature::deps) / sizeof(FeatureDep));
}

template<size_t n>
static inline void disable_depends(FeatureList<n> &features)
{
    // Disable feature dependencies
    ::disable_depends(features, Feature::deps, sizeof(Feature::deps) / sizeof(FeatureDep));
}

static const llvm::SmallVector<TargetData<feature_sz>, 0> &get_cmdline_targets(const char *cpu_target)
{
    auto feature_cb = [] (const char *str, size_t len, FeatureList<feature_sz> &list) {
        auto fbit = find_feature_bit(feature_names, nfeature_names, str, len);
        if (fbit == UINT32_MAX)
            return false;
        set_bit(list, fbit, true);
        return true;
    };
    auto &targets = ::get_cmdline_targets<feature_sz>(cpu_target, feature_cb);
    for (auto &t: targets) {
        if (auto nname = normalize_cpu_name(t.name)) {
            t.name = nname;
        }
    }
    return targets;
}

static llvm::SmallVector<TargetData<feature_sz>, 0> jit_targets;

static TargetData<feature_sz> arg_target_data(const TargetData<feature_sz> &arg, bool require_host)
{
    TargetData<feature_sz> res = arg;
    const FeatureList<feature_sz> *cpu_features = nullptr;
    if (res.name == "native") {
        res.name = host_cpu_name();
        cpu_features = &get_host_cpu().second;
    }
    else if (auto spec = find_cpu(res.name)) {
        cpu_features = &spec->features;
    }
    else {
        res.en.flags |= JL_TARGET_UNKNOWN_NAME;
    }
    if (cpu_features) {
        res.en.features = *cpu_features;
        enable_depends(res.en.features);
    }
    append_ext_features(res.ext_features, jl_get_cpu_features_llvm());
    return res;
}

static uint32_t sysimg_init_cb(void *ctx, const void *id, jl_value_t **rejection_reason)
{
    const char *cpu_target = (const char *)ctx;
    auto &cmdline = get_cmdline_targets(cpu_target);
    TargetData<feature_sz> target = arg_target_data(cmdline[0], true);
    uint32_t best_idx = 0;
    auto sysimg = deserialize_target_data<feature_sz>((const uint8_t*)id);
    for (uint32_t i = 0; i < sysimg.size(); i++) {
        auto &imgt = sysimg[i];
        if (imgt.name == target.name) {
            best_idx = i;
        }
    }
    jit_targets.push_back(std::move(target));
    return best_idx;
}

static uint32_t pkgimg_init_cb(void *ctx, const void *id, jl_value_t **rejection_reason)
{
    TargetData<feature_sz> target = jit_targets.front();
    uint32_t best_idx = 0;
    auto pkgimg = deserialize_target_data<feature_sz>((const uint8_t*)id);
    for (uint32_t i = 0; i < pkgimg.size(); i++) {
        auto &imgt = pkgimg[i];
        if (imgt.name == target.name) {
            best_idx = i;
        }
    }
    return best_idx;
}

static void ensure_jit_target(const char *cpu_target, bool imaging)
{
    auto &cmdline = get_cmdline_targets(cpu_target);
    check_cmdline(cmdline, imaging);
    if (!jit_targets.empty())
        return;
    for (auto &arg: cmdline) {
        auto data = arg_target_data(arg, jit_targets.empty());
        jit_targets.push_back(std::move(data));
    }
    auto ntargets = jit_targets.size();
    for (size_t i = 1; i < ntargets; i++) {
        auto &t = jit_targets[i];
        t.en.flags |= JL_TARGET_CLONE_ALL;
    }
}

static std::pair<std::string,llvm::SmallVector<std::string, 0>>
get_llvm_target_noext(const TargetData<feature_sz> &data)
{
    return std::make_pair(data.name, llvm::SmallVector<std::string, 0>{});
}

static std::pair<std::string,llvm::SmallVector<std::string, 0>>
get_llvm_target_vec(const TargetData<feature_sz> &data)
{
    auto res0 = get_llvm_target_noext(data);
    append_ext_features(res0.second, data.ext_features);
    return res0;
}

static std::pair<std::string,std::string>
get_llvm_target_str(const TargetData<feature_sz> &data)
{
    auto res0 = get_llvm_target_noext(data);
    auto features = join_feature_strs(res0.second);
    append_ext_features(features, data.ext_features);
    return std::make_pair(std::move(res0.first), std::move(features));
}

}

using namespace RISCV64;

jl_image_t jl_init_processor_sysimg(jl_image_buf_t image, const char *cpu_target)
{
    if (!jit_targets.empty())
        jl_error("JIT targets already initialized");
    return parse_sysimg(image, sysimg_init_cb, (void *)cpu_target);
}

jl_image_t jl_init_processor_pkgimg(jl_image_buf_t image)
{
    if (jit_targets.empty())
        jl_error("JIT targets not initialized");
    if (jit_targets.size() > 1)
        jl_error("Expected only one JIT target");
    return parse_sysimg(image, pkgimg_init_cb, NULL);
}

std::pair<std::string,llvm::SmallVector<std::string, 0>> jl_get_llvm_target(const char *cpu_target, bool imaging, uint32_t &flags)
{
    ensure_jit_target(cpu_target, imaging);
    flags = jit_targets[0].en.flags;
    return get_llvm_target_vec(jit_targets[0]);
}

const std::pair<std::string,std::string> &jl_get_llvm_disasm_target(void)
{
    static const auto res = get_llvm_target_str(TargetData<feature_sz>{host_cpu_name(),
                jl_get_cpu_features_llvm(), {{}, 0}, {{}, 0}, 0});
    return res;
}

#ifndef __clang_gcanalyzer__
llvm::SmallVector<jl_target_spec_t, 0> jl_get_llvm_clone_targets(const char *cpu_target)
{
    auto &cmdline = get_cmdline_targets(cpu_target);
    check_cmdline(cmdline, true);
    llvm::SmallVector<TargetData<feature_sz>, 0> image_targets;
    for (auto &arg: cmdline) {
        auto data = arg_target_data(arg, image_targets.empty());
        image_targets.push_back(std::move(data));
    }
    auto ntargets = image_targets.size();
    for (size_t i = 1; i < ntargets; i++) {
        auto &t = image_targets[i];
        t.en.flags |= JL_TARGET_CLONE_ALL;
    }
    if (image_targets.empty())
        jl_error("No image targets found");
    llvm::SmallVector<jl_target_spec_t, 0> res;
    for (auto &target: image_targets) {
        jl_target_spec_t ele;
        std::tie(ele.cpu_name, ele.cpu_features) = get_llvm_target_str(target);
        ele.data = serialize_target_data(target.name, target.en.features,
                                         target.dis.features, target.ext_features);
        ele.flags = target.en.flags;
        ele.base = 0;
        res.push_back(ele);
    }
    return res;
}
#endif

JL_DLLEXPORT jl_value_t *jl_cpu_has_fma(int bits)
{
    // RISC-V 64 has FMA instructions when the F and D extensions are available
    // and the Zfa extension is present
    auto &host = get_host_cpu();
    if (bits == 32) {
        return jl_box_bool(test_nbit(host.second, Feature::f) && test_nbit(host.second, Feature::zfa));
    }
    else if (bits == 64) {
        return jl_box_bool(test_nbit(host.second, Feature::d) && test_nbit(host.second, Feature::zfa));
    }
    return jl_false;
}

JL_DLLEXPORT void jl_dump_host_cpu(void)
{
    jl_safe_printf("CPU: %s\n", host_cpu_name().c_str());
    jl_safe_printf("Features: %s\n", jl_get_cpu_features_llvm().c_str());
}

JL_DLLEXPORT jl_value_t* jl_check_pkgimage_clones(char *data)
{
    jl_value_t *rejection_reason = NULL;
    JL_GC_PUSH1(&rejection_reason);
    uint32_t match_idx = pkgimg_init_cb(NULL, data, &rejection_reason);
    JL_GC_POP();
    if (match_idx == UINT32_MAX)
        return rejection_reason;
    return jl_nothing;
}

extern "C" int jl_test_cpu_feature(jl_cpu_feature_t feature)
{
    auto &host = get_host_cpu();
    return test_nbit(host.second, feature);
}

extern "C" JL_DLLEXPORT int32_t jl_get_zero_subnormals(void)
{
    // RISC-V 64 doesn't have a specific flag for this, return 0
    return 0;
}

extern "C" JL_DLLEXPORT int32_t jl_set_zero_subnormals(int8_t isZero)
{
    // RISC-V 64 doesn't support setting this, just return the input
    return isZero;
}

extern "C" JL_DLLEXPORT int32_t jl_get_default_nans(void)
{
    // RISC-V 64 doesn't have a specific flag for this, return 0
    return 0;
}

extern "C" JL_DLLEXPORT int32_t jl_set_default_nans(int8_t isDefault)
{
    // RISC-V 64 doesn't support setting this, just return the input
    return isDefault;
}
