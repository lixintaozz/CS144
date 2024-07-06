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

  //如果还没有传送任何数据，那么先发送SYN报文段
  if (input_.reader().bytes_popped() == 0) {
    Timer::RTO_time = initial_RTO_ms_; // 设置Timer类的初始RTO值
    TCPSenderMessage message = { isn_, true, "", false, false };
    transmit( message );
    Timer t( message, time_ );
    seq_buffer_.push( std::move( t ) );
  }

  //开始发送数据报文段
  string str;
  if (window_size_ == 0)   //处理windowsize为0的情况
    read( input_.reader(), window_size_ + 1, str);
  else if (window_size_ > TCPConfig::MAX_PAYLOAD_SIZE)   //处理windowsize过大的情况
    read( input_.reader(), TCPConfig::MAX_PAYLOAD_SIZE, str);
  else
    read( input_.reader(), window_size_, str);

  TCPSenderMessage message = {Wrap32::wrap(bytes_sent_, isn_),
                               false, str,false,false };
  transmit(message);
  Timer t( message, time_ );
  seq_buffer_.push( std::move( t ) );
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  //返回仅包含序列号的TCPSenderMessage，它不占用sequence number
  return {Wrap32::wrap(bytes_sent_ - 1, isn_), false, "", false, false};
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{

}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // 重新发送的逻辑由tick来实现
  (void)ms_since_last_tick;
  (void)transmit;
  (void)initial_RTO_ms_;
}

bool Timer::alarm( uint64_t time_now ) const
{
  return (time_now - initial_time_ >= RTO_time);
}

void Timer::reset( uint64_t time_now )
{
  initial_time_ = time_now;
}

void Timer::doubleRTO()
{
  RTO_time = RTO_time * 2;
}

void Timer::setRTO(uint64_t rto_time)
{
  RTO_time = rto_time;
}