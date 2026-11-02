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

#ifndef OHOS_HDI_DISPLAY_BUFFER_V1_0_IALLOCATOR_H
#define OHOS_HDI_DISPLAY_BUFFER_V1_0_IALLOCATOR_H

#include <stdint.h>
#include <base/native_buffer.h>
#include <hdf_base.h>
#include <hdi_base.h>
#include "display/buffer/v1_0/display_buffer_type.h"

#ifndef HDI_BUFF_MAX_SIZE
#define HDI_BUFF_MAX_SIZE (1024 * 200)
#endif

#ifndef HDI_CHECK_VALUE_RETURN
#define HDI_CHECK_VALUE_RETURN(lv, compare, rv, ret) do { \
    if ((lv) compare (rv)) { \
        return ret; \
    } \
} while (false)
#endif

#ifndef HDI_CHECK_VALUE_RET_GOTO
#define HDI_CHECK_VALUE_RET_GOTO(lv, compare, rv, ret, value, table) do { \
    if ((lv) compare (rv)) { \
        ret = value; \
        goto table; \
    } \
} while (false)
#endif

namespace OHOS {
namespace HDI {
namespace Display {
namespace Buffer {
namespace V1_0 {
using namespace OHOS;
using namespace OHOS::HDI;
using namespace OHOS::HDI::Base;

enum {
    CMD_ALLOCATOR_GET_VERSION = 0,
    CMD_ALLOCATOR_ALLOC_MEM = 1,
};

class IAllocator : public HdiBase {
public:
    DECLARE_HDI_DESCRIPTOR(u"ohos.hdi.display.buffer.v1_0.IAllocator");

    virtual ~IAllocator() = default;

    static sptr<OHOS::HDI::Display::Buffer::V1_0::IAllocator> Get(bool isStub = false);
    static sptr<OHOS::HDI::Display::Buffer::V1_0::IAllocator> Get(const std::string &serviceName, bool isStub = false);

    virtual int32_t AllocMem(const OHOS::HDI::Display::Buffer::V1_0::AllocInfo& info, sptr<NativeBuffer>& handle) = 0;

    virtual int32_t GetVersion(uint32_t& majorVer, uint32_t& minorVer)
    {
        majorVer = 1;
        minorVer = 0;
        return HDF_SUCCESS;
    }

    virtual bool IsProxy()
    {
        return false;
    }

    virtual const std::u16string GetDesc()
    {
        return metaDescriptor_;
    }
};
} // V1_0
} // Buffer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_BUFFER_V1_0_IALLOCATOR_H