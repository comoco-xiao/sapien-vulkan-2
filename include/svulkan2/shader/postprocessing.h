/*
 * Copyright 2025 Hillbot Inc.
 * Copyright 2020-2024 UCSD SU Lab
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once
#include "base_parser.h"

namespace svulkan2 {
namespace shader {

class PostprocessingShaderParser {
public:
  std::future<void> loadFileAsync(std::string const &filepath);
  void reflectSPV();

  inline std::unordered_map<uint32_t, DescriptorSetDescription> const &
  getResources() const {
    return mResources;
  };

  inline std::vector<uint32_t> const &getCode() const { return mSPVCode; }

private:
  std::vector<uint32_t> mSPVCode;
  std::unordered_map<uint32_t, DescriptorSetDescription> mResources;
};

} // namespace shader
} // namespace svulkan2