#include "wrapping_integers.hh"
#include <algorithm>

using namespace std;


uint64_t dist(uint64_t a, uint64_t b) {
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
  checkpoint += zero_point.raw_value_;
  uint64_t high32 = checkpoint & 0xffffffff00000000;
  uint64_t res = high32 | raw_value_;

  if (res < zero_point.raw_value_){
    return res + (1UL << 32) - zero_point.raw_value_;
  }

  uint64_t resl = res - (1UL << 32);
  uint64_t resr = res + (1UL << 32);

  uint64_t dl = dist(resl, checkpoint);
  uint64_t dm = dist(res, checkpoint);
  uint64_t dr = dist(resr, checkpoint);

  uint64_t dmin = min({dl, dm, dr});

  if (dmin == dl){
    return resl - zero_point.raw_value_;
  }else if (dmin == dm){
    return res - zero_point.raw_value_;
  }else{
    return resr - zero_point.raw_value_;
  }
}
