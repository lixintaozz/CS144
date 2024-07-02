#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  //接收来自发送端的报文段，并将其送入reassemble

  //如果为SYN报文段，记录其序号作为ISN
  if (message.SYN) {
    ISN = message.seqno;
  }else{
      reassembler_.insert(message.seqno.unwrap( ISN, writer().bytes_pushed() ) - 1,
                           message.payload, message.FIN);
  }

  //如果连接出错，则设置出错信息
  if (message.RST)
    reader().set_error();
}

TCPReceiverMessage TCPReceiver::send() const
{
  //发送ackno, window_size
  if (reassembler_.writer().bytes_pushed() == 0){
    TCPReceiverMessage message{nullopt,static_cast<uint16_t>(reassembler_.writer().available_capacity()),
                                 reader().has_error() };
    return message;
  }else{
    optional<Wrap32> ackno = Wrap32::wrap(writer().bytes_pushed() + 1, ISN);
    TCPReceiverMessage message{ackno, static_cast<uint16_t>(reassembler_.writer().available_capacity()), reader().has_error()};
    return message;
  }
}
