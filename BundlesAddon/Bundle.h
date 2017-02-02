#pragma once
#include <nan.h>
#include "BundlesLibrary.h"

using namespace Nan;
namespace aggregion {
class Bundle;
class BundleWorker : public AsyncWorker {
public:

  enum Operation {
    OpAttributeGet,
    OpAttributeSet,
    OpFileAttributeGet,
    OpFileAttributeSet,
    OpFileRead,
    OpFileWrite
  };

  explicit BundleWorker(Callback          *callback,
                        Operation          operation,
                        BundlePtr          bundlePtr,
                        int                fileIdx,
                        std::vector<char> *buffer,
                        std::string        param = "");
  virtual ~BundleWorker() {}

private:

  virtual void Execute();
  virtual void HandleOKCallback();


  // optional : data goes here.
  // data that doesn't go back to javascript can be any typedef
  // data that goes back to javascript needs to be a supported type

  Operation _operation;
  BundlePtr _bundle = nullptr;
  int _fileIdx;
  std::vector<char> *_buffer = nullptr;
  std::string _param;
};

class Bundle : public node::ObjectWrap {
public:

  static NAN_MODULE_INIT(Init);

private:

  explicit Bundle(const std::string& fileName,
                  BundleOpenMode     mode);
  virtual ~Bundle();

  static NAN_METHOD(New);

  /**
   * @param attr {"Private", "Public", "System"}
   * @example
   *   bundle.AttributeGet("Private", callback);
   */
  static NAN_METHOD(AttributeGet);

  /**
   * @param attr {"Private", "Public", "System"}
   * @param value Buffer
   * @example
   *   bundle.AttributeSet("Private", "SomeData", callback);
   */
  static NAN_METHOD(AttributeSet);

  /**
   * @param fileIndex
   * @example
   *   bundle.FileAttributeGet(100, callback);
   */
  static NAN_METHOD(FileAttributeGet);

  /**
   * @param fileIndex
   * @param value Buffer
   * @example
   *   bundle.FileAttributeSet(100, "SomeData", callback);
   */
  static NAN_METHOD(FileAttributeSet);

  /**
   * @example
   *   var fileNamesArray = bundle.FileNames();
   */
  static NAN_METHOD(FileNames);

  /**
   * @param fileName
   * @param openAlways
   * @example
   *   var fileIndex = bundle.FileOpen("SomeFile.dat", true);
   */
  static NAN_METHOD(FileOpen);

  /**
   * @param fileIndex
   * @param offset
   * @param origin {"Set", "Cur", "End"}
   * @example
   *   var newPos = bundle.FileSeek(100, 12312, "Set");
   */
  static NAN_METHOD(FileSeek);

  /**
   * @param fileIndex
   * @example
   *   var len = bundle.FileLength(100);
   */
  static NAN_METHOD(FileLength);

  /**
   * @param fileIndex
   * @param length
   * @example
   *   bundle.FileRead(100, 65535, callback);
   */
  static NAN_METHOD(FileRead);

  /**
   * @param fileIndex
   * @param buffer
   * @example
   *   bundle.FileWrite(100, someBuf, callback);
   */
  static NAN_METHOD(FileWrite);

  /**
   * @param fileIndex
   * @example
   *   bundle.FileDelete(100);
   */
  static NAN_METHOD(FileDelete);

private:

  BundlePtr _bundle = nullptr;

  static inline Persistent<v8::Function>& constructor() {
    static Persistent<v8::Function> _constructor;

    return _constructor;
  }
};

/**
 * @example
 *   var bundle = new BundlesAddon.Bundle("/tmp/someBundle.dat", ["Read", "Write", "OpenAlways"]);
 */
NODE_MODULE(BundlesAddon, Bundle::Init)
} // aggregion
