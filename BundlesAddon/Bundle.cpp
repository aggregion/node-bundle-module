#include <node.h>
#include <v8.h>
#include <uv.h>
#include <nan.h>

#include "Bundle.h"

using namespace std;
using namespace v8;
using namespace node;

/**
 * @example
 *   var bundle = new BundlesAddon.Bundle("/tmp/someBundle.dat", ["Read", "Write", "OpenAlways"]);
 */
void InitAll(Local<Object>exports) {
  aggregion::Bundle::Init(exports);
}

NODE_MODULE(BundlesAddon, InitAll)

namespace aggregion {
Persistent<Function> Bundle::constructor;

BundleAttribute fromParam(const string& param) {
  int type = 0;

  if (param.compare("Private") == 0) {
    type = BUNDLE_EXTRA_PRIVATE;
  } else if (param.compare("Public") == 0) {
    type = BUNDLE_EXTRA_PUBLIC;
  } else if (param.compare("System") == 0) {
    type = BUNDLE_EXTRA_SYSTEM;
  }
  return static_cast<BundleAttribute>(type);
}

vector<char>* dataFromArg(Local<Value>val) {
  auto ret = new vector<char>();

  if (node::Buffer::HasInstance(val)) {
    Local<Object> buf = val->ToObject();
    ret->resize(static_cast<size_t>(Buffer::Length(buf)));
    memcpy(ret->data(), Buffer::Data(buf), ret->size());
  } else if (!val->IsObject()) {
    Local<String> strInput = val->ToString();
    ret->resize(static_cast<size_t>(strInput->Utf8Length()));
    memcpy(ret->data(), *String::Utf8Value(strInput), ret->size());
  }
  return ret;
}

void buffer_delete_callback(char *data, void *the_vector) {
  delete reinterpret_cast<vector<char> *>(the_vector);
}

Bundle::Bundle(const std::string& fileName,
               BundleOpenMode     mode) {
  unsigned char dirKey[] = { 0x5C, 0xE5, 0xA2, 0x83, 0x10, 0xDA, 0x4F, 0x8F,
                             0x82, 0xAF, 0x61, 0xDD, 0x64, 0x74, 0x50, 0x85 };

  _bundle = BundleOpen(fileName.c_str(), mode);


  BundleInitialize(_bundle, dirKey, sizeof(dirKey));
}

Bundle::~Bundle() {
  if (_bundle != nullptr) {
    BundleClose(_bundle);
    _bundle = nullptr;
  }
}

void Bundle::Init(Local<Object>exports) {
  Isolate *isolate = exports->GetIsolate();

  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "Bundle"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "AttributeGet",     AttributeGet);
  NODE_SET_PROTOTYPE_METHOD(tpl, "AttributeSet",     AttributeSet);
  NODE_SET_PROTOTYPE_METHOD(tpl, "FileAttributeGet", FileAttributeGet);
  NODE_SET_PROTOTYPE_METHOD(tpl, "FileAttributeSet", FileAttributeSet);
  NODE_SET_PROTOTYPE_METHOD(tpl, "FileNames",        FileNames);
  NODE_SET_PROTOTYPE_METHOD(tpl, "FileOpen",         FileOpen);
  NODE_SET_PROTOTYPE_METHOD(tpl, "FileSeek",         FileSeek);
  NODE_SET_PROTOTYPE_METHOD(tpl, "FileLength",       FileLength);
  NODE_SET_PROTOTYPE_METHOD(tpl, "FileRead",         FileRead);
  NODE_SET_PROTOTYPE_METHOD(tpl, "FileWrite",        FileWrite);
  NODE_SET_PROTOTYPE_METHOD(tpl, "FileDelete",       FileDelete);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "Bundle"), tpl->GetFunction());
}

