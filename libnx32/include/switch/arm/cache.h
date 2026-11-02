/**
 * @file cache.h
 * @brief AArch64 cache operations.
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/**
 * @brief Performs a data cache flush on the specified buffer.
 * @param addr Address of the buffer.
 * @param size Size of the buffer, in bytes.
 * @remarks Cache flush is defined as Clean + Invalidate.
 * @note The start and end addresses of the buffer are forcibly rounded to cache line boundaries (read from CTR_EL0 system register).
 */
void armDCacheFlush(void* addr, size_t size);

/**
 * @brief Performs a data cache clean on the specified buffer.
 * @param addr Address of the buffer.
 * @param size Size of the buffer, in bytes.
 * @note The start and end addresses of the buffer are forcibly rounded to cache line boundaries (read from CTR_EL0 system register).
 */
void armDCacheClean(void* addr, size_t size);

/**
 * @brief Performs an instruction cache invalidation clean on the specified buffer.
 * @param addr Address of the buffer.
 * @param size Size of the buffer, in bytes.
 * @note The start and end addresses of the buffer are forcibly rounded to cache line boundaries (read from CTR_EL0 system register).
 * @note AArch32: cleans the range from the data cache, then invalidates the whole instruction cache of every core (see source/arm/cache32.c).
 */
void armICacheInvalidate(void* addr, size_t size);

/**
 * @brief Performs a data cache zeroing operation on the specified buffer.
 * @param addr Address of the buffer.
 * @param size Size of the buffer, in bytes.
 * @note The start and end addresses of the buffer are forcibly rounded to cache line boundaries (read from CTR_EL0 system register).
 */
void armDCacheZero(void* addr, size_t size);

#ifndef __ARM_ARCH_ISA_A64
// AArch32 (libnx32): EL0 has no data cache maintenance instructions, so the
// data cache functions are the kernel's (svcFlushProcessDataCache and
// svcStoreProcessDataCache on this process), and armICacheInvalidate has the
// kernel invalidate every core's instruction cache (source/arm/cache32.c).
// They are functions, as on AArch64, so they can be declared and called alike.
#endif
