#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  //该函数将路由表项添加到路由表中
  router_map_[route_prefix].prefix_length = prefix_length;
  router_map_[route_prefix].next_hop = next_hop;
  router_map_[route_prefix].interface_num = interface_num;
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  //该函数将收到的IP数据报转发到对应的网络
  //遍历router的每个interface，依次处理收到的IP数据报
  for (auto& pointer:_interfaces)
  {
    auto& queue_temp = pointer -> datagrams_received();
    while (!queue_temp.empty())
    {
      //如果IP数据报过期，直接丢弃
      if (queue_temp.front().header.ttl == 0 || queue_temp.front().header.ttl == 1) {
        queue_temp.pop();
        continue;
      }

      //遍历路由器转发表，寻找转发接口
      bool find = false;
      if (!router_map_.empty()) {
        for ( auto iter = router_map_.begin(); iter != router_map_.end(); ++ iter )
        {
          if ( match( queue_temp.front().header.dst, iter-> first, iter->second.prefix_length ) ) {
            find = true;
            // 如果下一跳是路由器，就使用转发表存储的下一跳地址；否则使用IP数据报的目的IP地址
            if ( iter->second.next_hop.has_value() )
              interface( iter->second.interface_num )
                ->send_datagram( queue_temp.front(), iter->second.next_hop.value() );
            else
              interface( iter->second.interface_num )
                ->send_datagram( queue_temp.front(),
                                 Address::from_ipv4_numeric( queue_temp.front().header.dst ) );

            queue_temp.pop();
            break;
          }
        }
      }
      // 如果没有找到合适的路由转发表项，就扔弃该IP数据报
      if ( !find )
        queue_temp.pop();
    }
  }
}

bool Router::match( uint32_t ip_address, uint32_t route_prefix, uint8_t prefix_length )
{
  //判断两个IP地址是否前缀匹配
  uint32_t mask = UINT32_MAX;
  mask <<= (32 - prefix_length);
  bool equal = (mask & ip_address) == route_prefix;
  return equal;
}
