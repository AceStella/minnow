#include "router.hh"
#include "debug.hh"

#include <iostream>

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

  routing_table_.push_back( { route_prefix, prefix_length, next_hop, interface_num } );
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  for ( auto& current_interface : interfaces_ ) {
    // Note: Depending on your specific Minnow version, receiving datagrams might use
    // a `maybe_receive()` method returning an optional, or direct access to a queue.
    // This assumes the standard `datagrams_received()` queue interface.
    auto& queue = current_interface->datagrams_received();

    while ( !queue.empty() ) {
      auto dgram = queue.front();
      queue.pop();

      // If the TTL was zero already, or hits zero after the decrement, drop the datagram
      if ( dgram.header.ttl <= 1 ) {
        continue;
      }

      int best_match_index = -1;
      int max_prefix_length = -1;
      uint32_t dst_ip = dgram.header.dst;

      // Search the routing table to find the longest-prefix match
      for ( size_t i = 0; i < routing_table_.size(); ++i ) {
        const auto& route_entry = routing_table_[i];

        // Avoid undefined behavior of shifting a 32-bit int by 32 bits
        uint32_t mask = ( route_entry.prefix_length == 0 ) ? 0 : ( ~0u ) << ( 32 - route_entry.prefix_length );

        // Most-significant prefix length bits of the destination address must be identical to the route_prefix
        if ( ( dst_ip & mask ) == ( route_entry.route_prefix & mask ) ) {
          // Choose the route with the biggest value of prefix_length
          if ( route_entry.prefix_length > max_prefix_length ) {
            max_prefix_length = route_entry.prefix_length;
            best_match_index = static_cast<int>( i );
          }
        }
      }

      // If a route matched, forward the datagram
      if ( best_match_index >= 0 ) {
        dgram.header.ttl--;

        // Recompute the checksum since the header (TTL) has been modified
        dgram.header.compute_checksum();

        const auto& best_route = routing_table_[best_match_index];

        // If the next hop is empty, the next hop is the datagram's destination address
        Address next_hop
          = best_route.next_hop.has_value() ? best_route.next_hop.value() : Address::from_ipv4_numeric( dst_ip );

        interface( best_route.interface_num )->send_datagram( dgram, next_hop );
      }
    }
  }
}
