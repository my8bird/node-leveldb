#ifndef DB_H_
#define DB_H_

#include <v8.h>
#include <node.h>

#include "leveldb/db.h"
#include "WriteBatch.h"

#include "helpers.h"

using namespace v8;
using namespace node;

namespace node_leveldb {

class DB : ObjectWrap {
public:
  DB();
  virtual ~DB();

  static void Init(Handle<Object> target);

  static bool HasInstance(Handle<Value> val);

  void Close();

private:
  static Handle<Value> New(const Arguments& args);

  static Handle<Value> Open(const Arguments& args);
  static Handle<Value> Close(const Arguments& args);

  static Handle<Value> Put(const Arguments& args);
  static Handle<Value> Del(const Arguments& args);
  static Handle<Value> Write(const Arguments& args);

  static Handle<Value> Get(const Arguments& args);

  static void unrefIterator(Persistent<Value> object, void* parameter);
  static Handle<Value> NewIterator(const Arguments& args);

  static Handle<Value> GetSnapshot(const Arguments& args);
  static Handle<Value> ReleaseSnapshot(const Arguments& args);

  static Handle<Value> GetProperty(const Arguments& args);
  static Handle<Value> GetApproximateSizes(const Arguments& args);

  static Handle<Value> DestroyDB(const Arguments& args);
  static Handle<Value> RepairDB(const Arguments& args);


  struct Params {
    Params(DB* self, Handle<Function> cb);
    virtual ~Params();
    virtual void Callback(Handle<Value> result = Handle<Value>());

    DB* self;
    Persistent<Function> callback;
    leveldb::Status status;
  };

  struct OpenParams : Params {
    OpenParams(DB *self, std::string name, leveldb::Options &options, Handle<Function> callback)
      : Params(self, callback), name(name), options(options) {}

    std::string name;
    leveldb::Options options;
  };

  struct ReadParams : Params {
    ReadParams(DB *self, leveldb::ReadOptions &options, bool asBuffer, Handle<Function> callback)
    : Params(self, callback), key(NULL), keyLen(0), options(options), asBuffer(asBuffer) {}

    char *key;
    int keyLen;
    Persistent<Object> keyBuf;
    leveldb::ReadOptions options;
    bool asBuffer;
    std::string result;
  };

  struct WriteParams : Params {
    WriteParams(DB *self, WriteBatch *writeBatch, leveldb::WriteOptions &options, Handle<Function> callback)
      : Params(self, callback), writeBatch(writeBatch), options(options), disposeWriteBatch(false) {}

    WriteBatch *writeBatch;
    leveldb::WriteOptions options;
    bool disposeWriteBatch;   // when set, EIO_AfterWrite will delete the writeBatch
  };

  static void EIO_BeforeOpen(OpenParams *params);
  static eio_return_type EIO_Open(eio_req *req);
  static int EIO_AfterOpen(eio_req *req);
  
  static void EIO_BeforeClose(Params *params);
  static eio_return_type EIO_Close(eio_req *req);
  static int EIO_AfterClose(eio_req *req);
  
  static void EIO_BeforeRead(ReadParams *params);
  static eio_return_type EIO_Read(eio_req *req);
  static int EIO_AfterRead(eio_req *req);
  
  static void EIO_BeforeWrite(WriteParams *params);
  static eio_return_type EIO_Write(eio_req *req);
  static int EIO_AfterWrite(eio_req *req);
  
  leveldb::DB* db;
  static Persistent<FunctionTemplate> persistent_function_template;
  std::vector< Persistent<Object> > iteratorList;
};

} // node_leveldb

#endif  // DB_H_
