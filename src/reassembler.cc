#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  const uint64_t next_expected_index = output_.writer().bytes_pushed();
  const uint64_t capacity = output_.writer().available_capacity();
  const uint64_t first_unacceptable_index = next_expected_index + capacity;

  // 1.Data adjust.
  // Omit empty data can't be EOF.
  if ( capacity == 0 ) {
    if ( first_index == next_expected_index && data.empty() ) {
      // Allow to pass.
    } else {
      return;
    }
  }
  // Omit data out of range.
  else if ( first_index >= first_unacceptable_index ) {
    return;
  }

  // Cut the part out of capacity range.
  if ( first_index < next_expected_index ) {
    const uint64_t offset = next_expected_index - first_index;

    // Check if data out of date.
    if ( data.length() < offset || ( data.length() == offset && !is_last_substring ) ) {
      return;
    }

    data = data.substr( offset );
    first_index = next_expected_index;
  }

  if ( first_index + data.length() > first_unacceptable_index ) {
    const uint64_t allowed_length = first_unacceptable_index - first_index;
    data = data.substr( 0, allowed_length );

    is_last_substring = false;
  }

  // 2.Check EOF, if data after cut is still last substr.
  if ( is_last_substring ) {
    eof_saved_ = true;
    eof_index_ = first_index + data.length();
  }

  // 3.Add the data to local memory.
  auto iter = buffer_.lower_bound( first_index );

  if ( iter != buffer_.begin() ) {
    auto prev_iter = prev( iter );
    const uint64_t prev_first = prev_iter->first;
    const uint64_t prev_last = prev_first + prev_iter->second.length();

    if ( prev_last >= first_index ) {
      iter = prev_iter;
    }
  }

  while ( iter != buffer_.end() && iter->first <= first_index + data.length() ) {
    const uint64_t iter_first = iter->first;
    const uint64_t iter_last = iter_first + iter->second.length();
    const uint64_t data_last = first_index + data.length();

    if ( iter_first < first_index ) {
      data.insert( 0, iter->second.substr( 0, first_index - iter_first ) );
      first_index = iter_first;
    }

    if ( iter_last > data_last ) {
      data += iter->second.substr( data_last - iter_first );
    }

    pending_bytes_ -= iter->second.length();
    iter = buffer_.erase( iter );
  }

  buffer_[first_index] = data;
  pending_bytes_ += data.length();

  // 4.Push possible data to stream.
  push_to_stream();
}

void Reassembler::push_to_stream()
{
  uint64_t next_expected_index = output_.writer().bytes_pushed();

  // Loop check if data can push to writer.
  while ( !buffer_.empty() && buffer_.begin()->first == next_expected_index ) {
    auto begin_iter = buffer_.begin();

    output_.writer().push( begin_iter->second );
    pending_bytes_ -= begin_iter->second.length();
    next_expected_index += begin_iter->second.length();
    buffer_.erase( begin_iter );
  }

  // Check if pushed all data.
  if ( eof_saved_ && output_.writer().bytes_pushed() == eof_index_ ) {
    output_.writer().close();
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  return pending_bytes_;
}
