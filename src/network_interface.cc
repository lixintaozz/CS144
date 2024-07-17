#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  //该函数用于将接收到的IP数据报打包为以太帧并发送
  if (ip_ethernet_map_.find(next_hop.ipv4_numeric())
       != ip_ethernet_map_.end())   //如果地址映射表中包含该IP地址对应的以太帧地址，直接发送
  {
    EthernetFrame ethernetFrame;
    ethernetFrame.header.type = EthernetHeader::TYPE_IPv4;
    ethernetFrame.header.src = ethernet_address_;
    ethernetFrame.header.dst = ip_ethernet_map_[next_hop.ipv4_numeric()].ethernet_address;
    std::vector<std::string> payload;
    serialize<InternetDatagram>(dgram,payload);
    ethernetFrame.payload = payload;
    transmit(ethernetFrame);
  }else   //否则发送ARP请求报文，记录ARP请求报文的发送时间，并将需要发送的数据报放入队列中
  {
    send_arp_( true, next_hop.ipv4_numeric(),
               0, ETHERNET_BROADCAST);  //这里ethernet_response和ip_response的值可以任意设置
    ip_to_send_[next_hop.ipv4_numeric()].ip_queue.push(dgram);
    ip_to_send_[next_hop.ipv4_numeric()].init_time = time_;
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // Your code here.
  (void)frame;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  (void)ms_since_last_tick;
}

void NetworkInterface::send_arp_( bool is_request, uint32_t ip_required,
                                  uint32_t ip_response, EthernetAddress ethernet_response)
{
  //用于发送ARP请求报文或者ARP相应报文

  //开始构造 ARP EtherNet Frame
  EthernetFrame ethernetFrame;
  ethernetFrame.header.type = EthernetHeader::TYPE_ARP;
  ethernetFrame.header.src = ethernet_address_;
  ARPMessage arpMessage;
  arpMessage.sender_ethernet_address = ethernet_address_;
  arpMessage.sender_ip_address = ip_address_.ipv4_numeric();


  if (is_request)
  {
    ethernetFrame.header.dst = ETHERNET_BROADCAST;
    arpMessage.opcode = 1;  //设置为1表示这是ARP请求报文
    arpMessage.target_ip_address = ip_required;
    std::vector<std::string> payload;
    serialize<ARPMessage>(arpMessage, payload);
    ethernetFrame.payload = payload;
  } else
  {
    ethernetFrame.header.dst = ethernet_response;
    arpMessage.opcode = 0;  //设置为0表示这是ARP响应报文
    arpMessage.target_ip_address = ip_response;
    arpMessage.target_ethernet_address = ethernet_response;
    std::vector<std::string> payload;
    serialize<ARPMessage>(arpMessage, payload);
    ethernetFrame.payload = payload;
  }

  //发送构造好的ARP EtherNet Frame
  transmit(ethernetFrame);
}
