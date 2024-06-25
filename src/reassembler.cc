#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  if (reader().bytes_buffered() > first_index){   //出现了重复发送的情况
    return;
  }

  if (writer().available_capacity() - buffersize_ >= data.size()){    //bytestream的可用空间足够
    if (reader().bytes_buffered() == first_index){   //数据按序到达的情况
      output_.writer().push(data);
      bool flag = false;
      while (true){
        for (auto& iter: buffer){
          if (iter.first == reader().bytes_buffered()){    //删除buffer中的满足条件的string
            output_.writer().push(iter.second);
            buffer.erase(iter.first);
            flag = true;
            break;
          }
        }
        if (!flag)
          break;
      }
      setBuffersize();   //更新buffersize_的值
    }else{   //数据错序到达的情况
      buffer.insert({first_index, data});
      setBuffersize();   //更新buffersize的值
    }
  }else{   //bytestream的可用空间不够
    auto datalen = writer().available_capacity() - buffersize_;
    data = data.substr(0, datalen);
    if (reader().bytes_buffered() == first_index){   //数据按序到达的情况
      output_.writer().push(data);
      bool flag = false;
      while (true){
        for (auto& iter: buffer){
          if (iter.first == reader().bytes_buffered()){    //删除buffer中的满足条件的string
            output_.writer().push(iter.second);
            buffer.erase(iter.first);
            flag = true;
            break;
          }
        }
        if (!flag)
          break;
      }
      setBuffersize();   //更新buffersize_的值
    }else{   //数据错序到达的情况
      buffer.insert({first_index, data});
      setBuffersize();   //更新buffersize的值
    }
  }
}

uint64_t Reassembler::bytes_pending() const
{
  return reader().bytes_buffered() + buffersize_;
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
