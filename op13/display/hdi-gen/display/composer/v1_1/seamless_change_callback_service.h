/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_HDI_DISPLAY_COMPOSER_V1_1_SEAMLESSCHANGECALLBACKSERVICE_H
#define OHOS_HDI_DISPLAY_COMPOSER_V1_1_SEAMLESSCHANGECALLBACKSERVICE_H

#include "v1_1/iseamless_change_callback.h"

namespace OHOS {
namespace HDI {
namespace Display {
namespace Composer {
namespace V1_1 {
class SeamlessChangeCallbackService : public OHOS::HDI::Display::Composer::V1_1::ISeamlessChangeCallback {
public:
    SeamlessChangeCallbackService() = default;
    virtual ~SeamlessChangeCallbackService() = default;

    int32_t OnSeamlessChange(uint32_t devId) override;

};
} // V1_1
} // Composer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_COMPOSER_V1_1_SEAMLESSCHANGECALLBACKSERVICE_H