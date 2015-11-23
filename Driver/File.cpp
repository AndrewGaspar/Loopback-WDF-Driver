#include "File.h"
#include "File.tmh"

#include "Device.h"

VOID
LoopbackDeviceInitSetFileObjectCallbacks(_In_ PWDFDEVICE_INIT deviceInit)
{
    TraceEntry();

    WDF_FILEOBJECT_CONFIG config;
    
    WDF_FILEOBJECT_CONFIG_INIT(
        &config,
        LoopbackEvtDeviceFileCreate,
        LoopbackEvtFileClose,
        WDF_NO_EVENT_CALLBACK);

    WdfDeviceInitSetFileObjectConfig(
        deviceInit, &config, WDF_NO_OBJECT_ATTRIBUTES);
}

VOID
LoopbackEvtDeviceFileCreate(
    _In_ WDFDEVICE     Device,
    _In_ WDFREQUEST    Request,
    _In_ WDFFILEOBJECT FileObject
    )
{
    UNREFERENCED_PARAMETER(FileObject);

    TraceEntry();

    auto context = GetDeviceContext(Device);

    if (context->OpenFiles == 2)
    {
        WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);
    }
    else
    {
        context->OpenFiles++;
        WdfRequestComplete(Request, STATUS_SUCCESS);
    }
}

VOID LoopbackEvtFileClose(
    _In_ WDFFILEOBJECT FileObject
    )
{
    TraceEntry();

    auto device = WdfFileObjectGetDevice(FileObject);

    auto context = GetDeviceContext(device);

    context->OpenFiles--;
}