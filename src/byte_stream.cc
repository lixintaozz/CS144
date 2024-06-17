#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return closed_;
}

void Writer::push( string data )
{
  if (available_capacity() >= data.size() && !is_closed()){
    bytestream << data;
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
  if (bytestream.eof() && closed_){
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
  if (!bytestream.eof())
    return bytestream.str();
}

void Reader::pop( uint64_t len )
{
  if (!bytestream.eof()){
    char ch;
    for (uint64_t i = 0; i < len; ++ i){
      bytestream >> ch;
    }
    accumu_bytes_pop += len;
    avail_capacity_ += len;
  }
}

uint64_t Reader::bytes_buffered() const
{
  return (capacity_ - avail_capacity_); 
}
