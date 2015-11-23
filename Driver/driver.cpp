#include "Driver.h"
#include "Driver.tmh"

#include "Device.h"

NTSTATUS
InitializeDriver(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath)
{
    NTSTATUS status = STATUS_SUCCESS;

    WDF_DRIVER_CONFIG config = {};
    WDF_OBJECT_ATTRIBUTES attributes = {};

    // Init WPP tracing - WPP traces can be viewed from your debugger or a standalone .exe
    WPP_INIT_TRACING(driverObject, registryPath);

    TraceEntry();

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    attributes.EvtCleanupCallback = LoopbackEvtDriverCleanup;

    WDF_DRIVER_CONFIG_INIT(&config, LoopbackEvtDeviceAdd);
    config.EvtDriverUnload = LoopbackEvtDriverUnload;

    status = WdfDriverCreate(driverObject, registryPath, &attributes, &config, WDF_NO_HANDLE);

    return status;
}

VOID
LoopbackEvtDriverCleanup(
    _In_ WDFOBJECT Object)
{
    UNREFERENCED_PARAMETER(Object);

    TraceEntry();

    WPP_CLEANUP(Object);
}

VOID
LoopbackEvtDriverUnload(
    _In_ WDFDRIVER Driver)
{
    UNREFERENCED_PARAMETER(Driver);

    TraceEntry();
}
