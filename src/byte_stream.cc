#include "byte_stream.hh"
#include "debug.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

// Push data to stream, but only as much as available capacity allows.
void Writer::push( string data )
{
  if ( is_closed_ || data.empty() || error_ ) {
    return;
  }

  uint64_t can_write = available_capacity();
  if ( can_write == 0 ) {
    return;
  }

  if ( data.size() > can_write ){
    data.resize( can_write );
  }

  uint64_t write_len = data.size();
  bytes_pushed_ += write_len;
  bytes_buffered_ += write_len;
  buffer_.push_back( std::move( data ) );
}

// Signal that the stream has reached its ending. Nothing more will be written.
void Writer::close()
{
  is_closed_ = true;
}

// Has the stream been closed?
bool Writer::is_closed() const
{
  return is_closed_;
}

// How many bytes can be pushed to the stream right now?
uint64_t Writer::available_capacity() const
{
  return capacity_ - bytes_buffered_;
}

// Total number of bytes cumulatively pushed to the stream
uint64_t Writer::bytes_pushed() const
{
  return bytes_pushed_;
}

// Peek at the next bytes in the buffer -- ideally as many as possible.
// It's not required to return a string_view of the *whole* buffer, but
// if the peeked string_view is only one byte at a time, it will probably force
// the caller to do a lot of extra work.
string_view Reader::peek() const
{
  if ( buffer_.empty() ) {
    return {};
  }

  return string_view ( buffer_.front() ).substr( removed_on_front_ );
}

// Remove `len` bytes from the buffer.
void Reader::pop( uint64_t len )
{
  if ( len == 0 || error_ ) return;
  uint64_t actual_pop_len = std::min( len, bytes_buffered_ );

  bytes_popped_ += actual_pop_len;
  bytes_buffered_ -= actual_pop_len;

  while ( actual_pop_len > 0 ) {
    uint64_t current_block_len = buffer_.front().size() - removed_on_front_;

    if ( actual_pop_len < current_block_len) {
      removed_on_front_ += actual_pop_len;
      break;
    } else {
      actual_pop_len -= current_block_len;
      buffer_.pop_front();
      removed_on_front_ = 0;
    }
  }
}

// Is the stream finished (closed and fully popped)?
bool Reader::is_finished() const
{
  return is_closed_ && (bytes_buffered_ == 0);
}

// Number of bytes currently buffered (pushed and not popped)
uint64_t Reader::bytes_buffered() const
{
  return bytes_buffered_;
}

// Total number of bytes cumulatively popped from stream
uint64_t Reader::bytes_popped() const
{
  return bytes_popped_;
}
