#pragma once
#include <node.h>
#include <v8.h>

#include "Bundle.h"

using v8::Local;
using v8::Object;

namespace aggregion {
namespace nodejs {

/**
 * @example
 *   var bundle = new BundlesAddon.Bundle("/tmp/someBundle.dat", ["Read", "Write", "OpenAlways"]);
 */

void InitAll(Local<Object> exports) {
  Bundle::Init(exports);
}
} // namespace nodejs
} // namespace aggregion
