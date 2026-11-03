/* SPDX-License-Identifier: Apache-2.0
 * Exercise the real donor Linux input backend in an empty device namespace.
 * No physical input node, kernel module, Binder device or HDF driver is used.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "input_manager.h"

int main(void)
{
    if (access("/hmos-input-test-namespace", F_OK) || !access("/dev/hdf", F_OK)) {
        fputs("Run this test only in its isolated empty-device namespace.\n", stderr);
        return 2;
    }
    IInputInterface *input = NULL;
    assert(GetInputInterface(&input) == INPUT_SUCCESS);
    assert(input && input->iInputManager && input->iInputController && input->iInputReporter);
    uint32_t count = 0;
    InputDeviceInfo *devices = NULL;
    /* No devices exist: the actual backend must return its real failure,
     * rather than fabricated device information or successful enumeration. */
    assert(input->iInputManager->GetInputDeviceList(&count, &devices, 32) != INPUT_SUCCESS);
    assert(devices == NULL && count == 0);
    assert(input->iInputManager->OpenInputDevice(UINT32_MAX) != INPUT_SUCCESS);
    ReleaseInputInterface(&input);
    assert(input == NULL);
    puts("PASS real ARM64 Linux input backend initializes without HDF/Binder devices; empty/invalid queries fail correctly");
    fflush(stdout);
    /* The donor owns detached background threads; end the test process group
     * without treating its process-global singleton as a hot-unload target. */
    _Exit(0);
}
