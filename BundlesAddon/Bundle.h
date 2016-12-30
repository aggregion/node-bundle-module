#pragma once
#include <string>
#include <node.h>
#include <node_object_wrap.h>
#include "BundlesLibrary.h"

using v8::FunctionCallbackInfo;
using v8::Value;
namespace aggregion {
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
   *   var attrBuf = bundle.AttributeGet("Private");
   */
  static void AttributeGet(const FunctionCallbackInfo<Value>& args);

  /**
   * @param attr {"Private", "Public", "System"}
   * @param value Buffer
   * @example
   *   bundle.AttributeSet("Private", "SomeData");
   */
  static void AttributeSet(const FunctionCallbackInfo<Value>& args);

  /**
   * @param fileIndex
   * @example
   *   var attrBuf = bundle.FileAttributeGet(100);
   */
  static void FileAttributeGet(const FunctionCallbackInfo<Value>& args);

  /**
   * @param fileIndex
   * @param value Buffer
   * @example
   *   bundle.FileAttributeSet(100, "SomeData");
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
   *   var buf = bundle.FileRead(100, 65535);
   */
  static void FileRead(const FunctionCallbackInfo<Value>& args);

  /**
   * @param fileIndex
   * @param buffer
   * @example
   *   var total = bundle.FileWrite(100, someBuf);
   */
  static void FileWrite(const FunctionCallbackInfo<Value>& args);

  /**
   * @param fileIndex
   * @example
   *   bundle.FileDelete(100);
   */
  static void FileDelete(const FunctionCallbackInfo<Value>& args);

private:

  BundlePtr _bundle;
  static v8::Persistent<v8::Function> constructor;
};
} // aggregion
