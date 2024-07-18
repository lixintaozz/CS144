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
  //如果收到的帧既不是广播帧也不是发往该接口的帧，直接返回
  if (frame.header.dst != ETHERNET_BROADCAST && frame.header.dst != ethernet_address_)
    return;

  if (frame.header.type == EthernetHeader::TYPE_IPv4)   //如果收到的是IP数据报
  {
    InternetDatagram internetDatagram;
    parse<InternetDatagram>(internetDatagram, frame.payload);
    datagrams_received_.push(internetDatagram);
  }else    //如果收到的是ARP数据报
  {
    ARPMessage arpMessage;
    parse<ARPMessage>(arpMessage, frame.payload);
    ip_ethernet_map_[arpMessage.sender_ip_address].ethernet_address = arpMessage.sender_ethernet_address;
    ip_ethernet_map_[arpMessage.sender_ip_address].init_time = time_;

    if (arpMessage.opcode == ARPMessage::OPCODE_REQUEST
         && arpMessage.target_ip_address == ip_address_.ipv4_numeric())   //如果收到的是ARP请求数据报
    {
      send_arp_( false, 0, arpMessage.sender_ip_address,arpMessage.sender_ethernet_address);
    }else
    {
      //将等待该响应报文的IP数据报全部发送出去
      auto send_queue = ip_to_send_[arpMessage.sender_ip_address].ip_queue;
      while (!send_queue.empty())
      {
        send_datagram(send_queue.front(), Address::from_ipv4_numeric(arpMessage.sender_ip_address));
        send_queue.pop();
      }
      ip_to_send_.erase(arpMessage.sender_ip_address);
    }
  }

}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  //更新时钟的值
  time_ += ms_since_last_tick;

  //遍历ip_ethernet_map_删除过期表项
  for (auto iter = ip_ethernet_map_.begin(); iter != ip_ethernet_map_.end(); )
  {
    if (time_ - iter -> second.init_time > 30000)
    {
      iter = ip_ethernet_map_.erase(iter);
    }else
    {
      ++ iter;
    }
  }

  //遍历ip_to_send_检查是否需要重新发送ARP请求报文
  for (auto iter = ip_to_send_.begin(); iter != ip_to_send_.end();++ iter)
  {
    if (ip_to_send_.find(iter -> first) != ip_to_send_.end() && time_ - iter -> second.init_time > 5000)
      send_arp_( true,iter -> first, 0, ETHERNET_BROADCAST);
  }
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
    arpMessage.opcode = ARPMessage::OPCODE_REQUEST;
    arpMessage.target_ip_address = ip_required;
    std::vector<std::string> payload;
    serialize<ARPMessage>(arpMessage, payload);
    ethernetFrame.payload = payload;
  } else
  {
    ethernetFrame.header.dst = ethernet_response;
    arpMessage.opcode = ARPMessage::OPCODE_REPLY;
    arpMessage.target_ip_address = ip_response;
    arpMessage.target_ethernet_address = ethernet_response;
    std::vector<std::string> payload;
    serialize<ARPMessage>(arpMessage, payload);
    ethernetFrame.payload = payload;
  }

  //发送构造好的ARP EtherNet Frame
  transmit(ethernetFrame);
}