void Bundle::New(const FunctionCallbackInfo<Value>& args) {
  Isolate *isolate = args.GetIsolate();

  if (args.Length() != 2) {
    isolate->ThrowException(
      Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    return;
  }

  if (!args[0]->IsString() || !args[1]->IsArray()) {
    isolate->ThrowException(
      Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments type")));
    return;
  }


  if (args.IsConstructCall()) {
    string fileName    = *String::Utf8Value(args[0]->ToString());
    Local<Array> modes = Local<Array>::Cast(args[1]);

    int bmode = 0;

    for (uint32_t i = 0, j = modes->Length(); i < j; i++) {
      string m = *String::Utf8Value(Local<String>::Cast(modes->Get(i)));

      if (m.compare("Read") == 0) {
        bmode |= BMODE_READ;
      }

      if (m.compare("Write") == 0) {
        bmode |= BMODE_WRITE;
      }

      if (m.compare("OpenAlways") == 0) {
        bmode |= BMODE_OPEN_ALWAYS;
      }
    }

    // Invoked as constructor: `new Bundle(...)`
    Bundle *obj = new Bundle(fileName, static_cast<BundleOpenMode>(bmode));

    if (obj->_bundle == nullptr) {
      isolate->ThrowException(
        Exception::TypeError(String::NewFromUtf8(isolate, "Failed to create or open bundle")));
      return;
    }
    obj->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  } else {
    // Invoked as plain function `Bundle(...)`, turn into construct call.
    const int       argc       = 2;
    Local<Value>    argv[argc] = { args[0], args[1] };
    Local<Context>  context    = isolate->GetCurrentContext();
    Local<Function> cons       = Local<Function>::New(isolate, constructor);
    Local<Object>   result     = cons->NewInstance(context, argc, argv).ToLocalChecked();
    args.GetReturnValue().Set(result);
  }
}

void Bundle::StartWorkAsync(const FunctionCallbackInfo<Value>& args,
                            Work::Operation                    operation,
                            Bundle                            *bundle,
                            int                                fileIdx,
                            std::vector<char>                 *buffer,
                            string                             param,
                            Local<Function>                    callback) {
  Isolate *isolate = Isolate::GetCurrent();

  isolate->ThrowException(
    Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));

  return;

  Work *work = new Work();
  work->request.data = work;
  work->operation    = operation;
  work->bundle       = bundle;
  work->fileIdx      = fileIdx;
  work->buffer       = buffer;
  work->param        = param;
  work->callback.Reset(isolate, callback);

  uv_queue_work(uv_default_loop(), &work->request, &Bundle::WorkAsync, &Bundle::WorkAsyncComplete);
  args.GetReturnValue().Set(Undefined(isolate));
}

void Bundle::WorkAsync(uv_work_t *req)
{
  Work   *work  = static_cast<Work *>(req->data);
  int64_t total = 0;
  bool    write = true;

  switch (work->operation) {
  case Work::Operation::OpAttributeGet:
    BundleAttributeGet(work->bundle, fromParam(work->param), work->buffer->data(), 0, &total);
    write = false;
    break;

  case Work::Operation::OpAttributeSet:
    total = BundleAttributeSet(work->bundle, fromParam(work->param), work->buffer->data(), 0,
                               static_cast<int64_t>(work->buffer->size()));
    break;

  case Work::Operation::OpFileAttributeGet:
    BundleFileAttributeGet(work->bundle, work->fileIdx, work->buffer->data(), 0, &total, nullptr);
    write = false;
    break;

  case Work::Operation::OpFileAttributeSet:
    total = BundleFileAttributeSet(work->bundle, work->fileIdx, work->buffer->data(), 0,
                                   static_cast<int64_t>(work->buffer->size()), nullptr);
    break;

  case Work::Operation::OpFileRead:
    BundleFileRead(work->bundle, work->fileIdx, work->buffer->data(), 0, &total, nullptr);
    write = false;
    break;

  case Work::Operation::OpFileWrite:
    total = BundleFileWrite(work->bundle, work->fileIdx,
                            work->buffer->data(), 0,
                            static_cast<int64_t>(work->buffer->size()), nullptr);
    break;
  }

  if (!write) {
    if ((total > 0) && (static_cast<size_t>(total) < work->buffer->size())) {
      work->buffer->resize(static_cast<size_t>(total));
    }
  } else {
    if (total != static_cast<int64_t>(work->buffer->size())) {
      work->error = "Failed to write content";
    }
  }
}

void Bundle::WorkAsyncComplete(uv_work_t *req, int status)
{
  Isolate *isolate = Isolate::GetCurrent();

  v8::HandleScope handleScope(isolate); // Required for Node 4.x

  Work *work = static_cast<Work *>(req->data);

  MaybeLocal<String> error = String::NewFromUtf8(isolate,
                                                 work->error.c_str(),
                                                 v8::NewStringType::kNormal);
  Handle<Value> errorLocal = Null(isolate);

  if (!work->error.empty()) {
    errorLocal = error.ToLocalChecked();
  }

  // set up return arguments
  Handle<Value> argv[2] = { errorLocal };
  bool write            = true;
  auto result           = Nan::NewBuffer(work->buffer->data(),
                                         work->buffer->size(),
                                         buffer_delete_callback,
                                         work->buffer);


  if ((work->operation == Work::Operation::OpAttributeGet) ||
      (work->operation == Work::Operation::OpFileAttributeGet) ||
      (work->operation == Work::Operation::OpFileRead)) {
    argv[1] = result.ToLocalChecked();
    write   = false;
  }

  // execute the callback
  Local<Function>::New(isolate, work->callback)->Call(
    isolate->GetCurrentContext()->Global(), write ? 1 : 2, argv);

  // Free up the persistent function callback
  work->callback.Reset();

  delete work;
}

void Bundle::AttributeGet(const FunctionCallbackInfo<Value>& args) {
  Isolate *isolate = args.GetIsolate();

  if ((args.Length() != 2) || !args[0]->IsString() || !args[1]->IsFunction()) {
    isolate->ThrowException(
      Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
    return;
  }
  Bundle *obj = ObjectWrap::Unwrap<Bundle>(args.Holder());

  BundleAttribute type = fromParam(*String::Utf8Value(args[0]->ToString()));

  auto dst       = new vector<char>();
  int64_t dstLen = 0;

  // calculate size
  BundleAttributeGet(obj->_bundle, type, nullptr, 0, &dstLen);
  dst->resize(static_cast<int>(dstLen));

  StartWorkAsync(args,
                 Work::Operation::OpAttributeGet,
                 obj,
                 -1,
                 dst,
                 *String::Utf8Value(args[0]->ToString()),
                 Local<Function>::Cast(args[1]));
}

void Bundle::AttributeSet(const FunctionCallbackInfo<Value>& args) {
  Isolate *isolate = args.GetIsolate();

  if ((args.Length() !=  4) || !args[0]->IsString() || !args[2]->IsFunction()) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
    return;
  }

  Bundle *obj = ObjectWrap::Unwrap<Bundle>(args.Holder());

  auto src = dataFromArg(args[1]);

  StartWorkAsync(args,
                 Work::Operation::OpAttributeSet,
                 obj,
                 -1,
                 src,
                 *String::Utf8Value(args[0]->ToString()),
                 Local<Function>::Cast(args[2]));
}

void Bundle::FileAttributeGet(const FunctionCallbackInfo<Value>& args)     {
  Isolate *isolate = args.GetIsolate();

  if ((args.Length() != 2) || !args[0]->IsNumber() || !args[1]->IsFunction()) {
    isolate->ThrowException(
      Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
    return;
  }
  Bundle *obj     = ObjectWrap::Unwrap<Bundle>(args.Holder());
  int     fileIdx = args[0]->Int32Value();
  auto    dst     = new vector<char>();
  int64_t dstLen  = 0;

  // calculate size
  BundleFileAttributeGet(obj->_bundle, fileIdx, nullptr, 0, &dstLen, nullptr);

  dst->resize(static_cast<int>(dstLen));
  StartWorkAsync(args,
                 Work::Operation::OpFileAttributeGet,
                 obj,
                 fileIdx,
                 dst,
                 "",
                 Local<Function>::Cast(args[1]));
}

void Bundle::FileAttributeSet(const FunctionCallbackInfo<Value>& args) {
  Isolate *isolate = args.GetIsolate();

  if ((args.Length() != 3) || !args[0]->IsInt32() || !args[2]->IsFunction()) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
    return;
  }
  Bundle *obj     = ObjectWrap::Unwrap<Bundle>(args.Holder());
  int     fileIdx = args[0]->Int32Value();


  auto src = dataFromArg(args[1]);

  StartWorkAsync(args,
                 Work::Operation::OpFileAttributeSet,
                 obj,
                 fileIdx,
                 src,
                 "",
                 Local<Function>::Cast(args[2]));
}

void Bundle::FileNames(const FunctionCallbackInfo<Value>& args) {
  Isolate *isolate = args.GetIsolate();
  Bundle  *obj     = ObjectWrap::Unwrap<Bundle>(args.Holder());

  vector<string> files;
  int idx = 0;

  do {
    char tmp[255] = { 0 };
    idx = BundleFileName(obj->_bundle, idx, tmp, sizeof(tmp));

    if ((idx != -1) && (tmp[0] != '\0')) {
      files.push_back(string(tmp));
    }
  } while (idx != -1);
  auto result = v8::Array::New(isolate, static_cast<int>(files.size()));

  for (int i = 0, j = static_cast<int>(files.size()); i < j; i++) {
    result->Set(i, String::NewFromUtf8(isolate, files[i].c_str()));
  }
  args.GetReturnValue().Set(result);
}

void Bundle::FileOpen(const FunctionCallbackInfo<Value>& args) {
  Isolate *isolate    = args.GetIsolate();
  bool     openAlways = false;

  if ((args.Length() < 1) || !args[0]->IsString()) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
    return;
  }

  if ((args.Length() > 1) && args[1]->IsBoolean()) {
    openAlways = args[1]->BooleanValue();
  }
  Bundle *obj      = ObjectWrap::Unwrap<Bundle>(args.Holder());
  string  fileName = *String::Utf8Value(args[0]->ToString());

  int idx = BundleFileOpen(obj->_bundle, fileName.c_str(), openAlways ? 1 : 0);

  args.GetReturnValue().Set(v8::Int32::New(isolate, idx));
}

