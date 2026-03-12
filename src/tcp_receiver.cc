#include "tcp_receiver.hh"
#include "debug.hh"
#include <utility>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( message.RST ) {
    reader().set_error();
    return;
  }

  if ( message.SYN && !isn_.has_value() ) {
    isn_ = message.seqno;
  } else if ( !isn_.has_value() ) {
    return;
  }

  const uint64_t checkpoint = writer().bytes_pushed() + 1;
  const uint64_t abs_seqno = message.seqno.unwrap( isn_.value(), checkpoint );

  uint64_t stream_index = 0;
  if ( message.SYN ) {
    stream_index = abs_seqno;
  } else {
    stream_index = abs_seqno - 1;
  }

  reassembler_.insert( stream_index, move( message.payload ), message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage message;

  const uint64_t capacity = writer().available_capacity();
  message.window_size = capacity > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>( capacity );

  if ( isn_.has_value() ) {
    uint64_t abs_seqno = 1 + writer().bytes_pushed();
    if ( writer().is_closed() ) {
      abs_seqno += 1;
    }
    message.ackno = Wrap32::wrap( abs_seqno, isn_.value() );
  }

  message.RST = writer().has_error();

  return message;
}
