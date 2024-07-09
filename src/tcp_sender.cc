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
  //如果还没有传送SYN报文段，那么先发送SYN报文段（不带payload的SYN报文段）
  if (!syn_ && !receive_first_)
  {
    Timer::RTO_time = initial_RTO_ms_; // 设置Timer类的初始RTO值
    TCPSenderMessage message;
    message = { isn_, true, "", false, input_.has_error() };
    transmit( message );
    Timer t( message, time_ );
    seq_buffer_.push_back( std::move( t ) );
    bytes_sent_ += 1;
    syn_ = true;
    return;
  }

  //如果所有数据均已经发送完毕且window还有空间，发送FIN报文段
  //仅当window_size_能够放下还没有被确认的数据时，我们才考虑发送FIN报文段
  //也能处理SYN和FIN同时为true且没有payload的报文段（特殊情况）
  if (input_.reader().is_finished() && window_size_ != 0 && !fin_ && window_size_ > sequence_numbers_in_flight())
  {
    TCPSenderMessage messages = { Wrap32::wrap(bytes_sent_, isn_),
                                  !syn_, "", true, input_.has_error() };
    transmit( messages );
    bool zero_seg = (window_size_ == 1 && zero_window_);  //该报文段是否为"零"报文段
    if (zero_seg)
      zero_window_ = false;
    Timer ts( messages, time_, zero_seg );
    seq_buffer_.push_back( std::move( ts ) );
    bytes_sent_ += 1;
    fin_ = true;

    //处理发送SYN+FIN为true且payload_len为0的报文段
    if (!syn_)
    {
      syn_ = true;
      bytes_sent_ += 1;
    }
    return;
  }


  //如果input_有数据要发送且window还有空间，那么发送数据报文段
  while (input_.reader().bytes_buffered() != 0 && window_size_ != 0) {
    //如果SYN报文段还没发送，那么就发送带数据的SYN报文段
    if (!syn_)
      window_size_ -= 1;

    string str;
    bool zero_seg = (window_size_ == 1 && zero_window_);  //该报文段是否为"零"报文段
    if ( window_size_ > TCPConfig::MAX_PAYLOAD_SIZE ) // 处理windowsize过大的情况
    {
      read( input_.reader(), TCPConfig::MAX_PAYLOAD_SIZE, str );
      window_size_ -= str.size();
    } else {
      if (zero_seg)
        zero_window_ = false;
      read( input_.reader(), window_size_, str );
      window_size_ -= str.size();
    }
    bool is_fin = input_.reader().is_finished() && window_size_ != 0;  //是否需要设置FIN为true
    TCPSenderMessage message = { Wrap32::wrap( bytes_sent_, isn_ ), !syn_, str, is_fin, input_.has_error() };
    transmit( message );
    if (!syn_)
    {
      bytes_sent_ += 1;
      syn_ = true;
    }
    bytes_sent_ += str.size();
    if (is_fin)
    {
      bytes_sent_ += 1;
      fin_ = true;
    }
    Timer t( message, time_, zero_seg );
    seq_buffer_.push_back( std::move( t ) );
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  //返回仅包含序列号的TCPSenderMessage，它不占用sequence number
  return {Wrap32::wrap(bytes_sent_, isn_), false, "", false, input_.has_error()};
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  //如果SYN报文段发送之前就收到了msg，设置receive_first_为true
  if (!syn_)
    receive_first_ = true;

  //设置receiver期望接收的absolute sequence number和receiver的windowsize
  if (msg.ackno.has_value()) {
    if (msg.ackno->unwrap( isn_, ack_ ) > bytes_sent_)   //如果返回的ack大于bytes_sent_，直接丢弃该msg
      return;
    if (msg.ackno->unwrap( isn_, ack_ ) > ack_)  //只有当收到的ackno比之前收到的ackno大时，才更新ack_的值
      ack_ = msg.ackno->unwrap( isn_, ack_ );
  }

  //如果msg的RST为true，那么设置input_的error位
  if (msg.RST)
    input_.set_error();

  //更新window_size_的值
  if ( msg.window_size == 0 ) {
    window_size_ = 1;
    zero_window_ = true;
  } else {
    window_size_ = msg.window_size;
  }


  bool flag = false;   //flag表示receiver是否传来对新的报文段的接收信号

  //遍历seq_buffer_移出收到ackno的报文段
  if (!seq_buffer_.empty())
  {
    for (auto iter = seq_buffer_.begin(); iter != seq_buffer_.end();)
    {
      //当接收端传来新的数据确认
      if (iter->seg_.sequence_length() + iter -> seg_.seqno.unwrap(isn_, bytes_sent_) <= ack_)
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
  bool flag = false;    //flag表示是否有计时器到时
  bool zero_seg = false;   //zero_seg表示重传的是不是"零"报文段
  if (!seq_buffer_.empty()) {
    for ( auto& item : seq_buffer_ ) {
      if ( item.alarm( time_ ) ) {
        transmit( item.seg_ );
        if (item.is_zero_)
          zero_seg = true;
        flag = true;
        break;
      }
    }
  }

  //如果计时器到时了，更新consecu_nums_和RTO_time的值
  if (flag)
  {
    if (!zero_seg)
      Timer::RTO_time <<= 1;    //这里使用移位操作速度更快
    consecu_nums_ += 1;

    // 计时器全部重新倒计时
    for ( auto& item : seq_buffer_ )
    {
      item.reset( time_ );
    }
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
