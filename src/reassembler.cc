#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring ){
  //先处理字符串为空的情况
  if (data.empty()){
    if (is_last_substring)
      output_.writer().close();
    return;
  }
  
  //如果字符串超出bytestream或者重复发送了，不做处理直接返回
  if (first_index + data.size() - 1 < writer().bytes_pushed() || 
  first_index > writer().bytes_pushed() + writer().available_capacity() - 1){
    return;
  }
 
 //子串头部有部分重复发送，截取需要处理的数据
  if (first_index < writer().bytes_pushed()){
    data = data.substr((writer().bytes_pushed() - first_index));
    first_index = writer().bytes_pushed();
  } 

  //子串尾部有部分溢出，截取需要处理的数据
  if ((data.size() + first_index) > (writer().bytes_pushed() + writer().available_capacity())){
    data = data.substr(0, (writer().bytes_pushed() - first_index));
  }


  //对特殊情况的数据处理完毕后，现在来正式处理字符串的插入
  //先考虑缓冲区为空的情况
  if (buffer.empty()){
    if (first_index == writer().bytes_pushed()){  //顺序直接接收
      output_.writer().push(data);
      if (is_last_substring)
        output_.writer().close();
    }else{  //未按顺序到达放入缓冲区
      buffer[first_index] = data;
      if (is_last_substring)
        is_last = true;
    }
  }else{  //缓冲区非空的情况
    if (first_index == writer().bytes_pushed()){    //数据顺序到达
    auto it = buffer.begin();
      if ((first_index + data.size() - 1) < it -> first){
        output_.writer().push(data);
      }else{
        output_.writer().push(data.substr(0, (it -> first - first_index)));
        while (true){ //while循环逐个处理缓冲区内的子串，有重叠则取后半部分，无重叠不用管（或者直接加入bytespending大小）

        }
      }
    }  

  }



}

uint64_t Reassembler::bytes_pending() const
{

}



