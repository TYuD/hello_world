#include "serialization_2.h"

void main()
{
  #if  CHAR_BIT == 8

    const size_t N = 6;

    char src[N] = { '\x00','\xA0','\xBC','\x0D','\x00','\x00' };
    char dst[N] = { '\xFF','\xFF','\xFF','\xFF','\xFF','\xFF' };
    char res[N] = { '\xAF','\xBC','\xFD','\xFF','\xFF','\xFF' };

    const size_t dst_offset_bits = 4 ;
    const size_t length_bits     = 16;
    const size_t src_offset_bits = 12;

  #elif CHAR_BIT == 32

    const size_t N = 4;

    char src[N] = { '\x00000000','\xA0000000','\xBCDE1234','\x00000000' };
    char dst[N] = { '\xFFFFFFFF','\xFFFFFFFF','\xFFFFFFFF','\xFFFFFFFF' };
    char res[N] = { '\xDE1234AF','\xFFFFFFBC','\xFFFFFFFF','\xFFFFFFFF' };

    const size_t dst_offset_bits = 4 ;
    const size_t length_bits     = 36;
    const size_t src_offset_bits = 60;

  #else
    #error Strange CHAR_BIT value
  #endif

  nunavutCopyBits(dst, dst_offset_bits, length_bits, src, src_offset_bits);
  bool Ok = true;
  for( unsigned i=0; i < N; i++ )
    if( dst[i] != res[i] )
      Ok = false;
}
