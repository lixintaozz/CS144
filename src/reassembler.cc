#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  //如果字符串为空且是最后一个子串，则关闭连接后返回，否则直接返回
  if (data.empty()){
    if (is_last_substring)
      output_.writer().close();
    return;
  }
  //重复发送或者超出reassembler直接返回
  if (data.size() + first_index <= writer().bytes_pushed() ||
       first_index >= writer().bytes_pushed() + writer().available_capacity()){
    return;
  }else if (first_index <= writer().bytes_pushed()){    //能够按序直接插入而不需要缓存
    if (buffer.empty()){   //缓冲区为空的情况
      if ((data.size() + first_index) <= (writer().available_capacity() + writer().bytes_pushed())) {    //整个子串截取插入
        data = data.substr(writer().bytes_pushed() - first_index);
        output_.writer().push( data );
        if (is_last_substring)
          output_.writer().close();
      }else{
        data = data.substr(writer().bytes_pushed() - first_index, writer().available_capacity());
        output_.writer().push(data);
      }
    }else{   //缓冲区非空的情况
      auto first_iter = buffer.begin();
      if ((data.size() + first_index) <= first_iter->first){
        data = data.substr(writer().bytes_pushed() - first_index);
        output_.writer().push( data );
        if (is_last_substring)
          output_.writer().close();
      }else{
        data = data.substr(writer().bytes_pushed() - first_index, (first_iter->first - writer().bytes_pushed()));
        output_.writer().push(data);
      }
    }
    //遍历缓冲区，将其中符合条件的字符串取出
    if (!buffer.empty()){
      while(true){
        bool flag = false;
        for (auto iter = buffer.begin(); iter != buffer.end(); ++ iter){
          if (iter -> first == writer().bytes_pushed()){
            output_.writer().push(iter->second);
            buffer.erase(iter);
            setBuffersize();
            flag = true;
            break;
          }
        }

        if (!flag)
          break;
      }
    }

    //如果缓冲区中含有末尾元素则关闭连接
    if (buffer.empty() && is_last)
      output_.writer().close();
  }else if (first_index > writer().bytes_pushed()){   //处理非顺序到达的子串
    if (buffer.empty()){   //缓冲区为空的情况
      if ((data.size() + first_index) <= (writer().bytes_pushed() + writer().available_capacity())) //待验证？？？
    }else{   //缓冲区非空的情况

    }
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
