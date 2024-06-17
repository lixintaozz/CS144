#include "byte_stream.hh"

using namespace std;

//写构造函数
ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), error_ (false),
                 closed_(false), bytestream(), avail_capacity_(capacity), accumu_bytes_push(0), accumu_bytes_pop(0){}  //列表初始化需要按照顺序

bool Writer::is_closed() const
{
  return closed_;
}

void Writer::push( string data )
{
  if (available_capacity() >= data.size() && !is_closed()){
    for (auto& ch: data){
      bytestream.push_back(ch);
    }
    avail_capacity_ -= data.size();  
    accumu_bytes_push += data.size();
  }
}

void Writer::close()
{
  closed_ = true;
}

uint64_t Writer::available_capacity() const
{

  return avail_capacity_;
}

uint64_t Writer::bytes_pushed() const
{
  return accumu_bytes_push;
}

bool Reader::is_finished() const
{
  if (bytestream.empty() && closed_){
    return true;
  }else{
    return false;
  }
}

uint64_t Reader::bytes_popped() const
{
  return accumu_bytes_pop;
}

string_view Reader::peek() const
{
  if (!bytestream.empty()){
    string str;
    str.reserve(bytestream.size());
    copy(bytestream.begin(), bytestream.end(), str.begin());
    return str;
  }
  else
    return {};
}

void Reader::pop( uint64_t len )
{
  if (!bytestream.empty()){
    for (uint64_t i = 0; i < len; ++ i){
      bytestream.pop_front();
    }
    accumu_bytes_pop += len;
    avail_capacity_ += len;
  }
}

uint64_t Reader::bytes_buffered() const
{
  return (capacity_ - avail_capacity_); 
}
