#ifndef NODE_LEVELDB_ITERATOR_H_
#define NODE_LEVELDB_ITERATOR_H_

#include <assert.h>
#include <errno.h>
#include <pthread.h>

#include <leveldb/iterator.h>
#include <node.h>
#include <v8.h>

#include "helpers.h"
#include "operation.h"

using namespace v8;
using namespace node;

namespace node_leveldb {

class JHandle;

class JIterator : ObjectWrap {
 public:
  virtual ~JIterator() {
    if (it_) {
      delete it_;
      it_ = NULL;
    }
    int err = pthread_mutex_destroy(&lock_);
    assert(err == 0);
  }

  static Persistent<FunctionTemplate> constructor;

  static void Initialize(Handle<Object> target);
  static Handle<Value> New(const Arguments& args);

  static Handle<Value> Valid(const Arguments& args);
  static Handle<Value> Seek(const Arguments& args);
  static Handle<Value> First(const Arguments& args);
  static Handle<Value> Last(const Arguments& args);
  static Handle<Value> Next(const Arguments& args);
  static Handle<Value> Prev(const Arguments& args);

  static Handle<Value> GetKey(const Arguments& args);
  static Handle<Value> GetValue(const Arguments& args);
  static Handle<Value> GetKeyValue(const Arguments& args);

 private:
  friend class JHandle;

  // No instance creation outside of Handle
  inline JIterator(leveldb::Iterator* it) : ObjectWrap(), it_(it) {
    int err = pthread_mutex_init(&lock_, NULL);
    assert(err == 0);
  }

  // No copying allowed
  JIterator(const JIterator&);
  void operator=(const JIterator&);

  inline bool Lock() {
    int err = pthread_mutex_trylock(&lock_);
    assert(err == 0 || err == EBUSY);
    return err == EBUSY;
  }

  inline void Unlock() {
    int err = pthread_mutex_unlock(&lock_);
    assert(err == 0);
  }

  class Op;
  class Op : public Operation<Op> {
   public:

    inline Op(const ExecFunction exec, const ConvFunction conv,
              Handle<Object>& handle, Handle<Function>& callback)
      : Operation<Op>(exec, conv, handle, callback), invalidState_(0)
    {
      self_ = ObjectWrap::Unwrap<JIterator>(handle);
      self_->Ref();
    }

    virtual ~Op() {
      self_->Unref();
      keyHandle_.Dispose();
    }

    virtual inline Handle<Value> BeforeRun() {
      if (self_->Lock()) return ThrowError("Concurrent operations not supported");
      return Handle<Value>();
    }

    virtual inline void BeforeReturn() {
      self_->Unlock();
    }

    JIterator* self_;

    leveldb::Status status_;
    leveldb::Slice key_;
    leveldb::Slice value_;

    Persistent<Value> keyHandle_;

    bool invalidState_;
  };

  static void Seek(Op* op);
  static void First(Op* op);
  static void Last(Op* op);
  static void Next(Op* op);
  static void Prev(Op* op);

  static void GetKey(Op* op);
  static void GetValue(Op* op);
  static void GetKeyValue(Op* op);

  static void Conv(Op* op, Handle<Value>& error, Handle<Value>& result);

  leveldb::Iterator* it_;
  pthread_mutex_t lock_;
};

} // node_leveldb

#endif // NODE_LEVELDB_ITERATOR_H_
