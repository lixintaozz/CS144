#include "wrapping_integers.hh"

using namespace std;


uint64_t distance(uint64_t a, uint64_t b) {
  return (a > b) ? (a - b) : (b - a);
}


Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  //将64位的绝对序号转换为32位的相对序号
  return zero_point + static_cast<uint32_t>(n);
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  //将32位的相对序号转换为64位的绝对序号
  (void)zero_point;
  (void)checkpoint;
  return {};
}
