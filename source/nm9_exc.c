/* nm9_exc.c -- SWP / SWPB, which this CPU does not run.
 *
 * libCopStory.so's own lock and refcount helpers (31 sites, all in
 * 0x7404bc-0x7410f4) use the ARMv5 swap instruction. On the Switch (ARMv8
 * AArch32, SWP disabled) it faults: crash.log showed insn e1053096 =
 * swp r3, r6, [r5], r6 = 1 (a spinlock), esr 02000000.
 *
 * The runtime offers port_exception_hook: return 1 and the thread resumes
 * with the changed registers. SWP is done here with an atomic exchange
 * (ldrex/strex), the result goes to Rt, and pc moves past it. Exception
 * context: no locks, no allocation, nothing but registers and one exchange.
 * Every fault that is not a plain SWP/SWPB goes on to the crash report. MIT.
 */
#include <stdint.h>

#include "exc_handler.h"

int port_exception_hook(uint32_t type, ThreadExceptionInfo32 *info, ThreadExceptionFrame32 *frame) {
  (void)type;
  if (info->pstate & 0x20) /* Thumb: SWP is an ARM-mode instruction */
    return 0;
  const uint32_t pc = info->pc;
  if (!dcr_is_code_addr(pc) || dcr_readable(pc, 4) < 4)
    return 0;
  const uint32_t w = *(const volatile uint32_t *)pc;
  /* cond 0001 0B00 Rn Rt 0000 1001 Rt2; only "always" is handled (the
   * library has no other) */
  if ((w & 0x0FB00FF0u) != 0x01000090u || (w >> 28) != 0xE)
    return 0;
  const unsigned rn = (w >> 16) & 15, rt = (w >> 12) & 15, rt2 = w & 15;
  if (rn == 15 || rt == 15 || rt2 == 15)
    return 0;
  const uint32_t addr = *rt_exc_reg(info, frame, rn);
  const uint32_t val = *rt_exc_reg(info, frame, rt2);
  uint32_t old;
  if (w & 0x00400000u) { /* SWPB */
    if (dcr_readable(addr, 1) < 1)
      return 0;
    old = __atomic_exchange_n((volatile uint8_t *)(uintptr_t)addr, (uint8_t)val, __ATOMIC_SEQ_CST);
  } else {
    if ((addr & 3) || dcr_readable(addr, 4) < 4)
      return 0;
    old = __atomic_exchange_n((volatile uint32_t *)(uintptr_t)addr, val, __ATOMIC_SEQ_CST);
  }
  *rt_exc_reg(info, frame, rt) = old;
  info->pc = pc + 4;
  return 1;
}