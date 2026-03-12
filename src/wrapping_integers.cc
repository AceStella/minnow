#include "wrapping_integers.hh"
#include "debug.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return zero_point + static_cast<uint32_t>( n );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  const uint32_t abs_low = this->raw_value_ - zero_point.raw_value_;
  const auto ckpt_low = static_cast<uint32_t>( checkpoint );

  const uint32_t right_gap = abs_low - ckpt_low;
  const uint32_t left_gap = ckpt_low - abs_low;

  if ( right_gap <= left_gap || checkpoint < left_gap ) {
    return checkpoint + right_gap;
  }
  return checkpoint - left_gap;
}
