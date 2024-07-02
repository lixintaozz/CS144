#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  //接收来自发送端的报文段，并将其送入reassemble

  //如果为SYN报文段，记录其序号作为ISN
  if (message.SYN) {
    ISN = message.seqno;
    ISNflag = true;
  }

  //如果收到的为开始传送数据之后的FIN报文段，则设置FINflag为true
  if (message.FIN && ISNflag)
    FINflag = true;

  //将收到的数据送入reassembler重排序
  if (message.SYN) {
    reassembler_.insert( (message.seqno + 1).unwrap( ISN, writer().bytes_pushed() ) - 1,
                         message.payload, message.FIN );
  }else{
    reassembler_.insert( message.seqno.unwrap( ISN, writer().bytes_pushed() ) - 1,
                         message.payload, message.FIN );
  }

  //如果连接出错，则设置出错信息
  if (message.RST)
    reader().set_error();
}

TCPReceiverMessage TCPReceiver::send() const
{
  //发送ackno, window_size

  //计算windowsize的大小
  uint16_t window_size_;
  if (reassembler_.writer().available_capacity()  >= UINT16_MAX){
    window_size_ = UINT16_MAX;
  }else{
    window_size_ = reassembler_.writer().available_capacity();
  }

  //计算ackno的大小并发送返回信息
  if (!ISNflag){
    TCPReceiverMessage message{nullopt,window_size_,
                                 reader().has_error() };
    return message;
  }else{
    Wrap32 ackno = Wrap32::wrap( writer().bytes_pushed() + 1, ISN );

    //如果报文段的FIN为true
    if (FINflag){
      ackno = ackno + 1;
    }

    TCPReceiverMessage message = { ackno, window_size_, reader().has_error() };
    return message;
  }
}
