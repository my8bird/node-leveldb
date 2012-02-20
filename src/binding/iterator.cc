#include <assert.h>
#include <pthread.h>

#include <leveldb/iterator.h>
#include <node.h>
#include <v8.h>

#include "handle.h"
#include "helpers.h"
#include "iterator.h"
#include "operation.h"

namespace node_leveldb {

Persistent<FunctionTemplate> JIterator::constructor;

void JIterator::Initialize(Handle<Object> target) {
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  constructor = Persistent<FunctionTemplate>::New(t);
  constructor->InstanceTemplate()->SetInternalFieldCount(1);
  constructor->SetClassName(String::NewSymbol("Iterator"));

  NODE_SET_PROTOTYPE_METHOD(constructor, "valid", Valid);
  NODE_SET_PROTOTYPE_METHOD(constructor, "first", First);
  NODE_SET_PROTOTYPE_METHOD(constructor, "last", Last);
  NODE_SET_PROTOTYPE_METHOD(constructor, "seek", Seek);
  NODE_SET_PROTOTYPE_METHOD(constructor, "next", Next);
  NODE_SET_PROTOTYPE_METHOD(constructor, "prev", Prev);
  NODE_SET_PROTOTYPE_METHOD(constructor, "key", GetKey);
  NODE_SET_PROTOTYPE_METHOD(constructor, "value", GetValue);
  NODE_SET_PROTOTYPE_METHOD(constructor, "current", GetKeyValue);
}

Handle<Value> JIterator::New(const Arguments& args) {
  HandleScope scope;

  assert(args.Length() == 1);
  assert(args[0]->IsExternal());

  leveldb::Iterator* it =
    static_cast<leveldb::Iterator*>(External::Unwrap(args[0]));

  assert(it);

  JIterator* iterator = new JIterator(it);
  iterator->Wrap(args.This());

  return args.This();
}

Handle<Value> JIterator::Valid(const Arguments& args) {
  HandleScope scope;
  JIterator* self = ObjectWrap::Unwrap<JIterator>(args.This());

  if (self->Lock()) return ThrowError("Concurrent operations not supported");
  bool valid = self->it_ != NULL && self->it_->Valid();
  self->Unlock();

  return valid ? True() : False();
}

Handle<Value> JIterator::Seek(const Arguments& args) {
  HandleScope scope;

  // Required key
  if (args.Length() < 1 || !Buffer::HasInstance(args[0]))
    return ThrowTypeError("Invalid arguments");

  Op* op = Op::New(Seek, Conv, args);
  op->key_ = ToSlice(args[0], op->keyHandle_);

  return op->Run();
}

Handle<Value> JIterator::First(const Arguments& args) {
  return Op::Go(First, Conv, args);
}

Handle<Value> JIterator::Last(const Arguments& args) {
  return Op::Go(Last, Conv, args);
}

Handle<Value> JIterator::Next(const Arguments& args) {
  return Op::Go(Next, Conv, args);
}

Handle<Value> JIterator::Prev(const Arguments& args) {
  return Op::Go(Prev, Conv, args);
}

Handle<Value> JIterator::GetKey(const Arguments& args) {
  return Op::Go(GetKey, Conv, args);
}

Handle<Value> JIterator::GetValue(const Arguments& args) {
  return Op::Go(GetValue, Conv, args);
}

Handle<Value> JIterator::GetKeyValue(const Arguments& args) {
  return Op::Go(GetKeyValue, Conv, args);
}


//
// ASYNC FUNCTIONS
//


void JIterator::Seek(Op* op) {
  leveldb::Iterator* it = op->self_->it_;
  it->Seek(op->key_);
  op->status_ = it->status();
}

void JIterator::First(Op* op) {
  leveldb::Iterator* it = op->self_->it_;
  it->SeekToFirst();
  op->status_ = it->status();
}

void JIterator::Last(Op* op) {
  leveldb::Iterator* it = op->self_->it_;
  it->SeekToLast();
  op->status_ = it->status();
}

void JIterator::Next(Op* op) {
  leveldb::Iterator* it = op->self_->it_;
  if (it->Valid()) {
    it->Next();
    op->status_ = it->status();
  } else {
    op->invalidState_ = true;
  }
}

void JIterator::Prev(Op* op) {
  leveldb::Iterator* it = op->self_->it_;
  if (it->Valid()) {
    it->Prev();
    op->status_ = it->status();
  } else {
    op->invalidState_ = true;
  }
}

void JIterator::GetKey(Op* op) {
  leveldb::Iterator* it = op->self_->it_;
  if (it->Valid()) {
    op->value_ = it->key();
  } else {
    op->status_ = it->status();
  }
}

void JIterator::GetValue(Op* op) {
  leveldb::Iterator* it = op->self_->it_;
  if (it->Valid()) {
    op->value_ = it->value();
  } else {
    op->status_ = it->status();
  }
}

void JIterator::GetKeyValue(Op* op) {
  leveldb::Iterator* it = op->self_->it_;
  if (it->Valid()) {
    op->key_ = it->key();
    op->value_ = it->value();
  } else {
    op->status_ = it->status();
  }
}

void JIterator::Conv(Op* op, Handle<Value>& error, Handle<Value>& result) {
  if (op->invalidState_) {
    error = Exception::Error(String::New("Illegal state"));
  } else if (!op->status_.ok()) {
    error = Exception::Error(String::New(op->status_.ToString().c_str()));
  } else if (!op->value_.empty()) {
    if (!op->key_.empty()) {
      Local<Array> array = Array::New(2);
      array->Set(0, ToBuffer(op->key_));
      array->Set(1, ToBuffer(op->value_));
      result = array;
    } else {
      result = ToBuffer(op->value_);
    }
  }
}

} // node_leveldb
