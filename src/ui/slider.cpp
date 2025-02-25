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
#include "svulkan2/ui/slider.h"
#include <imgui.h>

namespace svulkan2 {
namespace ui {

void SliderFloat::build() {
  if (mWidthRatio > 0) {
    ImGui::SetNextItemWidth(mWidthRatio * ImGui::GetWindowContentRegionWidth());
  }
  if (mValueGetter) {
    mValue = mValueGetter();
  }
  if (ImGui::SliderFloat(getLabelId().c_str(), &mValue, mMin, mMax)) {
    if (mValueSetter) {
      mValueSetter(mValue);
    }
    if (mCallback)
      mCallback(std::static_pointer_cast<SliderFloat>(shared_from_this()));
  }
}

void SliderAngle::build() {
  if (mWidthRatio > 0) {
    ImGui::SetNextItemWidth(mWidthRatio * ImGui::GetWindowContentRegionWidth());
  }
  if (mValueGetter) {
    mValue = mValueGetter();
  }
  if (ImGui::SliderAngle(getLabelId().c_str(), &mValue, mMin, mMax)) {
    if (mValueSetter) {
      mValueSetter(mValue);
    }
    if (mCallback) {
      mCallback(std::static_pointer_cast<SliderAngle>(shared_from_this()));
    }
  }
}

} // namespace ui
} // namespace svulkan2