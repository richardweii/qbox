// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// Slice is a simple structure containing a pointer into some external
// storage and a size.  The user of a Slice must ensure that the slice
// is not used after the corresponding external storage has been
// deallocated.
//
// Multiple threads can invoke const methods on a Slice without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same Slice must use
// external synchronization.
#ifndef __SLICE_H_
#define __SLICE_H_

#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>

// 对字符串的只读引用
class Slice {
public:
  // Create an empty slice.
  Slice() : _data(""), _size(0) {}

  // Create a slice that refers to d[0,n-1].
  Slice(const char *d, size_t n) : _data(d), _size(n) {}

  // Create a slice that refers to the contents of "s"
  Slice(const std::string &s) : _data(s.data()), _size(s.size()) {}

  // Create a slice that refers to s[0,strlen(s)-1]
  Slice(const char *s) : _data(s), _size(strlen(s)) {}

  // Intentionally copyable.
  Slice(const Slice &) = default;
  Slice &operator=(const Slice &) = default;

  // Return a pointer to the beginning of the referenced data
  const char *data() const { return _data; }

  // Return the length (in bytes) of the referenced data
  size_t size() const { return _size; }

  // Return true iff the length of the referenced data is zero
  bool empty() const { return _size == 0; }

  // Return the ith byte in the referenced data.
  // REQUIRES: n < size()
  char operator[](size_t n) const {
    assert(n < size());
    return _data[n];
  }

  // Change this slice to refer to an empty array
  void clear() {
    _data = "";
    _size = 0;
  }

  // Drop the first "n" bytes from this slice.
  void removePrefix(size_t n) {
    assert(n <= size());
    _data += n;
    _size -= n;
  }

  // Return a string that contains the copy of the referenced data.
  std::string toString() const { return std::string(_data, _size); }

  // Three-way comparison.  Returns value:
  //   <  0 iff "*this" <  "b",
  //   == 0 iff "*this" == "b",
  //   >  0 iff "*this" >  "b"
  int compare(const Slice &b) const;

  // Return true iff "x" is a prefix of "*this"
  bool startsWith(const Slice &x) const {
    return ((_size >= x._size) && (memcmp(_data, x._data, x._size) == 0));
  }

private:
  const char *_data;
  size_t _size;
};

inline bool operator==(const Slice &x, const Slice &y) {
  return ((x.size() == y.size()) &&
          (memcmp(x.data(), y.data(), x.size()) == 0));
}

inline bool operator!=(const Slice &x, const Slice &y) { return !(x == y); }

inline int Slice::compare(const Slice &b) const {
  const size_t min_len = (_size < b._size) ? _size : b._size;
  int r = memcmp(_data, b._data, min_len);
  if (r == 0) {
    if (_size < b._size)
      r = -1;
    else if (_size > b._size)
      r = +1;
  }
  return r;
}

#endif