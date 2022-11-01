/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <memory>
#include <vector>

#include <google/cloud/storage/client.h> // @manual
#include "fbpcf/io/cloud_util/IFileUploader.h"

namespace fbpcf::cloudio {
class GCSFileUploader : public IFileUploader {
 public:
  explicit GCSFileUploader(
      std::shared_ptr<google::cloud::storage::Client> gcsClient,
      const std::string& filePath)
      : gcsClient_{std::move(gcsClient)}, filePath_{filePath} {
    init();
  }
  int upload(std::vector<char>& buf) override;
  int complete() override;

 private:
  void init() override;
  google::cloud::Status abortUpload(std::string session_id);
  std::shared_ptr<google::cloud::storage::Client> gcsClient_;
  const std::string filePath_;
  std::size_t partNumber_ = 1;
  std::string sessionId_;

  google::cloud::storage::ObjectWriteStream stream_;
};

} // namespace fbpcf::cloudio
