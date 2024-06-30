#include "wrapping_integers.hh"

using namespace std;

const uint64_t num = 1UL << 32;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  //将64位的绝对序号转换为32位的相对序号
  return Wrap32(static_cast<uint32_t>((n + zero_point.raw_value_)% num));  

}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  //将32的相对序号转换为64位的绝对序号

  //先将checkpoint转换为其对应的32位序号
  auto check32 = wrap(checkpoint, zero_point);
  //计算wrap32的表示范围
  uint64_t l = checkpoint - check32.raw_value_;
  uint64_t r = checkpoint + num - 1 - check32.raw_value_;

  //while遍历寻找
  while (l <= r){
    if (wrap(l, zero_point) == *this){
      break;
    }
    ++ l;
  }
  return l;
}
