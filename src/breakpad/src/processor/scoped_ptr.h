//  (C) Copyright Greg Colvin and Beman Dawes 1998, 1999.
//  Copyright (c) 2001, 2002 Peter Dimov
//
//  Permission to copy, use, modify, sell and distribute this software
//  is granted provided this copyright notice appears in all copies.
//  This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.
//
//  See http://www.boost.org/libs/smart_ptr/scoped_ptr.htm for documentation.
//

//  scoped_ptr mimics a built-in pointer except that it guarantees deletion
//  of the object pointed to, either on destruction of the scoped_ptr or via
//  an explicit reset(). scoped_ptr is a simple solution for simple needs;
//  use shared_ptr or std::auto_ptr if your needs are more complex.

//  *** NOTE ***
//  If your scoped_ptr is a class member of class FOO pointing to a 
//  forward declared type BAR (as shown below), then you MUST use a non-inlined 
//  version of the destructor.  The destructor of a scoped_ptr (called from
//  FOO's destructor) must have a complete definition of BAR in order to 
//  destroy it.  Example:
//
//  -- foo.h --
//  class BAR;
//
//  class FOO {
//   public:
//    FOO();
//    ~FOO();  // Required for sources that instantiate class FOO to compile!
//    
//   private:
//    scoped_ptr<BAR> bar_;
//  };
//
//  -- foo.cc --
//  #include "foo.h"
//  FOO::~FOO() {} // Empty, but must be non-inlined to FOO's class definition.

//  scoped_ptr_malloc added by Google
//  When one of these goes out of scope, instead of doing a delete or
//  delete[], it calls free().  scoped_ptr_malloc<char> is likely to see
//  much more use than any other specializations.

//  release() added by Google
//  Use this to conditionally transfer ownership of a heap-allocated object
//  to the caller, usually on method success.

#ifndef PROCESSOR_SCOPED_PTR_H__
#define PROCESSOR_SCOPED_PTR_H__

#include <cstddef>            // for std::ptrdiff_t
#include <assert.h>           // for assert
#include <stdlib.h>           // for free() decl

namespace google_breakpad {

template <typename T>
class scoped_ptr {
 private:

  T* ptr;

  scoped_ptr(scoped_ptr const &);
  scoped_ptr & operator=(scoped_ptr const &);

 public:

  typedef T element_type;

  explicit scoped_ptr(T* p = 0): ptr(p) {}

  ~scoped_ptr() {
    typedef char type_must_be_complete[sizeof(T)];
    delete ptr;
  }

  void reset(T* p = 0) {
    typedef char type_must_be_complete[sizeof(T)];

    if (ptr != p) {
      delete ptr;
      ptr = p;
    }
  }

  T& operator*() const {
    assert(ptr != 0);
    return *ptr;
  }

  T* operator->() const  {
    assert(ptr != 0);
    return ptr;
  }

  bool operator==(T* p) const {
    return ptr == p;
  }

  bool operator!=(T* p) const {
    return ptr != p;
  }

  T* get() const  {
    return ptr;
  }

  void swap(scoped_ptr & b) {
    T* tmp = b.ptr;
    b.ptr = ptr;
    ptr = tmp;
  }

  T* release() {
    T* tmp = ptr;
    ptr = 0;
    return tmp;
  }

 private:

  // no reason to use these: each scoped_ptr should have its own object
  template <typename U> bool operator==(scoped_ptr<U> const& p) const;
  template <typename U> bool operator!=(scoped_ptr<U> const& p) const;
};

template<typename T> inline
void swap(scoped_ptr<T>& a, scoped_ptr<T>& b) {
  a.swap(b);
}

template<typename T> inline
bool operator==(T* p, const scoped_ptr<T>& b) {
  return p == b.get();
}

template<typename T> inline
bool operator!=(T* p, const scoped_ptr<T>& b) {
  return p != b.get();
}

//  scoped_array extends scoped_ptr to arrays. Deletion of the array pointed to
//  is guaranteed, either on destruction of the scoped_array or via an explicit
//  reset(). Use shared_array or std::vector if your needs are more complex.

template<typename T>
class scoped_array {
 private:

  T* ptr;

  scoped_array(scoped_array const &);
  scoped_array & operator=(scoped_array const &);

