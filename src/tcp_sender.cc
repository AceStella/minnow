#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"

using namespace std;

// How many sequence numbers are outstanding?
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return link_bytes_;
}

// How many consecutive retransmissions have happened?
uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive_retransmissions_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  const size_t current_window = ( window_size_ == 0 ) ? 1 : window_size_;

  while ( link_bytes_ < current_window ) {
    TCPSenderMessage msg;

    if ( reader().has_error() ) {
      msg.RST = true;
    }

    if ( next_seqno_ == 0 ) {
      msg.SYN = true;
    }

    const size_t window_space = current_window - link_bytes_;
    const size_t payload_limit = min( TCPConfig::MAX_PAYLOAD_SIZE, window_space - ( msg.SYN ? 1 : 0 ) );

    read( reader(), payload_limit, msg.payload );

    if ( reader().is_finished() && !fin_sent_ ) {
      if ( window_space > msg.sequence_length() ) {
        msg.FIN = true;
        fin_sent_ = true;
      }
    }
    if ( msg.sequence_length() == 0 && !msg.RST ) {
      break;
    }

    msg.seqno = Wrap32::wrap( next_seqno_, isn_ );
    transmit( msg );

    if ( msg.RST ) {
      return;
    }

    if ( !timer_running_ ) {
      timer_running_ = true;
      time_passed_ = 0;
    }

    const size_t len = msg.sequence_length();
    next_seqno_ += len;
    link_bytes_ += len;
    link_messages_.push( msg );

    if ( msg.FIN ) {
      break;
    }
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage msg;
  msg.seqno = Wrap32::wrap( next_seqno_, isn_ );

  if ( reader().has_error() ) {
    msg.RST = true;
  }

  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if ( msg.RST ) {
    writer().set_error();
  }

  // Update window size.
  window_size_ = msg.window_size;

  // Check if the first package is ACK, then calculate absolute ACK number.
  if ( !msg.ackno.has_value() ) {
    return;
  }
  const uint64_t abs_ackno = msg.ackno.value().unwrap( isn_, next_seqno_ );
  if ( abs_ackno > next_seqno_ ) {
    return;
  }

  // Pop the stored messages that were ACK to received.
  bool new_data_acked = false;
  while ( !link_messages_.empty() ) {
    auto& front_msg = link_messages_.front();
    const uint64_t front_abs_seqno = front_msg.seqno.unwrap( isn_, next_seqno_ );
    const size_t len = front_msg.sequence_length();

    if ( front_abs_seqno + len <= abs_ackno ) {
      link_bytes_ -= len;
      link_messages_.pop();
      new_data_acked = true;
    } else {
      break;
    }
  }

  if ( new_data_acked ) {
    current_RTO_ms_ = initial_RTO_ms_;
    consecutive_retransmissions_ = 0;
    time_passed_ = 0;
  }

  if ( link_messages_.empty() ) {
    timer_running_ = false;
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if ( timer_running_ ) {
    time_passed_ += ms_since_last_tick;
  }

  if ( timer_running_ && time_passed_ >= current_RTO_ms_ ) {
    if ( !link_messages_.empty() ) {
      transmit( link_messages_.front() );
    }
    if ( window_size_ > 0 ) {
      consecutive_retransmissions_++;
      current_RTO_ms_ *= 2;
    }
    time_passed_ = 0;
    timer_running_ = true;
  }
}
