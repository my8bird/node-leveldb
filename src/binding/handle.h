#ifndef NODE_LEVELDB_HANDLE_H_
#define NODE_LEVELDB_HANDLE_H_

#include <assert.h>

#include <vector>
#include <string>

#include <leveldb/db.h>
#include <node.h>
#include <v8.h>

#include "batch.h"
#include "iterator.h"
#include "node_async_shim.h"

using namespace node;
using namespace v8;

namespace node_leveldb {

class JHandle : public ObjectWrap {
 public:
  static Persistent<FunctionTemplate> constructor;
  static void Initialize(Handle<Object> target);

  static Handle<Value> New(const Arguments& args);

  static Handle<Value> Open(const Arguments& args);
  static Handle<Value> Destroy(const Arguments& args);
  static Handle<Value> Repair(const Arguments& args);

  static Handle<Value> Read(const Arguments& args);
  static Handle<Value> Write(const Arguments& args);

  static Handle<Value> Iterator(const Arguments& args);
  static Handle<Value> Snapshot(const Arguments& args);

  static Handle<Value> Property(const Arguments& args);
  static Handle<Value> ApproximateSizes(const Arguments& args);
  static Handle<Value> CompactRange(const Arguments& args);

 private:
  JHandle(leveldb::DB* db) : ObjectWrap(), db_(db) {}

  ~JHandle() {
    if (db_ != NULL) {
      delete db_;
      db_ = NULL;
    }
    comparator_.Dispose();
  };

  inline bool Valid() {
    return db_ != NULL;
  };

  inline Handle<Value> RefIterator(leveldb::Iterator* it) {
    Local<Value> argv[] = { External::New(it) };
    Local<Object> instance = JIterator::constructor->GetFunction()->NewInstance(1, argv);

    // Keep a weak reference
    Persistent<Object> weak = Persistent<Object>::New(instance);
    weak.MakeWeak(this, &UnrefIterator);

    Ref();

    return instance;
  }

  inline Handle<Value> RefSnapshot(const leveldb::Snapshot* snapshot) {
    Local<Value> instance = External::New((void*)snapshot);
    Persistent<Value> weak = Persistent<Value>::New(instance);
    weak.MakeWeak(this, &UnrefSnapshot);
    Ref();
    return instance;
  }

  static void UnrefIterator(Persistent<Value> object, void* parameter);
  static void UnrefSnapshot(Persistent<Value> object, void* parameter);

  template <class T>
  class Op : public Operation<T> {
   public:

    typedef void (*ExecFunction)(T* op);
    typedef void (*ConvFunction)(
      T* op, Handle<Value>& error, Handle<Value>& result);

    inline Op(const ExecFunction exec, const ConvFunction conv,
              Handle<Object>& handle, Handle<Function>& callback)
      : Operation<T>(exec, conv, handle, callback)
    {
      self_ = ObjectWrap::Unwrap<JHandle>(handle);
      self_->Ref();
    }

    virtual ~Op() {
      self_->Unref();
    }

    virtual inline Handle<Value> BeforeRun() {
      if (self_->db_ == NULL) return ThrowError("Handle closed");
      return Handle<Value>();
    }

    JHandle* self_;

    leveldb::Status status_;

  };

  class OpenOp;
  class OpenOp : public Operation<OpenOp> {
   public:

    inline OpenOp(const ExecFunction exec, const ConvFunction conv,
                  Handle<Object>& handle, Handle<Function>& callback)
      : Operation<OpenOp>(exec, conv, handle, callback), db_(NULL) {}

    std::string name_;

    leveldb::Options options_;
    leveldb::Status status_;
    leveldb::DB* db_;

    Persistent<Value> comparator_;
  };

  class ReadOp;
  class ReadOp : public Op<ReadOp> {
   public:

    inline ReadOp(const ExecFunction exec, const ConvFunction conv,
                  Handle<Object>& handle, Handle<Function>& callback)
      : Op<ReadOp>(exec, conv, handle, callback), result_(NULL) {}

    virtual ~ReadOp() {
      keyHandle_.Dispose();
    }

