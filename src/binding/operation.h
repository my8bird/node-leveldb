#ifndef NODE_LEVELDB_OP_H_
#define NODE_LEVELDB_OP_H_

#include <node.h>
#include <v8.h>

#include "node_async_shim.h"

using namespace v8;
using namespace node;

namespace node_leveldb {

template < class T > class Operation {
 public:

  typedef void (*ExecFunction)(T* op);
  typedef void (*ConvFunction)(
    T* op, Handle<Value>& error, Handle<Value>& result);

  inline Operation(const ExecFunction exec, const ConvFunction conv,
                   Handle<Object>& self, Handle<Function>& callback)
    : exec_(exec), conv_(conv)
  {
    handle_ = Persistent<Object>::New(self);
    callback_ = Persistent<Function>::New(callback);
  }

  virtual ~Operation() {
    handle_.Dispose();
    callback_.Dispose();
  }

  virtual inline Handle<Value> BeforeRun() { return Handle<Value>(); }
  virtual inline void AfterExecute() {}
  virtual inline void BeforeReturn() {}

  inline Handle<Value> Run() {
    Handle<Value> error = BeforeRun();

    if (!error.IsEmpty()) {

      delete this;
      return error;

    } else if (callback_.IsEmpty()) {

      Handle<Value> result = Null();

      T* self = static_cast<T*>(this);
      exec_(self);
      AfterExecute();

      conv_(self, error, result);
      BeforeReturn();

      delete this;

      return error.IsEmpty() ? result : ThrowException(error);

    } else {

      BEGIN_ASYNC(this, Async, After);
      return Undefined();

    }
  }

  static inline Handle<Value> Go(const ExecFunction run,
                                 const ConvFunction conv,
                                 const Arguments& args)
  {
    HandleScope scope;
    return New(run, conv, args)->Run();
  }

  static inline T* New(const ExecFunction run,
                       const ConvFunction conv,
                       const Arguments& args)
  {
    Handle<Object> self = args.This();
    Handle<Function> callback = GetCallback(args);
    return new T(run, conv, self, callback);
  }

  static async_rtn Async(uv_work_t* req) {
    Operation* op = static_cast<Operation*>(req->data);
    T* self = static_cast<T*>(req->data);
    op->exec_(self);
    op->AfterExecute();
    RETURN_ASYNC;
  }

  static async_rtn After(uv_work_t* req) {
    HandleScope scope;

    Handle<Value> error = Null();
    Handle<Value> result = Null();

    Operation* op = static_cast<Operation*>(req->data);
    T* self = static_cast<T*>(req->data);

    op->conv_(self, error, result);
    op->BeforeReturn();

    TryCatch tryCatch;

    assert(!op->callback_.IsEmpty());
    Handle<Value> argv[] = { error, result };
    op->callback_->Call(op->handle_, 2, argv);

    if (tryCatch.HasCaught()) FatalException(tryCatch);

    delete op;

    RETURN_ASYNC_AFTER;
  }

  ExecFunction exec_;
  ConvFunction conv_;

  Persistent<Object> handle_;
  Persistent<Function> callback_;
};

} // node_leveldb

#endif // NODE_LEVELDB_OP_H_