void Bundle::FileSeek(const FunctionCallbackInfo<Value>& args) {
  Isolate *isolate = args.GetIsolate();

  if ((args.Length() != 3) || !args[0]->IsInt32() || !args[1]->IsNumber() || !args[2]->IsString()) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
    return;
  }
  Bundle *obj     = ObjectWrap::Unwrap<Bundle>(args.Holder());
  int     fileIdx = args[0]->Int32Value();
  double  offset  = args[1]->NumberValue();
  string  param   = *String::Utf8Value(args[2]->ToString());

  int origin = 0;

  if (param.compare("Set") == 0) {
    origin = BUNDLE_FILE_ORIG_SET;
  } else if (param.compare("Cur") == 0) {
    origin = BUNDLE_FILE_ORIG_CUR;
  } else if (param.compare("End") == 0) {
    origin = BUNDLE_FILE_ORIG_END;
  }

  int64_t newPos = BundleFileSeek(obj->_bundle,
                                  fileIdx,
                                  static_cast<int64_t>(offset),
                                  static_cast<BundleFileOrigin>(origin));
  args.GetReturnValue().Set(v8::Number::New(isolate, static_cast<double>(newPos)));
}

void Bundle::FileLength(const FunctionCallbackInfo<Value>& args) {
  Isolate *isolate = args.GetIsolate();

  if ((args.Length() != 1) || !args[0]->IsInt32()) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
    return;
  }
  Bundle *obj     = ObjectWrap::Unwrap<Bundle>(args.Holder());
  int     fileIdx = args[0]->Int32Value();

  int64_t total = BundleFileLength(obj->_bundle, fileIdx);
  args.GetReturnValue().Set(v8::Number::New(isolate, static_cast<double>(total)));
}

