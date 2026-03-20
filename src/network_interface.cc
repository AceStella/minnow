#include <iostream>

#include "arp_message.hh"
#include "debug.hh"
#include "ethernet_frame.hh"
#include "exception.hh"
#include "helpers.hh"
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
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to
void NetworkInterface::send_datagram( InternetDatagram dgram, const Address& next_hop )
{
  const uint32_t next_hop_ip = next_hop.ipv4_numeric();

  // 1. 查找 ARP 缓存
  auto cache_it = arp_cache_.find( next_hop_ip );
  if ( cache_it != arp_cache_.end() ) {
    // 命中缓存：直接构造 IPv4 以太网帧并发送
    EthernetFrame frame;
    frame.header.type = EthernetHeader::TYPE_IPv4;
    frame.header.src = ethernet_address_;
    frame.header.dst = cache_it->second.mac;
    frame.payload = serialize( dgram );
    transmit( frame );
  } else {
    // 未命中：将数据报推入等待队列
    waiting_dgrams_[next_hop_ip].push_back( dgram );

    // 检查过去 5 秒内是否已经对该 IP 发送过 ARP 请求
    if ( arp_requests_timer_.find( next_hop_ip ) == arp_requests_timer_.end() ) {
      // 构造 ARP 请求
      ARPMessage arp_req;
      arp_req.opcode = ARPMessage::OPCODE_REQUEST;
      arp_req.sender_ethernet_address = ethernet_address_;
      arp_req.sender_ip_address = ip_address_.ipv4_numeric();
      arp_req.target_ethernet_address = {}; // 请求中目标 MAC 留空
      arp_req.target_ip_address = next_hop_ip;

      // 封装为以太网广播帧
      EthernetFrame frame;
      frame.header.type = EthernetHeader::TYPE_ARP;
      frame.header.src = ethernet_address_;
      frame.header.dst = ETHERNET_BROADCAST;
      frame.payload = serialize( arp_req );

      transmit( frame );

      // 记录该 IP 的 ARP 请求发送时间，开始计时
      arp_requests_timer_[next_hop_ip] = 0;
    }
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( EthernetFrame frame )
{
  // 1. 过滤：目标 MAC 既不是本机，也不是广播地址时丢弃
  if ( frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST ) {
    return;
  }

  // 2. 处理 IPv4 数据报
  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram dgram;
    if ( parse( dgram, frame.payload ) ) {
      datagrams_received_.push( dgram );
    }
  }
  // 3. 处理 ARP 消息
  else if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage arp_msg;
    if ( parse( arp_msg, frame.payload ) ) {
      const uint32_t sender_ip = arp_msg.sender_ip_address;
      const EthernetAddress sender_mac = arp_msg.sender_ethernet_address;

      // 学习映射：将发送方的信息记入缓存，存活时间重置为 0
      arp_cache_[sender_ip] = { sender_mac, 0 };

      // 发送之前因为等待该 MAC 地址而挂起的所有数据报
      auto waiting_it = waiting_dgrams_.find( sender_ip );
      if ( waiting_it != waiting_dgrams_.end() ) {
        for ( const auto& dgram : waiting_it->second ) {
          EthernetFrame out_frame;
          out_frame.header.type = EthernetHeader::TYPE_IPv4;
          out_frame.header.src = ethernet_address_;
          out_frame.header.dst = sender_mac;
          out_frame.payload = serialize( dgram );
          transmit( out_frame );
        }
        waiting_dgrams_.erase( waiting_it );
      }

      // 如果这是一个询问本机 IP 的 ARP 请求，发送 ARP 回复
      if ( arp_msg.opcode == ARPMessage::OPCODE_REQUEST
           && arp_msg.target_ip_address == ip_address_.ipv4_numeric() ) {

        ARPMessage arp_reply;
        arp_reply.opcode = ARPMessage::OPCODE_REPLY;
        arp_reply.sender_ethernet_address = ethernet_address_;
        arp_reply.sender_ip_address = ip_address_.ipv4_numeric();
        arp_reply.target_ethernet_address = sender_mac;
        arp_reply.target_ip_address = sender_ip;

        EthernetFrame reply_frame;
        reply_frame.header.type = EthernetHeader::TYPE_ARP;
        reply_frame.header.src = ethernet_address_;
        reply_frame.header.dst = sender_mac; // 单播回传给请求方
        reply_frame.payload = serialize( arp_reply );
        transmit( reply_frame );
      }
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // 1. 过期 ARP 缓存：清理存活超过 30 秒 (30000 毫秒) 的记录
  for ( auto it = arp_cache_.begin(); it != arp_cache_.end(); ) {
    it->second.time_to_live += ms_since_last_tick;
    if ( it->second.time_to_live > 30000 ) {
      it = arp_cache_.erase( it );
    } else {
      ++it;
    }
  }

  // 2. 过期 ARP 请求冷却：清理等待超过 5 秒 (5000 毫秒) 的请求记录
  for ( auto it = arp_requests_timer_.begin(); it != arp_requests_timer_.end(); ) {
    it->second += ms_since_last_tick;
    if ( it->second > 5000 ) {
      waiting_dgrams_.erase( it->first );
      it = arp_requests_timer_.erase( it );
    } else {
      ++it;
    }
  }
}
