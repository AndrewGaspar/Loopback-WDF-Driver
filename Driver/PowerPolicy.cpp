#include "PowerPolicy.h"
#include "PowerPolicy.tmh"

NTSTATUS LoopbackDeviceSetPowerPolicy(_In_ WDFDEVICE device)
{
    TraceEntry();

    NTSTATUS status = STATUS_SUCCESS;
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;

    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleCanWakeFromS0);
    idleSettings.IdleTimeout = 2000;
    idleSettings.Enabled = WdfTrue;
    idleSettings.DxState = PowerDeviceD1;

    status = WdfDeviceAssignS0IdleSettings(device, &idleSettings);

    TraceStatusAndReturn(status);
}