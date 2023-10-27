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

#ifndef _cavedb_slice_h_
#define _cavedb_slice_h_

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <string>

#include "config.h"
#include <leveldb/slice.h>

#ifdef ROCKSDB_FOUND
#include <rocksdb/slice.h>
#endif

namespace cavedb {
class Slice;
class Slice2
{
protected:
	const char* data_;
	size_t size_;
public:
	void assign(const char* d, size_t n) { data_ = d;size_= n;}

	const char* data() const { return data_; }
	size_t size() const { return size_; }
	bool empty() const { return size_ == 0; }
	char operator[](size_t n) const {
		assert(n < size());
		return data_[n];
	}
	void clear() { data_ = ""; size_ = 0; }
	void remove_prefix(size_t n) {
		assert(n <= size());
		data_ += n;
		size_ -= n;
	}
	std::string ToString() const { return std::string(data_, size_); }

	operator leveldb::Slice()
	{
		return leveldb::Slice(data_,size_);
	}

	bool operator==(const leveldb::Slice& y) const {
		return ((size() == y.size()) && (memcmp(data(), y.data(), size()) == 0));
	}

	bool operator!=(const leveldb::Slice& y) const {
		return !(*this == y);
	}

	int compare(const Slice2& b) const
	{
		const size_t min_len = (size_ < b.size_) ? size_ : b.size_;
		int r = memcmp(data_, b.data_, min_len);
		if (r == 0) {
			if (size_ < b.size_) r = -1;
			else if (size_ > b.size_) r = +1;
		}
		return r;
	}

	bool starts_with(const Slice2& x) const {
		return ((size_ >= x.size_) && (memcmp(data_, x.data_, x.size_) == 0));
	}
};


class Slice :public Slice2{
 public:
  // Create an empty slice.
  Slice() { data_ = "";size_= 0;}

  Slice(const ::leveldb::Slice& c) { data_ = c.data();size_= c.size();}
#ifdef ROCKSDB_FOUND
  Slice(const ::rocksdb::Slice& c) { data_ = c.data();size_= c.size();}
#endif
  Slice(const Slice2& c) { data_ = c.data();size_= c.size();}

  // Create a slice that refers to d[0,n-1].
  Slice(const char* d, size_t n) { data_ = d;size_= n;}

  // Create a slice that refers to the contents of "s"
  Slice(const std::string& s) { data_ = s.data();size_= s.size();}

  // Create a slice that refers to s[0,strlen(s)-1]
  Slice(const char* s) { data_ = s;size_= strlen(s);}

};


}  // namespace leveldb
#endif  // STORAGE_LEVELDB_INCLUDE_SLICE_H_
