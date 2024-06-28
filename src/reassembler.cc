#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring ){
  bool complete = true;
  //先处理字符串(data)为空的情况，字符串为空，为最后一个串，且bytestream内的字符全部读出后才能close
  if (data.empty()){
    if (is_last_substring && (writer().bytes_pushed() == reader().bytes_popped()))
      output_.writer().close();
    return;
  }

  //如果bytestream没有可用空间直接返回
  if (writer().available_capacity() == 0)
    return;

  //如果字符串超出bytestream或者重复发送了，不做处理直接返回
  if (first_index + data.size() - 1 < writer().bytes_pushed() || 
  first_index > writer().bytes_pushed() + writer().available_capacity() - 1){
    return;
  }
 
 //字符串头部有部分重复发送，截取需要处理的数据
  if (first_index < writer().bytes_pushed()){
    data = data.substr((writer().bytes_pushed() - first_index));
    first_index = writer().bytes_pushed();
  } 

  //字符串尾部有部分溢出，截取需要处理的数据
  if ((data.size() + first_index) > (writer().bytes_pushed() + writer().available_capacity())){
    data = data.substr(0, (writer().bytes_pushed() + writer().available_capacity() - first_index));
    complete = false;
  }


  //对特殊情况的数据处理完毕后，现在来正式处理字符串的插入

  //先把字符串直接插入缓冲区
  //相同键的buffer块，只保留大的buffer块；不同键的buffer块，直接插入
  auto find_iter = buffer.find(first_index);
  if (find_iter == buffer.end()){
    buffer[first_index] = std::move(data);
  }else{
    if (data.size() > find_iter -> second.size())
      buffer[first_index] = std::move(data);
  }

  //如果插入的是最后一个块，设置is_last = true
  if (is_last_substring && complete)
    is_last = true;

  //然后再使用while循环统一push
  auto iter = buffer.begin();
  while (iter != buffer.end()){ //while循环逐个处理缓冲区内的子串
    if (iter -> first <= writer().bytes_pushed() &&
          (iter -> first + iter -> second.size() - 1) >= writer().bytes_pushed()) {  //buffer块相交但不重合的情况，截取后半部分
      output_.writer().push( iter -> second.substr( writer().bytes_pushed() - iter -> first ) );
      iter = buffer.erase(iter);
    }else if (iter -> first <= writer().bytes_pushed() &&
                (iter -> first + iter -> second.size() - 1) < writer().bytes_pushed()) { // buffer块重合的情况，直接删除被覆盖的buffer块
      iter = buffer.erase( iter );
    }else{   //buffer块不相交
      break;
    }
  }

  //判断是否完成所有字符串的读取，如果完成了，则关闭连接
  if (buffer.empty() && is_last)
    output_.writer().close();
}


uint64_t Reassembler::bytes_pending() const
{
  //处理缓冲区为空的情况
  if (buffer.empty())
    return 0;

  //设置初始条件
  auto iter = buffer.begin();
  uint64_t  byte_size = 0;
  uint64_t cur = iter -> second.size() + iter -> first;
  byte_size += iter -> second.size();
  ++ iter;

  //while循环处理整个缓冲区
  while (iter != buffer.end()){
    if (iter -> first <= cur && (iter -> first + iter -> second.size() -1) >= cur) { //相交
      byte_size += ( iter->first + iter->second.size() - cur );
      cur += ( iter->first + iter->second.size() - cur );
      ++ iter;
    }else if (iter -> first <= cur && (iter -> first + iter -> second.size() -1) < cur){  //重合
      ++ iter;
    }else{ //不相交
      cur = iter -> second.size() + iter -> first;
      byte_size += iter -> second.size();
      ++ iter;
    }
  }

  return byte_size;
}



