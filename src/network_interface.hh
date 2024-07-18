#pragma once

#include <queue>
#include <unordered_map>

#include "address.hh"
#include "ethernet_frame.hh"
#include "ipv4_datagram.hh"


//用于存储待发送的IP数据报的结构
struct ip_wait{
  size_t init_time{};
  std::queue<InternetDatagram> ip_queue{};
};

//用于存储IP-ETHERNET地址映射表的结构
struct ip_ethernet_map{
  EthernetAddress ethernet_address{};
  size_t init_time{};
};

// A "network interface" that connects IP (the internet layer, or network layer)
// with Ethernet (the network access layer, or link layer).

// This module is the lowest layer of a TCP/IP stack
// (connecting IP with the lower-layer network protocol,
// e.g. Ethernet). But the same module is also used repeatedly
// as part of a router: a router generally has many network
// interfaces, and the router's job is to route Internet datagrams
// between the different interfaces.

// The network interface translates datagrams (coming from the
// "customer," e.g. a TCP/IP stack or router) into Ethernet
// frames. To fill in the Ethernet destination address, it looks up
// the Ethernet address of the next IP hop of each datagram, making
// requests with the [Address Resolution Protocol](\ref rfc::rfc826).
// In the opposite direction, the network interface accepts Ethernet
// frames, checks if they are intended for it, and if so, processes
// the the payload depending on its type. If it's an IPv4 datagram,
// the network interface passes it up the stack. If it's an ARP
// request or reply, the network interface processes the frame
// and learns or replies as necessary.
class NetworkInterface
{
public:
  // An abstraction for the physical output port where the NetworkInterface sends Ethernet frames
  class OutputPort
  {
  public:
    virtual void transmit( const NetworkInterface& sender, const EthernetFrame& frame ) = 0;
    virtual ~OutputPort() = default;
  };

  // Construct a network interface with given Ethernet (network-access-layer) and IP (internet-layer)
  // addresses
  NetworkInterface( std::string_view name,
                    std::shared_ptr<OutputPort> port,
                    const EthernetAddress& ethernet_address,
                    const Address& ip_address );

  // Sends an Internet datagram, encapsulated in an Ethernet frame (if it knows the Ethernet destination
  // address). Will need to use [ARP](\ref rfc::rfc826) to look up the Ethernet destination address for the next
  // hop. Sending is accomplished by calling `transmit()` (a member variable) on the frame.
  void send_datagram( const InternetDatagram& dgram, const Address& next_hop );

  // Receives an Ethernet frame and responds appropriately.
  // If type is IPv4, pushes the datagram to the datagrams_in queue.
  // If type is ARP request, learn a mapping from the "sender" fields, and send an ARP reply.
  // If type is ARP reply, learn a mapping from the "sender" fields.
  void recv_frame( const EthernetFrame& frame );

  // Called periodically when time elapses
  void tick( size_t ms_since_last_tick );

  // Accessors
  const std::string& name() const { return name_; }
  const OutputPort& output() const { return *port_; }
  OutputPort& output() { return *port_; }
  std::queue<InternetDatagram>& datagrams_received() { return datagrams_received_; }

private:
  // Human-readable name of the interface
  std::string name_;

  // The physical output port (+ a helper function `transmit` that uses it to send an Ethernet frame)
  std::shared_ptr<OutputPort> port_;
  void transmit( const EthernetFrame& frame ) const { port_->transmit( *this, frame ); }

  // Ethernet (known as hardware, network-access-layer, or link-layer) address of the interface
  EthernetAddress ethernet_address_;

  // IP (known as internet-layer or network-layer) address of the interface
  Address ip_address_;

  // Datagrams that have been received
  std::queue<InternetDatagram> datagrams_received_ {};

  std::unordered_map<uint32_t, ip_wait> ip_to_send_{};   //用于存储等待ARP响应报文的IP数据报
  std::unordered_map<uint32_t ,ip_ethernet_map> ip_ethernet_map_{};  //用于存储IP-ETHERNET地址映射表
  size_t time_ = 0;  //用于记录时间

  template<class T1>
  void serialize( const T1& datagram, std::vector<std::string>& payload )
  {
    //用于将数据报转换为以太帧的payload格式
    Serializer serializer;
    datagram.serialize(serializer);
    payload = serializer.output();
  }

  template<class T2>
  void parse( T2& datagram, const std::vector<std::string>& payload )
  {
    //用于从以太帧的payload格式恢复为数据报
    Parser parser{payload};
    datagram.parse(parser);
  }

  //用于发送ARP请求报文或者ARP相应报文
  void send_arp_(bool is_request, uint32_t ip_required,
                  uint32_t ip_response, EthernetAddress ethernet_response);
};
