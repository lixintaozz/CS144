#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  //返回没有被确认的sequence number数量
    return bytes_sent_ - ack_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  //返回重新发送的报文段数量
  return consecu_nums_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  //如果bytestream没有要发送的数据，直接返回
  if (input_.reader().bytes_buffered() == 0)
    return;

  //先处理windowsize为0的情况
  if (window_size_ == 0){
    string str;
    read( input_.reader(), 1, str);
    TCPSenderMessage message = {Wrap32::wrap(bytes_sent_, isn_), false};
  }else{

  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  //返回仅包含序列号的TCPSenderMessage，它不占用sequence number
  return {Wrap32::wrap(bytes_sent_ - 1, isn_), false, "", false, false};
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  (void)msg;
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  (void)ms_since_last_tick;
  (void)transmit;
  (void)initial_RTO_ms_;
}
