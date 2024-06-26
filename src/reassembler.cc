#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  if (data.empty()){
    if (is_last_substring)
      output_.writer().close();
    return;
  }


  if (first_index <= writer().bytes_pushed() && (first_index + data.size()) > writer().bytes_pushed()){
    if (data.size() <= writer().available_capacity()){
      data = data.substr(writer().bytes_pushed());   
      output_.writer().push(data);
      if (is_last_substring) {
        output_.writer().close();
        return;
      }
    }else{
      data = data.substr(writer().bytes_pushed(), writer().available_capacity());
      output_.writer().push(data);
    }
    if (!buffer.empty()){
      for (auto& iter: buffer){   //取出缓冲区中的字符串
        if (iter.first == writer().bytes_pushed()){
          output_.writer().push(iter.second);   
          setBuffersize();
          buffer.erase(iter.first);
          break;
        }
      }
    }
    if (buffer.empty() && is_last)
      output_.writer().close();
  }else if (first_index > writer().bytes_pushed()){
    if (data.size() <= writer().available_capacity()){
      buffer[first_index] = data; 
    }else{
      buffer[first_index] = data.substr(0, writer().available_capacity());
    }
    setBuffersize();
    if (is_last_substring)
      is_last = true;
  }
}

uint64_t Reassembler::bytes_pending() const
{
  return buffersize_;
}


void Reassembler::setBuffersize()
{
  string str = "";
  if (!buffer.empty()){
    for (const auto& iter:buffer){
      str += iter.second;
    }
  }
  buffersize_ = str.size();
}
