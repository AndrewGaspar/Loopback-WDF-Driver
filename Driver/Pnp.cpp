#include "Pnp.h"
#include "Pnp.tmh"

#include "Device.h"

VOID
LoopbackDeviceInitSetPnpPowerCallbacks(_In_ PWDFDEVICE_INIT deviceInit)
{
    TraceEntry();

    WDF_PNPPOWER_EVENT_CALLBACKS callbacks;

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&callbacks);

    //
    // Register pnp/power callback.
    //
    callbacks.EvtDevicePrepareHardware = LoopbackEvtDevicePrepareHardware;
    callbacks.EvtDeviceReleaseHardware = LoopbackEvtDeviceReleaseHardware;

    callbacks.EvtDeviceD0Entry = LoopbackEvtDeviceD0Entry;
    callbacks.EvtDeviceD0Exit = LoopbackEvtDeviceD0Exit;

    //
    // Register the PnP and power callbacks. Power policy related callbacks will be registered
    // later in SotwareInit.
    //
    WdfDeviceInitSetPnpPowerEventCallbacks(deviceInit, &callbacks);
}

NTSTATUS
LoopbackEvtDevicePrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated
    )
{
    PLOOPBACK_DEVICE_CONTEXT deviceContext;

    UNREFERENCED_PARAMETER(ResourcesTranslated);

    TraceEntry();

    deviceContext = GetDeviceContext(Device);

    NTSTATUS status = STATUS_SUCCESS;

    //
    // NOTE: For the purpose of this driver this resource code is
    // theoretical. This is a virtual device, as a result there are no real 
    // resources to be walked.
    //
    for (ULONG i = 0; i < WdfCmResourceListGetCount(ResourcesRaw); i++) {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR rawDesc =
            WdfCmResourceListGetDescriptor(ResourcesRaw, i);

        switch (rawDesc->Type) {
        case CmResourceTypeInterrupt:
            // Create and configure an interrupt object
            break;
        case CmResourceTypeMemory:
            // Map device memory to system memory
            break;
        default:
            // etc.
            break;
        }
    }

    return status;
}

NTSTATUS
LoopbackEvtDeviceReleaseHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesTranslated
    )
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(ResourcesTranslated);
    NTSTATUS status = STATUS_SUCCESS;

    TraceEntry();

    TraceStatusAndReturn(status);
}

NTSTATUS
LoopbackEvtDeviceD0Entry(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
    )
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(PreviousState);
    NTSTATUS status = STATUS_SUCCESS;

    TraceEntry();

    TraceStatusAndReturn(status);
}

NTSTATUS
LoopbackEvtDeviceD0Exit(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState
    )
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(TargetState);
    NTSTATUS status = STATUS_SUCCESS;

    TraceEntry();

    TraceStatusAndReturn(status);
}
