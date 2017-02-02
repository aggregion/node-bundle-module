#include "Bundle.h"

using namespace std;
using namespace v8;

namespace aggregion {
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
    ret->resize(static_cast<size_t>(node::Buffer::Length(buf)));
    memcpy(ret->data(), node::Buffer::Data(buf), ret->size());
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

NAN_MODULE_INIT(Bundle::Init) {
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(Bundle::New);
  tpl->SetClassName(Nan::New("Bundle").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  SetPrototypeMethod(tpl, "AttributeGet",     AttributeGet);
  SetPrototypeMethod(tpl, "AttributeSet",     AttributeSet);
  SetPrototypeMethod(tpl, "FileAttributeGet", FileAttributeGet);
  SetPrototypeMethod(tpl, "FileAttributeSet", FileAttributeSet);
  SetPrototypeMethod(tpl, "FileNames",        FileNames);
  SetPrototypeMethod(tpl, "FileOpen",         FileOpen);
  SetPrototypeMethod(tpl, "FileSeek",         FileSeek);
  SetPrototypeMethod(tpl, "FileLength",       FileLength);
  SetPrototypeMethod(tpl, "FileRead",         FileRead);
  SetPrototypeMethod(tpl, "FileWrite",        FileWrite);
  SetPrototypeMethod(tpl, "FileDelete",       FileDelete);

  constructor().Reset(GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("Bundle").ToLocalChecked(), GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(Bundle::New) {
  if (info.Length() != 2) {
    ThrowTypeError("Wrong number of arguments");
    return;
  }

  if (!info[0]->IsString() || !info[1]->IsArray()) {
    ThrowTypeError("Wrong arguments type");
    return;
  }

  if (info.IsConstructCall()) {
    string fileName    = *String::Utf8Value(info[0]->ToString());
    Local<Array> modes = Local<Array>::Cast(info[1]);

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
      ThrowReferenceError("Failed to create or open bundle");
      return;
    }
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    const int       argc       = 2;
    Local<Value>    argv[argc] = { info[0], info[1] };
    Local<Function> cons       = Nan::New(constructor());
    info.GetReturnValue().Set(cons->NewInstance(argc, argv));
  }
}

BundleWorker::BundleWorker(Callback          *callback,
                           Operation          operation,
                           BundlePtr          bundlePtr,
                           int                fileIdx,
                           std::vector<char> *buffer,
                           std::string        param)
  : AsyncWorker(callback), _operation(operation), _bundle(bundlePtr),
  _fileIdx(fileIdx), _buffer(buffer), _param(param) {}

void BundleWorker::Execute()
{
  int64_t total = _buffer->size();
  bool    write = true;

  try {
    switch (_operation) {
    case OpAttributeGet:
      BundleAttributeGet(_bundle, fromParam(_param), _buffer->data(), 0, &total);
      write = false;
      break;

    case OpAttributeSet:

	  total = BundleAttributeSet(_bundle, fromParam(_param), _buffer->data(), 0,
                                 static_cast<int64_t>(_buffer->size()));
      break;

    case OpFileAttributeGet:
      BundleFileAttributeGet(_bundle, _fileIdx, _buffer->data(), 0, &total, nullptr);
      write = false;
      break;

    case OpFileAttributeSet:
      total = BundleFileAttributeSet(_bundle, _fileIdx, _buffer->data(), 0,
                                     static_cast<int64_t>(_buffer->size()), nullptr);
      break;

    case OpFileRead:
      BundleFileRead(_bundle, _fileIdx, _buffer->data(), 0, &total, nullptr);
      write = false;
      break;

    case OpFileWrite:
      total = BundleFileWrite(_bundle, _fileIdx, _buffer->data(), 0,
                              static_cast<int64_t>(_buffer->size()), nullptr);
      break;
    }

    if (!write) {
      if ((total > 0) && (static_cast<size_t>(total) < _buffer->size())) {
        _buffer->resize(static_cast<size_t>(total));
      }
    } else {
      if (total != static_cast<int64_t>(_buffer->size())) {
        SetErrorMessage("Failed to write content");
      }
    }
  } catch (std::exception& e) {
    SetErrorMessage(e.what());
  }
}

void BundleWorker::HandleOKCallback()
{
  // set up return arguments
  Handle<Value> argv[2] = { Undefined() };

  if ((_operation == OpAttributeGet) ||
      (_operation == OpFileAttributeGet) ||
      (_operation == OpFileRead)) {
    auto result = NewBuffer(_buffer->data(), _buffer->size(), buffer_delete_callback, _buffer);
    argv[1] = result.ToLocalChecked();
    callback->Call(2, argv);
  } else {
    callback->Call(1, argv);
  }
}

NAN_METHOD(Bundle::AttributeGet) {
  if ((info.Length() != 2) || !info[0]->IsString() || !info[1]->IsFunction()) {
    ThrowTypeError("Wrong arguments");
    return;
  }
  Bundle *obj = ObjectWrap::Unwrap<Bundle>(info.Holder());

  BundleAttribute type = fromParam(*String::Utf8Value(info[0]->ToString()));

  auto dst       = new vector<char>();
  int64_t dstLen = 0;

  // calculate size
  BundleAttributeGet(obj->_bundle, type, nullptr, 0, &dstLen);
  dst->resize(static_cast<int>(dstLen));

  AsyncQueueWorker(new BundleWorker(new Callback(info[1].As<Function>()),
                                    BundleWorker::OpAttributeGet,
                                    obj->_bundle,
                                    -1,
                                    dst,
                                    *String::Utf8Value(info[0]->ToString())));
}

NAN_METHOD(Bundle::AttributeSet) {
  if ((info.Length() !=  3) || !info[0]->IsString() || !info[2]->IsFunction()) {
    ThrowTypeError("Wrong arguments");
    return;
  }

  Bundle *obj = ObjectWrap::Unwrap<Bundle>(info.Holder());

  auto src = dataFromArg(info[1]);

  AsyncQueueWorker(new BundleWorker(new Callback(info[2].As<Function>()),
                                    BundleWorker::OpAttributeSet,
                                    obj->_bundle,
                                    -1,
                                    src,
                                    *String::Utf8Value(info[0]->ToString())));
}

NAN_METHOD(Bundle::FileAttributeGet)     {
  if ((info.Length() != 2) || !info[0]->IsNumber() || !info[1]->IsFunction()) {
    ThrowTypeError("Wrong arguments");
    return;
  }
  Bundle *obj     = ObjectWrap::Unwrap<Bundle>(info.Holder());
  int     fileIdx = info[0]->Int32Value();
  auto    dst     = new vector<char>();
  int64_t dstLen  = 0;

  // calculate size
  BundleFileAttributeGet(obj->_bundle, fileIdx, nullptr, 0, &dstLen, nullptr);

  dst->resize(static_cast<int>(dstLen));
  AsyncQueueWorker(new BundleWorker(new Callback(info[1].As<Function>()),
                                    BundleWorker::OpFileAttributeGet, obj->_bundle, fileIdx, dst));
}

NAN_METHOD(Bundle::FileAttributeSet) {
  if ((info.Length() != 3) || !info[0]->IsInt32() || !info[2]->IsFunction()) {
    ThrowTypeError("Wrong arguments");
    return;
  }

  Bundle *obj     = ObjectWrap::Unwrap<Bundle>(info.Holder());
  int     fileIdx = info[0]->Int32Value();

  auto src = dataFromArg(info[1]);

  AsyncQueueWorker(new BundleWorker(new Callback(info[2].As<Function>()),
                                    BundleWorker::OpFileAttributeSet, obj->_bundle, fileIdx, src));
}

NAN_METHOD(Bundle::FileNames) {
  Bundle *obj = ObjectWrap::Unwrap<Bundle>(info.Holder());

  vector<string> files;
  int idx = 0;

  do {
    char tmp[255] = { 0 };
    idx = BundleFileName(obj->_bundle, idx, tmp, sizeof(tmp));

    if ((idx != -1) && (tmp[0] != '\0')) {
      files.push_back(string(tmp));
    }
  } while (idx != -1);
  Local<Array> result = Nan::New<Array>();

  for (int i = 0, j = static_cast<int>(files.size()); i < j; i++) {
    Nan::Set(result, i, Nan::New<String>(files[i].c_str()).ToLocalChecked());
  }
  info.GetReturnValue().Set(result);
}

NAN_METHOD(Bundle::FileOpen) {
  bool openAlways = false;

  if ((info.Length() < 1) || !info[0]->IsString()) {
    ThrowTypeError("Wrong arguments");
    return;
  }

  if ((info.Length() > 1) && info[1]->IsBoolean()) {
    openAlways = info[1]->BooleanValue();
  }
  Bundle *obj      = ObjectWrap::Unwrap<Bundle>(info.Holder());
  string  fileName = *String::Utf8Value(info[0]->ToString());

  int idx = BundleFileOpen(obj->_bundle, fileName.c_str(), openAlways ? 1 : 0);

  info.GetReturnValue().Set(Nan::New<Int32>(idx));
}

NAN_METHOD(Bundle::FileSeek) {
  if ((info.Length() != 3) || !info[0]->IsInt32() || !info[1]->IsNumber() || !info[2]->IsString()) {
    ThrowTypeError("Wrong arguments");
    return;
  }
  Bundle *obj     = ObjectWrap::Unwrap<Bundle>(info.Holder());
  int     fileIdx = info[0]->Int32Value();
  double  offset  = info[1]->NumberValue();
  string  param   = *String::Utf8Value(info[2]->ToString());

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
  info.GetReturnValue().Set(Nan::New<Number>(static_cast<double>(newPos)));
}

NAN_METHOD(Bundle::FileLength) {
  if ((info.Length() != 1) || !info[0]->IsInt32()) {
    ThrowTypeError("Wrong arguments");
    return;
  }
  Bundle *obj     = ObjectWrap::Unwrap<Bundle>(info.Holder());
  int     fileIdx = info[0]->Int32Value();

  int64_t total = BundleFileLength(obj->_bundle, fileIdx);
  info.GetReturnValue().Set(Nan::New<Number>(static_cast<double>(total)));
}

NAN_METHOD(Bundle::FileRead) {
  if ((info.Length() != 3) || !info[0]->IsInt32() || !info[1]->IsNumber() || !info[2]->IsFunction()) {
    ThrowTypeError("Wrong arguments");
    return;
  }
  Bundle *obj     = ObjectWrap::Unwrap<Bundle>(info.Holder());
  int     fileIdx = info[0]->Int32Value();
  double  total   = info[1]->NumberValue();

  if (total < 0) {
    ThrowTypeError("Bad length");
  }

  auto dst = new vector<char>(static_cast<size_t>(total));

  AsyncQueueWorker(new BundleWorker(new Callback(info[2].As<Function>()),
                                    BundleWorker::OpFileRead, obj->_bundle, fileIdx, dst));
}

NAN_METHOD(Bundle::FileWrite) {
  if ((info.Length() != 3) || !info[0]->IsInt32() || !info[2]->IsFunction()) {
    ThrowTypeError("Wrong arguments");
    return;
  }
  Bundle *obj     = ObjectWrap::Unwrap<Bundle>(info.Holder());
  int     fileIdx = info[0]->Int32Value();
  auto    src     = dataFromArg(info[1]);

  AsyncQueueWorker(new BundleWorker(new Callback(info[2].As<Function>()),
                                    BundleWorker::OpFileWrite, obj->_bundle, fileIdx, src));
}

NAN_METHOD(Bundle::FileDelete) {
  if ((info.Length() != 1) || !info[0]->IsInt32()) {
    ThrowTypeError("Wrong arguments");
    return;
  }
  Bundle *obj     = ObjectWrap::Unwrap<Bundle>(info.Holder());
  int     fileIdx = info[0]->Int32Value();

  BundleFileDelete(obj->_bundle, fileIdx);
}
} // namespace aggregion