    leveldb::Slice key_;
    leveldb::ReadOptions options_;

    std::string* result_;

    Persistent<Value> keyHandle_;
  };

  class WriteOp;
  class WriteOp : public Op<WriteOp> {
   public:

    inline WriteOp(const ExecFunction exec, const ConvFunction conv,
                   Handle<Object>& handle, Handle<Function>& callback)
      : Op<WriteOp>(exec, conv, handle, callback) {}

    virtual ~WriteOp() {
      batch_->Unref();
    }

    virtual inline Handle<Value> BeforeRun() {
      assert(batch_);
      batch_->Ref();
      return Op<WriteOp>::BeforeRun();
    }

    JBatch* batch_;
    leveldb::WriteOptions options_;
  };

  class IteratorOp;
  class IteratorOp : public Op<IteratorOp> {
   public:

    inline IteratorOp(const ExecFunction exec, const ConvFunction conv,
                      Handle<Object>& handle, Handle<Function>& callback)
      : Op<IteratorOp>(exec, conv, handle, callback) {}

    leveldb::Iterator* it_;
    leveldb::ReadOptions options_;
  };

  class SnapshotOp;
  class SnapshotOp : public Op<SnapshotOp> {
   public:

    inline SnapshotOp(const ExecFunction exec, const ConvFunction conv,
                      Handle<Object>& handle, Handle<Function>& callback)
      : Op<SnapshotOp>(exec, conv, handle, callback) {}

    leveldb::Snapshot* snap_;
  };

  class PropertyOp;
  class PropertyOp : public Op<PropertyOp> {
   public:

    inline PropertyOp(const ExecFunction exec, const ConvFunction conv,
                      Handle<Object>& handle, Handle<Function>& callback)
      : Op<PropertyOp>(exec, conv, handle, callback), hasProperty_(false) {}

    std::string name_;
    std::string value_;
    bool hasProperty_;
  };

  class ApproximateSizesOp;
  class ApproximateSizesOp : public Op<ApproximateSizesOp> {
   public:

    inline ApproximateSizesOp(const ExecFunction exec, const ConvFunction conv,
                              Handle<Object>& handle, Handle<Function>& callback)
      : Op<ApproximateSizesOp>(exec, conv, handle, callback), sizes_(NULL) {}

    virtual ~ApproximateSizesOp() {
      std::vector< Persistent<Value> >::iterator it;
      for (it = handles_.begin(); it < handles_.end(); ++it) it->Dispose();
      if (sizes_) delete[] sizes_;
    }

    std::vector<leveldb::Range> ranges_;
    std::vector< Persistent<Value> > handles_;

    uint64_t* sizes_;
  };

  static void Open(OpenOp* op);
  static void Open(OpenOp* op, Handle<Value>& error, Handle<Value>& result);

  static void Destroy(OpenOp* op);
  static void Repair(OpenOp* op);

  static void OpenConv(OpenOp* op, Handle<Value>& error, Handle<Value>& result);

  static void Read(ReadOp* op);
  static void Read(ReadOp* op, Handle<Value>& error, Handle<Value>& result);

  static void Write(WriteOp* op);
  static void Write(WriteOp* op, Handle<Value>& error, Handle<Value>& result);

  static void Iterator(IteratorOp* op);
  static void Iterator(IteratorOp* op,
                       Handle<Value>& error, Handle<Value>& result);

  static void Snapshot(SnapshotOp* op);
  static void Snapshot(SnapshotOp* op,
                       Handle<Value>& error, Handle<Value>& result);

  static void Property(PropertyOp* op);
  static void Property(PropertyOp* op,
                       Handle<Value>& error, Handle<Value>& result);

  static void ApproximateSizes(ApproximateSizesOp* op);
  static void ApproximateSizes(ApproximateSizesOp* op,
                               Handle<Value>& error, Handle<Value>& result);

  leveldb::DB* db_;
  Persistent<Value> comparator_;
};

} // namespace node_leveldb

#endif // NODE_LEVELDB_HANDLE_H_
