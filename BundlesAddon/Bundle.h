#pragma once
#include <string>
#include <node.h>
#include <node_object_wrap.h>
#include <uv.h>
#include "BundlesLibrary.h"

using v8::FunctionCallbackInfo;
using v8::Value;
namespace aggregion {
struct Work {
  // required
  uv_work_t                   request;  // libuv
  v8::Persistent<v8::Function>callback; // javascript callback

  // optional : data goes here.
  // data that doesn't go back to javascript can be any typedef
  // data that goes back to javascript needs to be a supported type
  enum class Operation {
    OpAttributeGet, OpAttributeSet, OpFileAttributeGet, OpFileAttributeSet, OpFileRead, OpFileWrite
  };

  Operation          operation;
  class Bundle      *bundle;
  int                fileIdx;
  std::vector<char> *buffer;
  std::string        param;

  // operation error
  std::string error;
};

class Bundle : public node::ObjectWrap {
public:

  static void Init(v8::Local<v8::Object>exports);

private:

  explicit Bundle(const std::string& fileName,
                  BundleOpenMode     mode);
  virtual ~Bundle();

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

  /**
   * @param attr {"Private", "Public", "System"}
   * @example
   *   bundle.AttributeGet("Private", callback);
   */
  static void AttributeGet(const FunctionCallbackInfo<Value>& args);

  /**
   * @param attr {"Private", "Public", "System"}
   * @param value Buffer
   * @example
   *   bundle.AttributeSet("Private", "SomeData", callback);
   */
  static void AttributeSet(const FunctionCallbackInfo<Value>& args);

  /**
   * @param fileIndex
   * @example
   *   bundle.FileAttributeGet(100, callback);
   */
  static void FileAttributeGet(const FunctionCallbackInfo<Value>& args);

  /**
   * @param fileIndex
   * @param value Buffer
   * @example
   *   bundle.FileAttributeSet(100, "SomeData", callback);
   */
  static void FileAttributeSet(const FunctionCallbackInfo<Value>& args);

  /**
   * @example
   *   var fileNamesArray = bundle.FileNames();
   */
  static void FileNames(const FunctionCallbackInfo<Value>& args);

  /**
   * @param fileName
   * @param openAlways
   * @example
   *   var fileIndex = bundle.FileOpen("SomeFile.dat", true);
   */
  static void FileOpen(const FunctionCallbackInfo<Value>& args);

  /**
   * @param fileIndex
   * @param offset
   * @param origin {"Set", "Cur", "End"}
   * @example
   *   var newPos = bundle.FileSeek(100, 12312, "Set");
   */
  static void FileSeek(const FunctionCallbackInfo<Value>& args);

  /**
   * @param fileIndex
   * @example
   *   var len = bundle.FileLength(100);
   */
  static void FileLength(const FunctionCallbackInfo<Value>& args);

  /**
   * @param fileIndex
   * @param length
   * @example
   *   bundle.FileRead(100, 65535, callback);
   */
  static void FileRead(const FunctionCallbackInfo<Value>& args);

  /**
   * @param fileIndex
   * @param buffer
   * @example
   *   bundle.FileWrite(100, someBuf, callback);
   */
  static void FileWrite(const FunctionCallbackInfo<Value>& args);

  /**
   * @param fileIndex
   * @example
   *   bundle.FileDelete(100);
   */
  static void FileDelete(const FunctionCallbackInfo<Value>& args);

  static void StartWorkAsync(const FunctionCallbackInfo<Value>& args,
                             Work::Operation                    operation,
                             Bundle                            *bundle,
                             int                                fileIdx,
                             std::vector<char>                 *buffer,
                             std::string                        param,
                             v8::Local<v8::Function>            callback);

  static void WorkAsync(uv_work_t *req);
  static void WorkAsyncComplete(uv_work_t *req,
                                int        status);

private:

  BundlePtr _bundle;
  static v8::Persistent<v8::Function>constructor;
};
} // aggregion