void Bundle::FileRead(const FunctionCallbackInfo<Value>& args) {
  Isolate *isolate = args.GetIsolate();

  if ((args.Length() != 3) || !args[0]->IsInt32() || !args[1]->IsNumber() || !args[2]->IsFunction()) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
    return;
  }
  Bundle *obj     = ObjectWrap::Unwrap<Bundle>(args.Holder());
  int     fileIdx = args[0]->Int32Value();
  double  total   = args[1]->NumberValue();

  if (total < 0) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Bad length")));
  }

  auto dst = new vector<char>(static_cast<size_t>(total));

  StartWorkAsync(args,
                 Work::Operation::OpFileRead,
                 obj,
                 fileIdx,
                 dst,
                 "",
                 Local<Function>::Cast(args[2]));
}

void Bundle::FileWrite(const FunctionCallbackInfo<Value>& args) {
  Isolate *isolate = args.GetIsolate();

  if ((args.Length() != 3) || !args[0]->IsInt32() || !args[2]->IsFunction()) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
    return;
  }
  Bundle *obj     = ObjectWrap::Unwrap<Bundle>(args.Holder());
  int     fileIdx = args[0]->Int32Value();
  auto    src     = dataFromArg(args[1]);

  StartWorkAsync(args,
                 Work::Operation::OpAttributeSet,
                 obj,
                 fileIdx,
                 src,
                 "",
                 Local<Function>::Cast(args[2]));
}

void Bundle::FileDelete(const FunctionCallbackInfo<Value>& args) {
  Isolate *isolate = args.GetIsolate();

  if ((args.Length() != 1) || !args[0]->IsInt32()) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
    return;
  }
  Bundle *obj     = ObjectWrap::Unwrap<Bundle>(args.Holder());
  int     fileIdx = args[0]->Int32Value();

  BundleFileDelete(obj->_bundle, fileIdx);
}
} // namespace aggregion
