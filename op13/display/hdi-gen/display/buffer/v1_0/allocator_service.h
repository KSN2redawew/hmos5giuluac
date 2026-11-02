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

#ifndef OHOS_HDI_DISPLAY_BUFFER_V1_0_ALLOCATORSERVICE_H
#define OHOS_HDI_DISPLAY_BUFFER_V1_0_ALLOCATORSERVICE_H

#include "v1_0/iallocator.h"

namespace OHOS {
namespace HDI {
namespace Display {
namespace Buffer {
namespace V1_0 {
class AllocatorService : public OHOS::HDI::Display::Buffer::V1_0::IAllocator {
public:
    AllocatorService() = default;
    virtual ~AllocatorService() = default;

    int32_t AllocMem(const OHOS::HDI::Display::Buffer::V1_0::AllocInfo& info, sptr<NativeBuffer>& handle) override;

};
} // V1_0
} // Buffer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_BUFFER_V1_0_ALLOCATORSERVICE_H