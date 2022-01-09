#ifndef SERIALIZATION_2_H
#define SERIALIZATION_2_H

#include <string.h>
#include <limits.h>

static inline size_t nunavutChooseMin(const size_t a, const size_t b)
{
    return (a < b) ? a : b;
}
// ----------------------------------------------------
#if CHAR_BIT == 8
  #define CHAR_SHIFT 3
#elif CHAR_BIT == 16
  #define CHAR_SHIFT 4
#elif CHAR_BIT == 32
  #define CHAR_SHIFT 5
#elif CHAR_BIT == 64
  #define CHAR_SHIFT 6
#else
  #error Strange CHAR_BIT value
#endif

#define CHAR_SHIFT_MASK  ((1u << CHAR_SHIFT) - 1u)
// ----------------------------------------------------
static inline void nunavutCopyBits(void* const dst, const size_t dst_offset_bits, const size_t length_bits, const void* const src, const size_t src_offset_bits)
{
    unsigned ch_shift = CHAR_SHIFT;
    unsigned ch_mask  = UCHAR_MAX ;
    if (0 == (src_offset_bits & CHAR_SHIFT_MASK) && 0 == (dst_offset_bits & CHAR_SHIFT_MASK))  // Aligned copy, optimized, most common case.
    {
        const size_t length_chars = size_t(length_bits >> CHAR_SHIFT);
        const char* const psrc    = (src_offset_bits >> CHAR_SHIFT) + (const char*)src;
        char*       const pdst    = (dst_offset_bits >> CHAR_SHIFT) + (      char*)dst;
        memmove(pdst, psrc, length_chars);
        const unsigned length_mod = length_bits & CHAR_SHIFT_MASK;
        if (length_mod == 0)
            return;
        const char* const last_src = psrc + length_chars;
        char* const       last_dst = pdst + length_chars;
        const unsigned    mask     = (1u << length_mod) - 1;
        *last_dst = (*last_dst & unsigned(~mask)) | (*last_src & mask);
        return;
    }
    const char* const psrc     = (const char*)src;
    char*       const pdst     = (      char*)dst;
    size_t            src_off  = src_offset_bits;
    size_t            dst_off  = dst_offset_bits;
    const size_t      last_bit = src_off + length_bits;
    while (last_bit > src_off)
    {
        const unsigned src_mod = src_off & CHAR_SHIFT_MASK;
        const unsigned dst_mod = dst_off & CHAR_SHIFT_MASK;
        const unsigned max_mod = src_mod > dst_mod ? src_mod : dst_mod;
        const unsigned size    = nunavutChooseMin(CHAR_BIT - max_mod, last_bit - src_off);
        const unsigned mask    = (((1u << size) - 1) << dst_mod) & UCHAR_MAX;
        const unsigned in      = (unsigned(psrc[src_off >> CHAR_SHIFT] >> src_mod) << dst_mod) & UCHAR_MAX;
        const unsigned a       = pdst[dst_off >> CHAR_SHIFT] & (~mask);
        const unsigned b       = in & mask;
        pdst[dst_off >> CHAR_SHIFT] = a | b;
        src_off += size;
        dst_off += size;
    }
}
// ----------------------------------------------------
#endif // SERIALIZATION_2_H