 public:

  typedef T element_type;

  explicit scoped_array(T* p = 0) : ptr(p) {}

  ~scoped_array() {
    typedef char type_must_be_complete[sizeof(T)];
    delete[] ptr;
  }

  void reset(T* p = 0) {
    typedef char type_must_be_complete[sizeof(T)];

    if (ptr != p) {
      delete [] ptr;
      ptr = p;
    }
  }

  T& operator[](std::ptrdiff_t i) const {
    assert(ptr != 0);
    assert(i >= 0);
    return ptr[i];
  }

  bool operator==(T* p) const {
    return ptr == p;
  }

  bool operator!=(T* p) const {
    return ptr != p;
  }

  T* get() const {
    return ptr;
  }

  void swap(scoped_array & b) {
    T* tmp = b.ptr;
    b.ptr = ptr;
    ptr = tmp;
  }

  T* release() {
    T* tmp = ptr;
    ptr = 0;
    return tmp;
  }

 private:

  // no reason to use these: each scoped_array should have its own object
  template <typename U> bool operator==(scoped_array<U> const& p) const;
  template <typename U> bool operator!=(scoped_array<U> const& p) const;
};

template<class T> inline
void swap(scoped_array<T>& a, scoped_array<T>& b) {
  a.swap(b);
}

template<typename T> inline
bool operator==(T* p, const scoped_array<T>& b) {
  return p == b.get();
}

template<typename T> inline
bool operator!=(T* p, const scoped_array<T>& b) {
  return p != b.get();
}


// This class wraps the c library function free() in a class that can be
// passed as a template argument to scoped_ptr_malloc below.
class ScopedPtrMallocFree {
 public:
  inline void operator()(void* x) const {
    free(x);
  }
};

// scoped_ptr_malloc<> is similar to scoped_ptr<>, but it accepts a
// second template argument, the functor used to free the object.

template<typename T, typename FreeProc = ScopedPtrMallocFree>
class scoped_ptr_malloc {
 private:

  T* ptr;

  scoped_ptr_malloc(scoped_ptr_malloc const &);
  scoped_ptr_malloc & operator=(scoped_ptr_malloc const &);

 public:

  typedef T element_type;

  explicit scoped_ptr_malloc(T* p = 0): ptr(p) {}

  ~scoped_ptr_malloc() {
    typedef char type_must_be_complete[sizeof(T)];
    free_((void*) ptr);
  }

  void reset(T* p = 0) {
    typedef char type_must_be_complete[sizeof(T)];

    if (ptr != p) {
      free_((void*) ptr);
      ptr = p;
    }
  }

  T& operator*() const {
    assert(ptr != 0);
    return *ptr;
  }

  T* operator->() const {
    assert(ptr != 0);
    return ptr;
  }

  bool operator==(T* p) const {
    return ptr == p;
  }

  bool operator!=(T* p) const {
    return ptr != p;
  }

  T* get() const {
    return ptr;
  }

  void swap(scoped_ptr_malloc & b) {
    T* tmp = b.ptr;
    b.ptr = ptr;
    ptr = tmp;
  }

  T* release() {
    T* tmp = ptr;
    ptr = 0;
    return tmp;
  }

 private:

  // no reason to use these: each scoped_ptr_malloc should have its own object
  template <typename U, typename GP>
  bool operator==(scoped_ptr_malloc<U, GP> const& p) const;
  template <typename U, typename GP>
  bool operator!=(scoped_ptr_malloc<U, GP> const& p) const;

  static FreeProc const free_;
};

template<typename T, typename FP>
FP const scoped_ptr_malloc<T,FP>::free_ = FP();

template<typename T, typename FP> inline
void swap(scoped_ptr_malloc<T,FP>& a, scoped_ptr_malloc<T,FP>& b) {
  a.swap(b);
}

template<typename T, typename FP> inline
bool operator==(T* p, const scoped_ptr_malloc<T,FP>& b) {
  return p == b.get();
}

template<typename T, typename FP> inline
bool operator!=(T* p, const scoped_ptr_malloc<T,FP>& b) {
  return p != b.get();
}

}  // namespace google_breakpad

#endif  // PROCESSOR_SCOPED_PTR_H__
