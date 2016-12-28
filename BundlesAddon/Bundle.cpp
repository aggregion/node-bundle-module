#include <nan.h>
#include "Bundle.h"

using namespace std;
namespace aggregion {
using v8::Context;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Isolate;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::Persistent;
using v8::String;
using v8::Value;
using v8::Exception;
using v8::Array;

Persistent<Function> Bundle::constructor;

Bundle::Bundle(const std::string& fileName,
               BundleOpenMode     mode) {
  _bundle = BundleOpen(fileName.c_str(), mode);
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
  NODE_SET_PROTOTYPE_METHOD(tpl, "bundleInitialize",       Initialize);
  NODE_SET_PROTOTYPE_METHOD(tpl, "bundleAttributeGet",     AttributeGet);
  NODE_SET_PROTOTYPE_METHOD(tpl, "bundleAttributeSet",     AttributeSet);
  NODE_SET_PROTOTYPE_METHOD(tpl, "bundleFileAttributeGet", FileAttributeGet);
  NODE_SET_PROTOTYPE_METHOD(tpl, "bundleFileAttributeSet", FileAttributeSet);
  NODE_SET_PROTOTYPE_METHOD(tpl, "bundleFileNames",        FileNames);
  NODE_SET_PROTOTYPE_METHOD(tpl, "bundleFileOpen",         FileOpen);
  NODE_SET_PROTOTYPE_METHOD(tpl, "bundleFileSeek",         FileSeek);
  NODE_SET_PROTOTYPE_METHOD(tpl, "bundleFileLength",       FileLength);
  NODE_SET_PROTOTYPE_METHOD(tpl, "bundleFileRead",         FileRead);
  NODE_SET_PROTOTYPE_METHOD(tpl, "bundleFileWrite",        FileWrite);
  NODE_SET_PROTOTYPE_METHOD(tpl, "bundleFileDelete",       FileDelete);

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

void Bundle::Initialize(const FunctionCallbackInfo<Value>& args) {
  unsigned char dirKey[] = { 0x5C, 0xE5, 0xA2, 0x83, 0x10, 0xDA, 0x4F, 0x8F,
                             0x82, 0xAF, 0x61, 0xDD, 0x64, 0x74, 0x50, 0x85 };

  Bundle *obj = ObjectWrap::Unwrap<Bundle>(args.Holder());

  BundleInitialize(obj->_bundle, dirKey, sizeof(dirKey));
}

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

void Bundle::AttributeGet(const FunctionCallbackInfo<Value>& args) {
  Isolate *isolate = args.GetIsolate();

  if ((args.Length() != 1) || !args[0]->IsString()) {
    isolate->ThrowException(
      Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
    return;
  }
  Bundle *obj = ObjectWrap::Unwrap<Bundle>(args.Holder());

  BundleAttribute type = fromParam(*String::Utf8Value(args[0]->ToString()));

  vector<char> dst;
  int64_t dstLen = 0;

  // calculate size
  BundleAttributeGet(obj->_bundle, type, nullptr, 0, &dstLen);

  if (dstLen > 0) {
    dst.resize(static_cast<int>(dstLen));
    BundleAttributeGet(obj->_bundle, type, dst.data(), 0, &dstLen);
  }

  auto result = node::Buffer::New(isolate, dst.data(), dst.size());
  args.GetReturnValue().Set(result.ToLocalChecked());
}

void Bundle::AttributeSet(const FunctionCallbackInfo<Value>& args) {
  Isolate *isolate = args.GetIsolate();

  if ((args.Length() != 2) || !args[0]->IsString() || !node::Buffer::HasInstance(args[1])) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
    return;
  }
  Bundle *obj = ObjectWrap::Unwrap<Bundle>(args.Holder());

  BundleAttribute type = fromParam(*String::Utf8Value(args[0]->ToString()));
  Local<Object>   buf  = args[1]->ToObject();

  // set
  int64_t total = BundleAttributeSet(obj->_bundle,
                                     type,
                                     node::Buffer::Data(buf),
                                     0,
                                     (int64_t)node::Buffer::Length(buf));

  if (total != node::Buffer::Length(buf)) {
    isolate->ThrowException(Exception::TypeError(
                              String::NewFromUtf8(isolate, "Failed to write attribute")));
  }
}

void Bundle::FileAttributeGet(const FunctionCallbackInfo<Value>& args)     {
  Isolate *isolate = args.GetIsolate();

  if ((args.Length() != 1) || !args[0]->IsNumber()) {
    isolate->ThrowException(
      Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
    return;
  }
  Bundle *obj     = ObjectWrap::Unwrap<Bundle>(args.Holder());
  int     fileIdx = args[0]->Int32Value();
  vector<char> dst;
  int64_t dstLen = 0;

  // calculate size
  BundleFileAttributeGet(obj->_bundle, fileIdx, nullptr, 0, &dstLen, nullptr);

  if (dstLen > 0) {
    dst.resize(static_cast<int>(dstLen));
    BundleFileAttributeGet(obj->_bundle, fileIdx, dst.data(), 0, &dstLen, nullptr);
  }

  auto result = node::Buffer::New(isolate, dst.data(), dst.size());
  args.GetReturnValue().Set(result.ToLocalChecked());
}

void Bundle::FileAttributeSet(const FunctionCallbackInfo<Value>& args) {
  Isolate *isolate = args.GetIsolate();

  if ((args.Length() != 2) || !args[0]->IsInt32() || !node::Buffer::HasInstance(args[1])) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
    return;
  }
  Bundle *obj       = ObjectWrap::Unwrap<Bundle>(args.Holder());
  int     fileIdx   = args[0]->Int32Value();
  Local<Object> buf = args[1]->ToObject();

  // set
  int64_t total = BundleFileAttributeSet(obj->_bundle,
                                         fileIdx,
                                         node::Buffer::Data(buf),
                                         0,
                                         (int64_t)node::Buffer::Length(buf),
                                         nullptr);

  if (total != node::Buffer::Length(buf)) {
    isolate->ThrowException(Exception::TypeError(
                              String::NewFromUtf8(isolate, "Failed to write attribute")));
  }
}

void Bundle::FileNames(const FunctionCallbackInfo<Value>& args) {
  Isolate *isolate = args.GetIsolate();
  Bundle  *obj     = ObjectWrap::Unwrap<Bundle>(args.Holder());

  vector<string> files;

  for (int idx = 0; idx != -1;) {
    char tmp[255];
    idx = BundleFileName(obj->_bundle, idx, tmp, sizeof(tmp));
    files.push_back(string(tmp));
  }
  auto result = v8::Array::New(isolate, files.size());

  for (int i = 0, j = files.size(); i < j; i++) {
    result->Set(i, String::NewFromUtf8(isolate, files[i].c_str()));
  }
  args.GetReturnValue().Set(result);
}

void Bundle::FileOpen(const FunctionCallbackInfo<Value>& args) {
  Isolate *isolate    = args.GetIsolate();
  bool     openAlways = false;

  if ((args.Length() < 1) || !args[0]->IsString()) {
    isolate->ThrowException(
      Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
    return;
  }

  if ((args.Length() > 1) && args[1]->IsBoolean()) {
    openAlways = args[0]->BooleanValue();
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
                                  offset,
                                  static_cast<BundleFileOrigin>(origin));
  args.GetReturnValue().Set(v8::Number::New(isolate, newPos));
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
  args.GetReturnValue().Set(v8::Number::New(isolate, total));
}

void Bundle::FileRead(const FunctionCallbackInfo<Value>& args) {
  Isolate *isolate = args.GetIsolate();

  if ((args.Length() != 2) || !args[0]->IsInt32() || !args[1]->IsNumber()) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
    return;
  }
  Bundle *obj     = ObjectWrap::Unwrap<Bundle>(args.Holder());
  int     fileIdx = args[0]->Int32Value();
  double  total   = args[1]->NumberValue();

  if (total < 0) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Bad length")));
  }

  void   *dst    = malloc(total);
  int64_t dstLen = total;

  if (dst == nullptr) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Out of memory")));
  }

  // calculate size
  dstLen = BundleFileRead(obj->_bundle, fileIdx, dst, 0, &dstLen, nullptr);

  auto result = node::Buffer::New(isolate, reinterpret_cast<char *>(dst), dstLen);
  args.GetReturnValue().Set(result.ToLocalChecked());

  free(dst);
}

void Bundle::FileWrite(const FunctionCallbackInfo<Value>& args) {
  Isolate *isolate = args.GetIsolate();

  if ((args.Length() != 2) || !args[0]->IsInt32() || !node::Buffer::HasInstance(args[1])) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
    return;
  }
  Bundle *obj       = ObjectWrap::Unwrap<Bundle>(args.Holder());
  int     fileIdx   = args[0]->Int32Value();
  Local<Object> buf = args[1]->ToObject();

  // set
  int64_t total = BundleFileWrite(obj->_bundle,
                                  fileIdx,
                                  node::Buffer::Data(buf),
                                  0,
                                  (int64_t)node::Buffer::Length(buf),
                                  nullptr);

  if (total != node::Buffer::Length(buf)) {
    isolate->ThrowException(Exception::TypeError(
                              String::NewFromUtf8(isolate, "Failed to write data")));
  }
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
