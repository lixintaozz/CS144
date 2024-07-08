#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>
#include <deque>




class Timer
{
public:
  static uint64_t  RTO_time; //RTO_time在push SYN报文段之前设置
  TCPSenderMessage seg_;

  bool alarm(uint64_t time_now) const;
  void reset(uint64_t time_now);
  Timer(TCPSenderMessage seg, uint64_t initial_time): seg_(std::move(seg)),initial_time_(initial_time){}

private:
  uint64_t initial_time_;
};


class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms )
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

private:
  // Variables initialized in constructor
  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  uint64_t bytes_sent_ = isn_.unwrap(isn_, 0) ;  //sender将要发送的absolute sequence number，初始为isn_
  uint64_t ack_ = isn_.unwrap(isn_, 0); //receiver返回的期望接收的absolute sequence number，初始为isn_
  uint64_t consecu_nums_ = 0;  //重新发送的报文段数量
  uint16_t window_size_ = 0;   //receiver的windowsize，初始尺寸默认为0
  uint64_t time_ = 0;  //sender已经存活的时间，充当时钟的作用
  std::deque<Timer> seq_buffer_{};  //用于存储outstanding报文段
  bool syn_ = false;  //表示SYN报文段是否已经发送，默认为false
  bool zero_window_ = false;  //表示window是否为空
  bool fin_ = false;  //表示FIN报文段是否已经发送，默认为false
};
