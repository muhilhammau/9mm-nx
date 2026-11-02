/**
 * @file exception32.h
 * @brief AArch32 user-mode exception handling (libnx32).
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

#ifndef __ARM_ARCH_ISA_A64

/// What the kernel saved for the faulting thread. svcReturnFromException(0)
/// resumes it with these values, so a handler may change them (pc, sp, r0-r7...).
typedef struct {
    u32 r[8];        ///< r0-r7
    u32 sp;
    u32 lr;
    u32 pc;
    u32 flags;
    u32 pstate;      ///< CPSR (bit 5: Thumb)
    u32 afsr0;
    u32 afsr1;
    u32 esr;
    u32 far;         ///< Fault address
} ThreadExceptionInfo32;

/// What __libnx_exception_entry saved (the kernel does not): restored before
/// svcReturnFromException, so a handler may change these too.
typedef struct {
    u32 fpscr;
    u32 pad;
    u64 d16_31[16];
    u64 d0_15[16];
    u32 r8_12[5];    ///< r8-r12
    u32 lr_entry;
} ThreadExceptionFrame32;

/**
 * Exception types (r0 at entry). The type is coarse: Mesosphère reports an
 * undefined instruction, a trapped coprocessor access (MRC/MCR, MRRC/MCRR),
 * an illegal execution state and a BKPT as InstructionAbort too, the same as a
 * failed instruction fetch, and it never reports UndefinedInstruction. A
 * handler tells them apart with the syndrome's exception class
 * (\ref threadExceptionClass32).
 */
enum {
    ThreadExceptionType32_InstructionAbort       = 0x100,
    ThreadExceptionType32_DataAbort              = 0x101,
    ThreadExceptionType32_UnalignedInstruction   = 0x102,
    ThreadExceptionType32_UnalignedData          = 0x103,
    ThreadExceptionType32_UndefinedInstruction   = 0x104,
    ThreadExceptionType32_ExceptionInstruction   = 0x105,
    ThreadExceptionType32_MemorySystemError      = 0x106,
    ThreadExceptionType32_FpuException           = 0x200,
};

/// Exception classes (ESR bits 31-26, \ref threadExceptionClass32) an AArch32
/// thread can raise.
enum {
    ThreadExceptionClass32_Unknown           = 0x00, ///< Undefined instruction
    ThreadExceptionClass32_WfiWfe            = 0x01,
    ThreadExceptionClass32_Cp15McrMrc        = 0x03, ///< Trapped CP15 MCR/MRC
    ThreadExceptionClass32_Cp15McrrMrrc      = 0x04, ///< Trapped CP15 MCRR/MRRC
    ThreadExceptionClass32_Cp14McrMrc        = 0x05,
    ThreadExceptionClass32_Cp14LdcStc        = 0x06,
    ThreadExceptionClass32_FpAccess          = 0x07, ///< Trapped VFP/NEON access (reported as DataAbort)
    ThreadExceptionClass32_Cp14Mrrc          = 0x0C,
    ThreadExceptionClass32_IllegalExecution  = 0x0E,
    ThreadExceptionClass32_Svc               = 0x11,
    ThreadExceptionClass32_InstructionAbort  = 0x20, ///< Failed instruction fetch
    ThreadExceptionClass32_PcAlignment       = 0x22,
    ThreadExceptionClass32_DataAbort         = 0x24,
    ThreadExceptionClass32_SpAlignment       = 0x26,
    ThreadExceptionClass32_FpException       = 0x28,
    ThreadExceptionClass32_SError            = 0x2F,
    ThreadExceptionClass32_Bkpt              = 0x38,
};

/// The exception class of a fault (ESR bits 31-26).
static inline u32 threadExceptionClass32(const ThreadExceptionInfo32* info)
{
    return info->esr >> 26;
}

/**
 * @brief Optional program-defined exception handler, called on a 32 KiB
 * exception stack. Return 0 to resume the thread (with any changes made to
 * \p info and \p frame), or a failure to let the kernel end the process.
 * Without one, every fault ends the process.
 * @note Runs in exception context: avoid locks the faulting thread may hold
 * (newlib's stdio and malloc included).
 */
Result __libnx_exception_handler32(u32 type, ThreadExceptionInfo32* info, ThreadExceptionFrame32* frame);

#endif
