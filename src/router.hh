#pragma once

#include <memory>
#include <optional>
#include <map>

#include "exception.hh"
#include "network_interface.hh"

//路由器转发表表项的数据结构
struct router_item{
  uint32_t route_prefix{};
  std::optional<Address> next_hop{};
  size_t interface_num{};
};

// \brief A router that has multiple network interfaces and
// performs longest-prefix-match routing between them.
class Router
{
public:
  // Add an interface to the router
  // \param[in] interface an already-constructed network interface
  // \returns The index of the interface after it has been added to the router
  size_t add_interface( std::shared_ptr<NetworkInterface> interface )
  {
    _interfaces.push_back( notnull( "add_interface", std::move( interface ) ) );
    return _interfaces.size() - 1;
  }

  // Access an interface by index
  std::shared_ptr<NetworkInterface> interface( const size_t N ) { return _interfaces.at( N ); }

  // Add a route (a forwarding rule)
  void add_route( uint32_t route_prefix,
                  uint8_t prefix_length,
                  std::optional<Address> next_hop,
                  size_t interface_num );

  // Route packets between the interfaces
  void route();

private:
  // The router's collection of network interfaces
  std::vector<std::shared_ptr<NetworkInterface>> _interfaces {};
  std::map<uint8_t, router_item> router_map_ {};   //路由转发表
  bool match(uint32_t ip_address, uint32_t route_prefix, uint8_t prefix_length);   //判断两个IP地址是否前缀匹配
};
