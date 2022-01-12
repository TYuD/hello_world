#ifndef SERIALIZATION_2_H
#define SERIALIZATION_2_H
//-----------------------------------------------------------------------------
#include <string.h>
#include <limits.h>
//-----------------------------------------------------------------------------
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
#define NUNAVUT_SUCCESS 0
//-----------------------------------------------------------------------------
// API usage errors:
#define NUNAVUT_ERROR_INVALID_ARGUMENT                  2
#define NUNAVUT_ERROR_SERIALIZATION_BUFFER_TOO_SMALL    3
// Invalid representation (caused by bad input data, not API misuse):
#define NUNAVUT_ERROR_REPRESENTATION_BAD_ARRAY_LENGTH        10
#define NUNAVUT_ERROR_REPRESENTATION_BAD_UNION_TAG           11
#define NUNAVUT_ERROR_REPRESENTATION_BAD_DELIMITER_HEADER    12
//-----------------------------------------------------------------------------
static inline size_t nunavutChooseMin(const size_t a, const size_t b)
{
    return (a < b) ? a : b;
}
static inline size_t nunavutSaturateBufferFragmentBitLength(const size_t buf_size_chars, const size_t fragment_offset_bits, const size_t fragment_length_bits)
{
    const size_t size_bits = size_t(buf_size_chars * CHAR_BIT);
    const size_t tail_bits = size_bits - nunavutChooseMin(size_bits, fragment_offset_bits);
    return nunavutChooseMin(fragment_length_bits, tail_bits);
}
//-----------------------------------------------------------------------------
static inline void nunavutCopyBits(void* const dst, const size_t dst_offset_bits, const size_t length_bits, const void* const src, const size_t src_offset_bits)
{
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
//-----------------------------------------------------------------------------
static inline int nunavutSetBit( void* const buf, const size_t buf_size_chars, const size_t off_bits, const bool value )
{
    if (buf_size_chars * CHAR_BIT <= off_bits)
        return -NUNAVUT_ERROR_SERIALIZATION_BUFFER_TOO_SMALL;
    const unsigned val = value ? 1U : 0U;
    nunavutCopyBits(buf, off_bits, 1U, &val, 0U);
    return NUNAVUT_SUCCESS;
}
//-----------------------------------------------------------------------------
static inline int nunavutSetUxx( void* const buf, const size_t buf_size_chars, const size_t off_bits, const uint64_t value, const size_t len_bits )
{
    if (buf_size_chars * CHAR_BIT < off_bits + len_bits)
        return -NUNAVUT_ERROR_SERIALIZATION_BUFFER_TOO_SMALL;
    const size_t saturated_len_bits = nunavutChooseMin(len_bits,64U);
    nunavutCopyBits(buf, off_bits, saturated_len_bits, &value, 0U);
    return NUNAVUT_SUCCESS;
}
//-----------------------------------------------------------------------------
static inline int nunavutSetIxx( void* const buf, const size_t buf_size_chars, const size_t off_bits, const int64_t value, const size_t len_bits)
{
    return nunavutSetUxx(buf, buf_size_chars, off_bits, uint64_t(value), len_bits);
}
//-----------------------------------------------------------------------------
static inline uint_fast8_t nunavutGetU8(const void* const buf, const size_t buf_size_chars, const size_t off_bits, const size_t len_bits)
{
    const size_t bits = nunavutSaturateBufferFragmentBitLength(buf_size_chars, off_bits, nunavutChooseMin(len_bits,8U));
    uint_fast8_t val = 0U;
    nunavutCopyBits(&val, 0U, bits, buf, off_bits);
    return val;
}
//-----------------------------------------------------------------------------
static inline bool nunavutGetBit(const void* const buf, const size_t buf_size_chars, const size_t off_bits)
{
    return 1U == nunavutGetU8(buf, buf_size_chars, off_bits, 1U);
}
//-----------------------------------------------------------------------------
static inline uint_fast16_t nunavutGetU16(const void* const buf, const size_t buf_size_chars, const size_t off_bits, const size_t len_bits)
{
    const size_t bits = nunavutSaturateBufferFragmentBitLength(buf_size_chars, off_bits, nunavutChooseMin(len_bits,16U));
    uint_fast16_t val = 0U;
    nunavutCopyBits(&val, 0U, bits, buf, off_bits);
    return val;
}
//-----------------------------------------------------------------------------
static inline uint_fast32_t nunavutGetU32(const void* const buf, const size_t buf_size_chars, const size_t off_bits, const size_t len_bits)
{
    const size_t bits = nunavutSaturateBufferFragmentBitLength(buf_size_chars, off_bits, nunavutChooseMin(len_bits,32U));
    uint_fast32_t val = 0U;
    nunavutCopyBits(&val, 0U, bits, buf, off_bits);
    return val;
}
//-----------------------------------------------------------------------------
static inline uint64_t nunavutGetU64(const void* const buf, const size_t buf_size_chars, const size_t off_bits, const size_t len_bits)
{
    const size_t bits = nunavutSaturateBufferFragmentBitLength(buf_size_chars, off_bits, nunavutChooseMin(len_bits,64U));
    uint64_t val = 0U;
    nunavutCopyBits(&val, 0U, bits, buf, off_bits);
    return val;
}
//-----------------------------------------------------------------------------
static inline int nunavutSetF32(void* const buf, const size_t buf_size_chars, const size_t off_bits, const float value)
{
    union
    {
        float fl;
        uint_fast32_t in;
    } tmp;
    tmp.fl = value;
    return nunavutSetUxx(buf, buf_size_chars, off_bits, tmp.in, 32U);
}
//-----------------------------------------------------------------------------
static inline float nunavutGetF32( const void* const buf, const size_t buf_size_chars, const size_t off_bits)
{
    union
    {
        uint_fast32_t in;
        float fl;
    } tmp;
    tmp.in = nunavutGetU32(buf, buf_size_chars, off_bits, 32U);
    return tmp.fl;
}
//-----------------------------------------------------------------------------
static inline int nunavutSetF64(void* const buf, const size_t buf_size_chars, const size_t off_bits, const double value)
{
    union
    {
        double fl;
        uint64_t in;
    } tmp;
    tmp.fl = value;
    return nunavutSetUxx(buf, buf_size_chars, off_bits, tmp.in, 64U);
}
//-----------------------------------------------------------------------------
static inline double nunavutGetF64( const void* const buf, const size_t buf_size_chars, const size_t off_bits)
{
    union
    {
        uint64_t in;
        double fl;
    } tmp;
    tmp.in = nunavutGetU64(buf, buf_size_chars, off_bits, 64U);
    return tmp.fl;
}
//-----------------------------------------------------------------------------
static inline uint_fast16_t nunavutFloat16Pack(const float value)
{
    typedef union
    {
        uint_fast32_t bits;
        float real;
    } Float32Bits;
    const uint_fast32_t round_mask = ~uint_fast32_t(0x0FFFU);
    Float32Bits f32inf;
    Float32Bits f16inf;
    Float32Bits magic;
    Float32Bits in;
    f32inf.bits = uint_fast32_t(255U) << 23U;
    f16inf.bits = uint_fast32_t( 31U) << 23U;
    magic.bits  = uint_fast32_t( 15U) << 23U;
    in.real = value;
    const uint_fast32_t sign = in.bits & (uint_fast32_t(1U) << 31U);
    in.bits ^= sign;
    uint_fast16_t out = 0;
    if (in.bits >= f32inf.bits)
        out = (in.bits & 0x7FFFFFUL) ? 0x7E00U : in.bits > f32inf.bits ? 0x7FFFU : 0x7C00U;
    else
    {
        in.bits &= round_mask;
        in.real *= magic.real;
        in.bits -= round_mask;
        if (in.bits > f16inf.bits)
            in.bits = f16inf.bits;
        out = uint_fast16_t(in.bits >> 13U);
    }
    out |= uint_fast16_t(sign >> 16U);
    return out;
}
//-----------------------------------------------------------------------------
static inline float nunavutFloat16Unpack(const uint_fast16_t value)
{
    typedef union
    {
        uint_fast32_t bits;
        float real;
    } Float32Bits;
    Float32Bits magic;
    Float32Bits inf_nan;
    Float32Bits out;
    magic.bits   = uint_fast32_t(0xEFU) << 23U;
    inf_nan.bits = uint_fast32_t(0x8FU) << 23U;
    out.bits     = uint_fast32_t(value & 0x7FFFU) << 13U;
    out.real    *= magic.real;
    if (out.real >= inf_nan.real)
        out.bits |= uint_fast32_t(0xFFU) << 23U;
    out.bits |= uint_fast32_t(value & 0x8000U) << 16U;
    return out.real;
}
//-----------------------------------------------------------------------------
static inline int nunavutSetF16( void* const buf, const size_t buf_size_chars, const size_t off_bits, const float value)
{
    return nunavutSetUxx(buf, buf_size_chars, off_bits, nunavutFloat16Pack(value),16U);
}
//-----------------------------------------------------------------------------
static inline float nunavutGetF16( const void* const buf, const size_t buf_size_chars, const size_t off_bits)
{
    return nunavutFloat16Unpack(nunavutGetU16(buf, buf_size_chars, off_bits, 16U));
}
//-----------------------------------------------------------------------------
#endif // SERIALIZATION_2_H
