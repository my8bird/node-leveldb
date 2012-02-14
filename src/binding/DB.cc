#include <stdlib.h>

#include <algorithm>
#include <sstream>
#include <vector>

#include <node_buffer.h>

#include "DB.h"
#include "helpers.h"
#include "Iterator.h"
#include "options.h"
#include "snapshot.h"
#include "WriteBatch.h"

#define CHECK_VALID_STATE                                               \
  if (self->db == NULL) {                                               \
    return ThrowError("Illegal state: DB.open() has not been called");  \
  }

namespace node_leveldb {

Persistent<FunctionTemplate> DB::persistent_function_template;

DB::DB() : db(NULL) {}

DB::~DB() {
  CloseIterators();
  DisposeIterators();
  Close();
}

void DB::Init(Handle<Object> target) {
  HandleScope scope; // used by v8 for garbage collection

  // Our constructor
  Local<FunctionTemplate> local_function_template = FunctionTemplate::New(New);
  persistent_function_template = Persistent<FunctionTemplate>::New(local_function_template);
  persistent_function_template->InstanceTemplate()->SetInternalFieldCount(1);
  persistent_function_template->SetClassName(String::NewSymbol("DB"));

  // Instance methods
  NODE_SET_PROTOTYPE_METHOD(persistent_function_template, "open", Open);
  NODE_SET_PROTOTYPE_METHOD(persistent_function_template, "close", Close);
  NODE_SET_PROTOTYPE_METHOD(persistent_function_template, "put", Put);
  NODE_SET_PROTOTYPE_METHOD(persistent_function_template, "del", Del);
  NODE_SET_PROTOTYPE_METHOD(persistent_function_template, "write", Write);
  NODE_SET_PROTOTYPE_METHOD(persistent_function_template, "get", Get);
  NODE_SET_PROTOTYPE_METHOD(persistent_function_template, "newIterator", NewIterator);
  NODE_SET_PROTOTYPE_METHOD(persistent_function_template, "getSnapshot", GetSnapshot);
  NODE_SET_PROTOTYPE_METHOD(persistent_function_template, "getProperty", GetProperty);
  NODE_SET_PROTOTYPE_METHOD(persistent_function_template, "getApproximateSizes", GetApproximateSizes);
  NODE_SET_PROTOTYPE_METHOD(persistent_function_template, "compactRange", CompactRange);

  // Static methods
  NODE_SET_METHOD(persistent_function_template, "destroyDB", DestroyDB);
  NODE_SET_METHOD(persistent_function_template, "repairDB", RepairDB);

  // Binding our constructor function to the target variable
  target->Set(String::NewSymbol("DB"), persistent_function_template->GetFunction());

  // Set version
  std::stringstream version;
  version << leveldb::kMajorVersion << "." << leveldb::kMinorVersion;
  target->Set(String::New("bindingVersion"),
              String::New(version.str().c_str()),
              static_cast<v8::PropertyAttribute>(v8::ReadOnly|v8::DontDelete));
}

bool DB::HasInstance(Handle<Value> value) {
  return value->IsObject() &&
    persistent_function_template->HasInstance(value->ToObject());
}

leveldb::DB* DB::operator*() {
  return db;
}


//
// Constructor
//

Handle<Value> DB::New(const Arguments& args) {
  HandleScope scope;

  DB* self = new DB();
  self->Wrap(args.This());

  return args.This();
}


//
// Open
//

#define USAGE_ERROR(msg) \
  return ThrowTypeError(msg ": DB.open(<filename>, <options?>, <callback?>)");

Handle<Value> DB::Open(const Arguments& args) {
  HandleScope scope;

  // Get this and arguments
  DB* self = ObjectWrap::Unwrap<DB>(args.This());
  int argv = args.Length();

  if (argv < 1)
    USAGE_ERROR("Invalid number of arguments");

  if (!args[0]->IsString())
    USAGE_ERROR("Argument 1 must be a string");

  // Required filename
  String::Utf8Value name(args[0]);

  // Optional options
  leveldb::Options options = leveldb::Options();
  if (argv > 1 && args[1]->IsObject() && !args[1]->IsFunction())
    Options::Parse(args[1], options);

  // Optional callback
  Local<Function> callback = GET_CALLBACK_ARG(args, argv);

  // Pass parameters to async function
  OpenParams *params = new OpenParams(self, *name, options, callback);
  EIO_BeforeOpen(params);

  return args.This();
}

#undef USAGE_ERROR

void DB::EIO_BeforeOpen(OpenParams *params) {
  eio_custom(EIO_Open, EIO_PRI_DEFAULT, EIO_AfterOpen, params);
}

EIO_RETURN_TYPE DB::EIO_Open(eio_req *req) {
  OpenParams *params = static_cast<OpenParams*>(req->data);
  DB *self = params->self;

  // If open() is called more than once, we first need to close the old db
  self->CloseIterators();
  self->Close();

  // Do the actual work
  params->status = leveldb::DB::Open(params->options, params->name, &self->db);

  EIO_RETURN_STMT;
}

int DB::EIO_AfterOpen(eio_req *req) {
  OpenParams *params = static_cast<OpenParams*>(req->data);

  // If we closed any iterators in the asynchronous process, we can dispose of
  // those handles now.
  params->self->DisposeIterators();

  params->Callback();
  delete params;

  return 0;
}


//
// Close
//

void DB::Close() {
  if (db != NULL) {
    delete db;
    db = NULL;
  }
};

//
// CloseIterators
//

void DB::CloseIterators() {
  // Free iterator objects, this function should run asynchronously.
  if (db != NULL) {
    std::vector< Persistent<Object> >::iterator it;

    for (it = iteratorList.begin(); it != iteratorList.end(); it++) {
      Iterator *iterator = ObjectWrap::Unwrap<Iterator>(*it);
      if (iterator) iterator->Close();
    }
  }
}

//
// Dispose Iterators
//

void DB::DisposeIterators() {
  // Dispose iterator handles, this function MUST run in the main thread.
  if (db != NULL) {
    std::vector< Persistent<Object> >::iterator it;

    for (it = iteratorList.begin(); it != iteratorList.end(); it++) {
      it->Dispose();
      it->Clear();
    }

    iteratorList.clear();
  }
}

Handle<Value> DB::Close(const Arguments& args) {
  HandleScope scope;

  // Get this and arguments
  DB* self = ObjectWrap::Unwrap<DB>(args.This());
  int argv = args.Length();

  // Optional callback
  Local<Function> callback = GET_CALLBACK_ARG(args, argv);

  Params *params = new Params(self, callback);
  EIO_BeforeClose(params);

  return args.This();
}

void DB::EIO_BeforeClose(Params *params) {
  eio_custom(EIO_Close, EIO_PRI_DEFAULT, EIO_AfterClose, params);
}

EIO_RETURN_TYPE DB::EIO_Close(eio_req *req) {
  Params *params = static_cast<Params*>(req->data);
  DB *self = params->self;

  // Disable all iterators before freeing the database object
  self->CloseIterators();
  self->Close();

  EIO_RETURN_STMT;
}

int DB::EIO_AfterClose(eio_req *req) {
  Params *params = static_cast<Params*>(req->data);

  // We're back in the V8 thread, now dispose of the iterator handles
  params->self->DisposeIterators();

  params->Callback();
  delete params;

  return 0;
}


//
// Put
//

#define USAGE_ERROR(msg) \
  return ThrowTypeError(msg ": DB.put(<key>, <value>, <options?>, <callback?>)");

Handle<Value> DB::Put(const Arguments& args) {
  HandleScope scope;

  // Get this and arguments
  DB* self = ObjectWrap::Unwrap<DB>(args.This());
  int argv = args.Length();

  CHECK_VALID_STATE;

  if (argv < 2)
    USAGE_ERROR("Invalid number of arguments");

  if (!args[0]->IsString() && !Buffer::HasInstance(args[0]))
    USAGE_ERROR("Argument 1 must be a string or buffer");

  if (!args[1]->IsString() && !Buffer::HasInstance(args[1]))
    USAGE_ERROR("Argument 2 must be a string or buffer");

  // Use temporary WriteBatch to implement Put
  WriteBatch *writeBatch = new WriteBatch();
  leveldb::Slice key = JsToSlice(args[0], &writeBatch->strings);
  leveldb::Slice value = JsToSlice(args[1], &writeBatch->strings);
  writeBatch->wb.Put(key, value);

  // Optional write options
  leveldb::WriteOptions options;
  if (argv > 2 && args[2]->IsObject() && !args[2]->IsFunction())
    Options::ParseForWrite(args[2], options);

  // Optional callback
  Local<Function> callback = GET_CALLBACK_ARG(args, argv);

  WriteParams *params = new WriteParams(self, writeBatch, options, callback);
  params->disposeWriteBatch = true;
  EIO_BeforeWrite(params);

  return args.This();
}

#undef USAGE_ERROR


//
// Del
//

#define USAGE_ERROR(msg) \
  return ThrowTypeError(msg ": DB.del(<key>, <options?>, <callback?>)");

Handle<Value> DB::Del(const Arguments& args) {
  HandleScope scope;

  // Get this and arguments
  DB* self = ObjectWrap::Unwrap<DB>(args.This());
  int argv = args.Length();

  CHECK_VALID_STATE;

  if (argv < 1)
    USAGE_ERROR("Invalid number of arguments");

  // Use temporary WriteBatch to implement Del
  WriteBatch *writeBatch = new WriteBatch();
  leveldb::Slice key = JsToSlice(args[0], &writeBatch->strings);
  writeBatch->wb.Delete(key);

  // Optional write options
  leveldb::WriteOptions options;
  GET_WRITE_OPTIONS_ARG(options, args, argv, 1);

  // Optional callback
  Local<Function> callback = GET_CALLBACK_ARG(args, argv);

  WriteParams *params = new WriteParams(self, writeBatch, options, callback);
  params->disposeWriteBatch = true;
  EIO_BeforeWrite(params);

  return args.This();
}

#undef USAGE_ERROR


//
// Write
//

#define USAGE_ERROR(msg) \
  return ThrowTypeError(msg ": DB.write(<key>, <options?>, <callback?>)");

Handle<Value> DB::Write(const Arguments& args) {
  HandleScope scope;

  // Get this and arguments
  DB* self = ObjectWrap::Unwrap<DB>(args.This());
  int argv = args.Length();

  CHECK_VALID_STATE;

  if (argv < 1)
    USAGE_ERROR("Invalid number of arguments");

  // Required WriteBatch
  if (!args[0]->IsObject())
    USAGE_ERROR("Argument 1 must be a WriteBatch object");

  Local<Object> writeBatchObject = Object::Cast(*args[0]);
  WriteBatch* writeBatch = ObjectWrap::Unwrap<WriteBatch>(writeBatchObject);

  if (writeBatch == NULL)
    USAGE_ERROR("Argument 1 must be a WriteBatch object");

  // Optional write options
  leveldb::WriteOptions options;
  GET_WRITE_OPTIONS_ARG(options, args, argv, 2);

  // Optional callback
  Local<Function> callback = GET_CALLBACK_ARG(args, argv);

  // Pass parameters to async function
  WriteParams *params = new WriteParams(self, writeBatch, options, callback);
  if (!params->disposeWriteBatch) writeBatch->Ref();
  EIO_BeforeWrite(params);

  return args.This();
}

#undef USAGE_ERROR

void DB::EIO_BeforeWrite(WriteParams *params) {
  eio_custom(EIO_Write, EIO_PRI_DEFAULT, EIO_AfterWrite, params);
}

EIO_RETURN_TYPE DB::EIO_Write(eio_req *req) {
  WriteParams *params = static_cast<WriteParams*>(req->data);
  DB *self = params->self;

  // Do the actual work
  if (self->db != NULL) {
    params->status = self->db->Write(params->options, &params->writeBatch->wb);
  }

  EIO_RETURN_STMT;
}

int DB::EIO_AfterWrite(eio_req *req) {
  HandleScope scope;

  WriteParams *params = static_cast<WriteParams*>(req->data);
  params->Callback();

  if (params->disposeWriteBatch) {
    delete params->writeBatch;
  } else {
    params->writeBatch->Unref();
  }

  delete params;
  return 0;
}


//
// Get
//

#define USAGE_ERROR(msg) \
  return ThrowTypeError(msg ": DB.get(<key>, <options?>, <callback?>)");

Handle<Value> DB::Get(const Arguments& args) {
  HandleScope scope;

  DB* self = ObjectWrap::Unwrap<DB>(args.This());
  int argv = args.Length();

  CHECK_VALID_STATE;

  if (argv < 1)
    USAGE_ERROR("Invalid number of arguments");

  if (!args[0]->IsString() && !Buffer::HasInstance(args[0]))
    USAGE_ERROR("Argument 1 must be a string or buffer");

  bool asBuffer = false;

  // Optional read options
  leveldb::ReadOptions options;
  GET_READ_OPTIONS_ARG(options, asBuffer, args, argv, 1);

  // Optional callback
  Local<Function> callback = GET_CALLBACK_ARG(args, argv);

  // Pass parameters to async function
  ReadParams *params = new ReadParams(self, options, asBuffer, callback);

  // Set key parameter
  if (args[0]->IsString()) {
    String::Utf8Value str(args[0]);
    params->keyLen = str.length();
    char *tmp = (char*)malloc(params->keyLen);
    memcpy(tmp, *str, params->keyLen);
    params->key = tmp;
    params->keyBuf = Persistent<Object>();
  } else {
    Handle<Object> buf = args[0]->ToObject();
    params->key = Buffer::Data(buf);
    params->keyLen = Buffer::Length(buf);
    params->keyBuf = Persistent<Object>::New(buf);
  }
  EIO_BeforeRead(params);

  return args.This();
}

#undef USAGE_ERROR

void DB::EIO_BeforeRead(ReadParams *params) {
  eio_custom(EIO_Read, EIO_PRI_DEFAULT, EIO_AfterRead, params);
}

EIO_RETURN_TYPE DB::EIO_Read(eio_req *req) {
  ReadParams *params = static_cast<ReadParams*>(req->data);
  DB *self = params->self;

  // Do the actual work
  if (self->db != NULL) {
    leveldb::Slice key(params->key, params->keyLen);
    params->status = self->db->Get(params->options, key, &params->result);
  }

  EIO_RETURN_STMT;
}

int DB::EIO_AfterRead(eio_req *req) {
  HandleScope scope;

  ReadParams *params = static_cast<ReadParams*>(req->data);
  if (params->asBuffer) {
    params->Callback(Bufferize(params->result));
  } else {
    params->Callback(String::New(params->result.data(), params->result.length()));
  }

  if (!params->keyBuf.IsEmpty()) {
    params->keyBuf.Dispose();
  } else {
    free(params->key);
  }

  delete params;
  return 0;
}


//
// NewIterator
//

Handle<Value> DB::NewIterator(const Arguments& args) {
  HandleScope scope;

  DB* self = ObjectWrap::Unwrap<DB>(args.This());
  int argv = args.Length();

  CHECK_VALID_STATE;

  bool asBuffer = false;

  leveldb::ReadOptions options;
  GET_READ_OPTIONS_ARG(options, asBuffer, args, argv, 0);

  leveldb::Iterator* it = self->db->NewIterator(options);

  // https://github.com/joyent/node/blob/master/deps/v8/include/v8.h#L2102
  Local<Value> itArgs[] = {External::New(it), args.This()};
  Handle<Object> itHandle = Iterator::persistent_function_template->GetFunction()->NewInstance(2, itArgs);

  // Keep a weak reference
  Persistent<Object> weakHandle = Persistent<Object>::New(itHandle);
  weakHandle.MakeWeak(&self->iteratorList, &DB::unrefIterator);
  self->iteratorList.push_back(weakHandle);

  return scope.Close( itHandle );
}

bool CheckAlive(Persistent<Object> o) {
  return !o.IsNearDeath();
};

void DB::unrefIterator(Persistent<Value> object, void* parameter) {
  std::vector< Persistent<Object> > *iteratorList =
    (std::vector< Persistent<Object> > *) parameter;

  Iterator *itTarget = ObjectWrap::Unwrap<Iterator>(object->ToObject());

  std::vector< Persistent<Object> >::iterator i =
    partition(iteratorList->begin(), iteratorList->end(), CheckAlive);

  iteratorList->erase(i, iteratorList->end());
}


//
// GetSnapshot
//

Handle<Value> DB::GetSnapshot(const Arguments& args) {
  HandleScope scope;
  DB* self = ObjectWrap::Unwrap<DB>(args.This());

  CHECK_VALID_STATE;

  Local<Value> snapshotArgs[] = {args.This()};
  Handle<Object> snapshot = Snapshot::constructor->GetFunction()->NewInstance(1, snapshotArgs);

  return scope.Close( snapshot );
}


//
// GetProperty
//

#define USAGE_ERROR(msg) \
  return ThrowTypeError(msg ": DB.getProperty(<name>)");

Handle<Value> DB::GetProperty(const Arguments& args) {
  HandleScope scope;
  DB* self = ObjectWrap::Unwrap<DB>(args.This());

  CHECK_VALID_STATE;

  if (args.Length() < 1)
    USAGE_ERROR("Invalid number of arguments");

  if (!args[0]->IsString())
    USAGE_ERROR("Argument 1 must be a string");

  std::string name(*String::Utf8Value(args[0]));
  std::string value;

  if (self->db->GetProperty(leveldb::Slice(name), &value)) {
    return scope.Close( String::New(value.c_str()) );
  } else {
    return Undefined();
  }
}

#undef USAGE_ERROR


//
// GetApproximateSizes
//

#define USAGE_ERROR(msg) \
  return ThrowTypeError(msg ": DB.getApproximateSizes([<start>, <limit>] | [[start, limit]*])");

static inline void AddRange(std::vector<leveldb::Range> &ranges, Handle<Value> startValue, Handle<Value> limitValue) {
  leveldb::Slice start;
  leveldb::Slice limit;

  if (startValue->IsString()) {
    std::string str(*String::Utf8Value(startValue));
    start = leveldb::Slice(str.c_str());
  } else if (Buffer::HasInstance(startValue)) {
    Local<Object> buf = startValue->ToObject();
    start = leveldb::Slice(Buffer::Data(buf), Buffer::Length(buf));
  } else {
    return;
  }

  if (limitValue->IsString()) {
    std::string str(*String::Utf8Value(limitValue));
    limit = leveldb::Slice(str.c_str());
  } else if (Buffer::HasInstance(limitValue)) {
    Local<Object> buf = limitValue->ToObject();
    limit = leveldb::Slice(Buffer::Data(buf), Buffer::Length(buf));
  } else {
    return;
  }

  ranges.push_back(leveldb::Range(start, limit));
}

Handle<Value> DB::GetApproximateSizes(const Arguments& args) {
  HandleScope scope;
  DB* self = ObjectWrap::Unwrap<DB>(args.This());

  CHECK_VALID_STATE;

  int argc = args.Length();

  if (argc < 1)
    USAGE_ERROR("Invalid number of arguments");

  std::vector<leveldb::Range> ranges;

  if (args[0]->IsArray()) {

    Local<Array> array(Array::Cast(*args[0]));
    int len = array->Length();

    for (int i = 0; i < len; ++i) {
      if (array->Has(i)) {
        Local<Value> elem = array->Get(i);
        if (elem->IsArray()) {
          Local<Array> bounds(Array::Cast(*elem));
          if (bounds->Length() >= 2) {
            AddRange(ranges, bounds->Get(0), bounds->Get(1));
          }
        }
      }
    }

  } else if (argc >= 2) {

    if (!args[0]->IsString() && !Buffer::HasInstance(args[0]))
      USAGE_ERROR("Argument 1 must be a string or buffer");

    if (!args[1]->IsString() && !Buffer::HasInstance(args[1]))
      USAGE_ERROR("Argument 2 must be a string or buffer");

    AddRange(ranges, args[0], args[1]);

  } else {
    USAGE_ERROR("Invalid arguments");
  }

  uint64_t sizes = 0;

  self->db->GetApproximateSizes(&ranges[0], ranges.size(), &sizes);

  return scope.Close( Integer::New((int32_t)sizes) );
}

#undef USAGE_ERROR


//
// CompactRange
//

Handle<Value> DB::CompactRange(const Arguments& args) {
  HandleScope scope;
  return ThrowError("Method not implemented");
}


//
// DestroyDB
//

#define USAGE_ERROR(msg) \
  return ThrowTypeError(msg ": DB.destroyDB(<filename>, <options?>)");

Handle<Value> DB::DestroyDB(const Arguments& args) {
  HandleScope scope;

  int argv = args.Length();

  if (argv < 1)
    USAGE_ERROR("Invalid number of arguments");

  if (!args[0]->IsString())
    USAGE_ERROR("Argument 1 must be a string");

  String::Utf8Value name(args[0]);
  leveldb::Options options;
  GET_OPTIONS_ARG(options, args, argv, 1);

  return scope.Close( processStatus(leveldb::DestroyDB(*name, options)) );
}

#undef USAGE_ERROR


//
// RepairDB
//

#define USAGE_ERROR(msg) \
  return ThrowTypeError(msg ": DB.repairDB(<filename>, <options?>)");

Handle<Value> DB::RepairDB(const Arguments& args) {
  HandleScope scope;

  int argv = args.Length();

  if (argv < 1)
    USAGE_ERROR("Invalid number of arguments");

  if (!args[0]->IsString())
    USAGE_ERROR("Argument 1 must be a string");

  String::Utf8Value name(args[0]);
  leveldb::Options options;
  GET_OPTIONS_ARG(options, args, argv, 1);

  return scope.Close( processStatus(leveldb::RepairDB(*name, options)) );
}

#undef USAGE_ERROR


//
// Implementation of Params, which are passed from JS thread to EIO thread and back again
//

DB::Params::Params(DB* self, Handle<Function> cb)
  : self(self)
{
  self->Ref();
  ev_ref(EV_DEFAULT_UC);
  callback = Persistent<Function>::New(cb);
}

DB::Params::~Params() {
  self->Unref();
  ev_unref(EV_DEFAULT_UC);
  callback.Dispose();
}

void DB::Params::Callback(Handle<Value> result) {
  if (!callback.IsEmpty()) {
    TryCatch try_catch;
    if (status.ok()) {
      // no error, callback with no arguments, or result as second argument
      if (result.IsEmpty()) {
        callback->Call(self->handle_, 0, NULL);
      } else {
        Handle<Value> argv[] = { Null(), result };
        callback->Call(self->handle_, 2, argv);
      }
    } else if (status.IsNotFound()) {
      // not found, return (null, undef)
      Handle<Value> argv[] = { Null() };
      callback->Call(self->handle_, 1, argv);
    } else {
      // error, callback with first argument as error object
      Handle<String> message = String::New(status.ToString().c_str());
      Handle<Value> argv[] = { Exception::Error(message) };
      callback->Call(self->handle_, 1, argv);
    }
    if (try_catch.HasCaught()) {
      FatalException(try_catch);
    }
  }
}

} // namespace node_leveldb
