#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;


//不能在头文件中定义静态成员变量，而是应该在源文件中进行定义，
//头文件中只能声明静态成员变量，而不能定义。
uint64_t Timer::RTO_time;  // 在类定义外定义静态成员变量


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
  if (input_.reader().bytes_buffered() == 0 && syn_)
    return;

  //如果还没有传送SYN报文段，那么先发送SYN报文段
  if (!syn_)
  {
    Timer::RTO_time = initial_RTO_ms_; // 设置Timer类的初始RTO值
    TCPSenderMessage message = { isn_, true, "", false, false };
    transmit( message );
    Timer t( message, time_ );
    seq_buffer_.push_back( std::move( t ) );
    bytes_sent_ += 1;
    syn_ = true;
    return;
  }

  //开始发送数据报文段
  string str;
  if (window_size_ == 0)   //处理windowsize为0的情况
    read( input_.reader(), window_size_ + 1, str);
  else if (window_size_ > TCPConfig::MAX_PAYLOAD_SIZE)   //处理windowsize过大的情况
  {
    read( input_.reader(), TCPConfig::MAX_PAYLOAD_SIZE, str );
    window_size_ -= str.size();
  }
  else {
    read( input_.reader(), window_size_, str );
    window_size_ -= str.size();
  }
  TCPSenderMessage message = {Wrap32::wrap(bytes_sent_, isn_),
                               false, str,false,false };
  transmit(message);
  bytes_sent_ += str.size();
  Timer t( message, time_ );
  seq_buffer_.push_back( std::move( t ) );

  //如果所有数据均已经发送完毕，发送FIN报文段
  if (input_.reader().is_finished())
  {
    TCPSenderMessage messages = { Wrap32::wrap(bytes_sent_, isn_),
                                  false, "", true, false };
    transmit( messages );
    Timer ts( messages, time_ );
    seq_buffer_.push_back( std::move( ts ) );
    bytes_sent_ += 1;
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  //返回仅包含序列号的TCPSenderMessage，它不占用sequence number
  return {Wrap32::wrap(bytes_sent_, isn_), false, "", false, false};
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{

  //设置receiver期望接收的absolute sequence number和receiver的windowsize
  if (msg.ackno.has_value())
    ack_ = msg.ackno->unwrap(isn_, ack_);
  window_size_ = msg.window_size;

  bool flag = false;   //flag表示receiver是否传来对新的报文段的接收信号

  //遍历seq_buffer_移出收到ackno的报文段
  if (!seq_buffer_.empty())
  {
    for (auto iter = seq_buffer_.begin(); iter != seq_buffer_.end();)
    {
      //当接收端传来新的数据确认
      if (iter->seg_.sequence_length() <= ack_)
      {
        iter = seq_buffer_.erase(iter);
        flag = true;
      }else
      {
        break;
      }
    }
  }

  //如果新收到返回确认信号
  if (flag)
  {
    Timer::RTO_time = initial_RTO_ms_;
    if (!seq_buffer_.empty())
    {
      for (auto& item: seq_buffer_)
      {
        item.reset(time_);
      }
    }
    consecu_nums_ = 0;
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  //更新sender此刻的时间
  time_ += ms_since_last_tick;
  //检查此时是否有报文段需要重传，需要的话重新传送该报文段
  for (auto& item: seq_buffer_)
  {
    if (item.alarm(time_))
    {
      transmit(item.seg_);
      break;
    }
  }
  //更新consecu_nums_和RTO_time的值
  if (window_size_ != 0)
  {
    consecu_nums_ += 1;
    Timer::RTO_time *= 2;
  }

  //计时器全部重新倒计时
  for (auto& item: seq_buffer_)
  {
    item.reset(time_);
  }
}

bool Timer::alarm( uint64_t time_now ) const
{
  return (time_now - initial_time_ >= RTO_time);
}

void Timer::reset( uint64_t time_now )
{
  initial_time_ = time_now;
}
