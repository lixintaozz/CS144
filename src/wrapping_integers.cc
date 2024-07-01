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

  //定义l,r分别向左右遍历
  uint64_t l = checkpoint;
  uint64_t r = checkpoint;

  //考虑checkpoint为0的情况
  if (checkpoint == 0){
    while (!(wrap(r, zero_point) == *this)){
      ++ r;
    }
    return r;
  }

  while (!(wrap(l, zero_point) == *this) && !( wrap(r, zero_point ) == *this)) {
    if (l > 0)
      -- l;
    ++ r;
  }
  if (wrap(l, zero_point) == *this)
    return l;
  else
    return r;
}
