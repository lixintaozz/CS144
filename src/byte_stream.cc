#include "byte_stream.hh"


using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), error_ (false),
                 closed_(false), bytestream(), avail_capacity_(capacity), accumu_bytes_push(0), accumu_bytes_pop(0){}


bool Writer::is_closed() const
{
  return closed_;
}

void Writer::push( string data )
{
  if (is_closed()){
    return;
  }

  if (available_capacity() >= data.size()){
    bytestream += data;
    avail_capacity_ -= data.size();  
    accumu_bytes_push += data.size();
  }else{
    bytestream += data.substr(0, available_capacity());
    accumu_bytes_push += available_capacity();
    avail_capacity_ -= available_capacity();  
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
  if (bytestream.empty() && writer().is_closed()){
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
  return bytestream;
}

void Reader::pop( uint64_t len )
{
  if (len <= bytes_buffered()){
    bytestream = bytestream.substr(len);
    accumu_bytes_pop += len;
    avail_capacity_ += len;
  }else{
    bytestream.clear();
    accumu_bytes_pop += bytes_buffered();
    avail_capacity_ += bytes_buffered();
  }
}

uint64_t Reader::bytes_buffered() const
{
  return (capacity_ - avail_capacity_);
}
