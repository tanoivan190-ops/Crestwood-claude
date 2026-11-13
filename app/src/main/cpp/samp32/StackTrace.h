//
// Created by x1y2z on 20.03.2023.
//

#ifndef LIVERUSSIA_CSTACKTRACE_H
#define LIVERUSSIA_CSTACKTRACE_H

#include <stdio.h>
#include <dlfcn.h>
#include <dlfcn.h>
#include <unwind.h>
#include "../main.h"

#if VER_x32
#define PRINT_CRASH_STATES(context) \
	FLog("register states:"); \
	FLog("r0: 0x%X, r1: 0x%X, r2: 0x%X, r3: 0x%X", (context)->uc_mcontext.arm_r0, (context)->uc_mcontext.arm_r1, (context)->uc_mcontext.arm_r2, (context)->uc_mcontext.arm_r3); \
	FLog("r4: 0x%x, r5: 0x%x, r6: 0x%x, r7: 0x%x", (context)->uc_mcontext.arm_r4, (context)->uc_mcontext.arm_r5, (context)->uc_mcontext.arm_r6, (context)->uc_mcontext.arm_r7); \
	FLog("r8: 0x%x, r9: 0x%x, sl: 0x%x, fp: 0x%x", (context)->uc_mcontext.arm_r8, (context)->uc_mcontext.arm_r9, (context)->uc_mcontext.arm_r10, (context)->uc_mcontext.arm_fp); \
	FLog("ip: 0x%x, sp: 0x%x, lr: 0x%x, pc: 0x%x", (context)->uc_mcontext.arm_ip, (context)->uc_mcontext.arm_sp, (context)->uc_mcontext.arm_lr, (context)->uc_mcontext.arm_pc); \
    FLog("1: libGTASA.so + 0x%X", context->uc_mcontext.arm_pc - g_libGTASA); \
    FLog("2: libGTASA.so + 0x%X", context->uc_mcontext.arm_lr - g_libGTASA); \
    FLog("1: libsamp.so + 0x%X", context->uc_mcontext.arm_pc - g_libSAMP); \
    FLog("2: libsamp.so + 0x%X", context->uc_mcontext.arm_lr - g_libSAMP); \
    LogCrashAddr("pc", (uintptr_t)(context->uc_mcontext.arm_pc)); \
    LogCrashAddr("lr", (uintptr_t)(context->uc_mcontext.arm_lr));
#else
#define PRINT_CRASH_STATES(context) \
    FLog("1: libGTASA.so + 0x%llx", context->uc_mcontext.pc - g_libGTASA); \
    FLog("2: libGTASA.so + 0x%llx", context->uc_mcontext.regs[30] - g_libGTASA); \
    FLog("1: libsamp.so + 0x%llx", context->uc_mcontext.pc - g_libSAMP); \
    FLog("2: libsamp.so + 0x%llx", context->uc_mcontext.regs[30] - g_libSAMP); \
    LogCrashAddr("pc", (uintptr_t)(context->uc_mcontext.pc)); \
    LogCrashAddr("lr", (uintptr_t)(context->uc_mcontext.regs[30]));
#endif

void FLog(const char* fmt, ...);

// Identifie la bibliothèque qui contient une adresse (le crash peut être hors de libGame/libsamp :
// pilote GPU, WebView, libc...). Utilise dladdr pour donner le nom du .so et l'offset dedans.
static inline void LogCrashAddr(const char* tag, uintptr_t addr)
{
    Dl_info info;
    if (addr && dladdr(reinterpret_cast<void*>(addr), &info) && info.dli_fname)
    {
        FLog("%s %p -> %s + 0x%llx %s", tag, reinterpret_cast<void*>(addr), info.dli_fname,
            (unsigned long long)(addr - reinterpret_cast<uintptr_t>(info.dli_fbase)),
            info.dli_sname ? info.dli_sname : "");
    }
    else
    {
        FLog("%s %p -> module inconnu", tag, reinterpret_cast<void*>(addr));
    }
}
extern int g_iLastRenderedObject;
class CStackTrace
{
public:
    static void printBacktrace()
    {
        FLog("Last rendered object: %d", g_iLastRenderedObject);
        FLog("------------ START BACKTRACE ------------");
        FLog(" ");
        PrintStackTrace();
    }

private:
    static _Unwind_Reason_Code TraceFunction(_Unwind_Context* context, void* arg) {
        uintptr_t pc = _Unwind_GetIP(context);

        Dl_info info;
        if (dladdr(reinterpret_cast<void*>(pc), &info) && info.dli_sname != nullptr) {
            FLog("[adr: %p samp: %p gta: %p] %s\n",
                     reinterpret_cast<void*>(pc),
                     reinterpret_cast<void*>(pc - g_libSAMP),
                     reinterpret_cast<void*>(pc - g_libGTASA),
                     info.dli_sname);
        } else {
            FLog("[adr: %p samp: %p gta: %p] name not found\n",
                     reinterpret_cast<void*>(pc),
                     reinterpret_cast<void*>(pc - g_libSAMP),
                     reinterpret_cast<void*>(pc - g_libGTASA));
        }

        return _URC_NO_REASON;
    }

    static void PrintStackTrace() {
        _Unwind_Backtrace(TraceFunction, nullptr);
    }

};

#endif //LIVERUSSIA_CSTACKTRACE_H
