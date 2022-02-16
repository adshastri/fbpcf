/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include "fbpcf/io/api/ICloser.h"
#include "fbpcf/io/api/IReader.h"

namespace fbpcf::io {

/*
This class is the API for reading data from network
socket. It is constructed with a socket file descriptor.
*/
class SocketReader : public IReader, public ICloser {
 public:
  int close() override;
  int read(char buf[]) override;
  ~SocketReader() override;
};

} // namespace fbpcf::io
