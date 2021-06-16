/*
 *   Copyright (c) 2020 Project CHIP Authors
 *   All rights reserved.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */

#include "ModelCommand.h"

#include <app/InteractionModelEngine.h>
#include <inttypes.h>

using namespace ::chip;

namespace {
constexpr uint16_t kWaitDurationInSeconds = 10;
} // namespace

void DispatchSingleClusterCommand(chip::ClusterId aClusterId, chip::CommandId aCommandId, chip::EndpointId aEndPointId,
                                  chip::TLV::TLVReader & aReader, Command * apCommandObj)
{
    ChipLogDetail(Controller, "Received Cluster Command: Cluster=%" PRIx16 " Command=%" PRIx8 " Endpoint=%" PRIx8, aClusterId,
                  aCommandId, aEndPointId);
    ChipLogError(
        Controller,
        "Default DispatchSingleClusterCommand is called, this should be replaced by actual dispatched for cluster commands");
}

CHIP_ERROR WaitForSessionSetup(chip::Controller::Device * device)
{
    constexpr time_t kWaitPerIterationSec = 1;
    constexpr uint16_t kIterationCount    = 5;

    struct timespec sleep_time;
    sleep_time.tv_sec  = kWaitPerIterationSec;
    sleep_time.tv_nsec = 0;

    for (uint32_t i = 0; i < kIterationCount && device->IsSessionSetupInProgress(); i++)
    {
        nanosleep(&sleep_time, nullptr);
    }

    ReturnErrorCodeIf(!device->IsSecureConnected(), CHIP_ERROR_TIMEOUT);

    return CHIP_NO_ERROR;
}

CHIP_ERROR ModelCommand::Run(NodeId localId, NodeId remoteId)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    //
    // Set this to true first BEFORE we send commands to ensure we don't
    // end up in a situation where the response comes back faster than we can
    // set the variable to true, which will cause it to block indefinitely.
    //
    UpdateWaitForResponse(true);

    {
        chip::DeviceLayer::StackLock lock;

        err = GetExecContext()->commissioner->GetDevice(remoteId, &mDevice);
        VerifyOrExit(err == CHIP_NO_ERROR, ChipLogError(chipTool, "Init failure! No pairing for device: %" PRIu64, localId));

        // TODO - Implement notification from device object when the secure session is available.
        // Current code is polling the device object to check for secure session availability. This should
        // be updated to a notification/callback mechanism.
        err = WaitForSessionSetup(mDevice);
        VerifyOrExit(err == CHIP_NO_ERROR,
                     ChipLogError(chipTool, "Timed out while waiting for session setup for device: %" PRIu64, localId));

        err = SendCommand(mDevice, mEndPointId);
        VerifyOrExit(err == CHIP_NO_ERROR, ChipLogError(chipTool, "Failed to send message: %s", ErrorStr(err)));
    }

    WaitForResponse(kWaitDurationInSeconds);

    VerifyOrExit(GetCommandExitStatus(), err = CHIP_ERROR_INTERNAL);

exit:
    return err;
}
