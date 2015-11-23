#define INITGUID

#include <ntddk.h>
//#include <Windows.h>
#include <wdf.h>

#include "trace.h"
#include "driver.tmh"


// Declarations are in a single file for easy reference while working
// on the labs. You can collapse the section in Visual studio IDE.

//
// Loopback device specific settings are saved in the device context
//
typedef struct _LOOPBACK_DEVICE_CONTEXT
{
    // Store the maximum supported length for read and write requests
    size_t MaxLengthInBytesForRWTransfers;

    // number of open files
    size_t OpenFiles;
} LOOPBACK_DEVICE_CONTEXT, *PLOOPBACK_DEVICE_CONTEXT;

//
// This macro will generate an inline function called GetDeviceContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(LOOPBACK_DEVICE_CONTEXT, GetDeviceContext)

//
// Unique device interface GUID
//
// 38553a6e-35a9-acee-bd18-3201f198a68b
DEFINE_GUID(GUID_DEVINTERFACE_LOOPBACK,
    0x38553a6e, 0x35a9, 0xacee, 0xbd, 0x18, 0x32, 0x01, 0xf1, 0x98, 0xa6, 0x8b);

#define LOOPBACK_IOCTL_ALTER_MAX_LENGTH 0x3245

extern "C" {

    DRIVER_INITIALIZE DriverEntry;
    EVT_WDF_DRIVER_DEVICE_ADD LoopbackEvtDeviceAdd;
    EVT_WDF_DRIVER_UNLOAD LoopbackEvtDriverUnload;
    EVT_WDF_OBJECT_CONTEXT_CLEANUP LoopbackEvtDriverCleanup;

    //
    // Device events
    //
    EVT_WDF_DEVICE_PREPARE_HARDWARE LoopbackEvtDevicePrepareHardware;
    EVT_WDF_DEVICE_RELEASE_HARDWARE LoopbackEvtDeviceReleaseHardware;

    EVT_WDF_DEVICE_D0_ENTRY LoopbackEvtDeviceD0Entry;
    EVT_WDF_DEVICE_D0_EXIT LoopbackEvtDeviceD0Exit;

    EVT_WDF_IO_QUEUE_IO_READ LoopbackEvtIoRead;
    EVT_WDF_IO_QUEUE_IO_WRITE LoopbackEvtIoWrite;
    EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL LoopbackEvtIoDeviceControl;

    EVT_WDF_DEVICE_FILE_CREATE LoopbackEvtDeviceFileCreate;
    EVT_WDF_FILE_CLOSE LoopbackEvtFileClose;
    

    VOID ForwardRequestToIoTarget(_In_ WDFDEVICE device, _In_ WDFREQUEST request);

    NTSTATUS
    DriverEntry(
        _In_ PDRIVER_OBJECT     DriverObject,
        _In_ PUNICODE_STRING    RegistryPath)
    {
        WDF_DRIVER_CONFIG config = {};
        WDF_OBJECT_ATTRIBUTES attributes = {};
        NTSTATUS status = STATUS_SUCCESS;

        // Init WPP tracing - WPP traces can be viewed from your debugger or a standalone .exe
        WPP_INIT_TRACING(DriverObject, RegistryPath);

        Trace(TRACE_LEVEL_INFORMATION, "Entering %!FUNC!.");

        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

        attributes.EvtCleanupCallback = LoopbackEvtDriverCleanup;

        WDF_DRIVER_CONFIG_INIT(&config, LoopbackEvtDeviceAdd);
        config.EvtDriverUnload = LoopbackEvtDriverUnload;

        status = WdfDriverCreate(DriverObject, RegistryPath, &attributes, &config, WDF_NO_HANDLE);

        return status;
    }

    NTSTATUS
    LoopbackEvtDeviceAdd(
        _In_ WDFDRIVER Driver,
        _In_ PWDFDEVICE_INIT DeviceInit)
    {
        UNREFERENCED_PARAMETER(Driver);

        NTSTATUS status = STATUS_SUCCESS;

        WDF_OBJECT_ATTRIBUTES attributes;
        WDFDEVICE device;
        PLOOPBACK_DEVICE_CONTEXT deviceContext;

        WDFQUEUE wdfQueue;
        WDF_IO_QUEUE_CONFIG queueConfig;

        WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;

        WDF_OBJECT_ATTRIBUTES fileObjectAttributes;
        WDF_FILEOBJECT_CONFIG fileObjectConfig;

        WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;

        TraceEntry();

        WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

        //
        // Register pnp/power callback.
        //
        pnpPowerCallbacks.EvtDevicePrepareHardware = LoopbackEvtDevicePrepareHardware;
        pnpPowerCallbacks.EvtDeviceReleaseHardware = LoopbackEvtDeviceReleaseHardware;

        pnpPowerCallbacks.EvtDeviceD0Entry = LoopbackEvtDeviceD0Entry;
        pnpPowerCallbacks.EvtDeviceD0Exit = LoopbackEvtDeviceD0Exit;

        //
        // Register the PnP and power callbacks. Power policy related callbacks will be registered
        // later in SotwareInit.
        //
        WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

        //
        // Configure the device attributes so that a context is allocated
        //
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, LOOPBACK_DEVICE_CONTEXT);

        WDF_OBJECT_ATTRIBUTES_INIT(&fileObjectAttributes);

        fileObjectAttributes.SynchronizationScope = WdfSynchronizationScopeNone;

        WDF_FILEOBJECT_CONFIG_INIT(
            &fileObjectConfig, 
            LoopbackEvtDeviceFileCreate, 
            LoopbackEvtFileClose, 
            WDF_NO_EVENT_CALLBACK);

        WdfDeviceInitSetFileObjectConfig(
            DeviceInit, &fileObjectConfig, &fileObjectAttributes);

        status = WdfDeviceCreate(&DeviceInit, &attributes, &device);
        if (!NT_SUCCESS(status))
        {
            TraceError("%!FUNC! - WdfDeviceCreate failed with error %!STATUS!", status);
            goto Cleanup;
        }

        //
        // Get the device context and initialize it. GetDeviceContext is an
        // inline function generated by WDF_DECLARE_CONTEXT_TYPE_WITH_NAME.
        // This function will do the type checking and return
        // the device context. If you pass a wrong object  handle
        // it will return NULL and assert if run under framework verifier mode.
        //
        deviceContext = GetDeviceContext(device);
        deviceContext->MaxLengthInBytesForRWTransfers = 128;
        deviceContext->OpenFiles = 0;

        //
        // Create a device interface so that application can find and talk
        // to us.
        //
        status = WdfDeviceCreateDeviceInterface(device,
            &GUID_DEVINTERFACE_LOOPBACK,
            NULL); // Reference string
        if (!NT_SUCCESS(status)) {
            TraceError("%!FUNC! - WdfDeviceCreateDeviceInterface failed with error: %!STATUS!", status);
            goto Cleanup;
        }

        //
        // Register I/O callbacks to tell the framework that you are interested
        // in handling read and write requests.
        //
        // A default queue is where requests are delivered by the framework 
        // by default unless specific request handlers are configured using 
        // WdfDeviceConfigureRequestDispatching.
        //
        // WdfIoQueueDispatchSequential means that the queue is setup to dispatch 
        // no more than one WDFREQUEST at a time, so driver does not need to
        // protect data that could be accessed by the queue's callbacks concurrently
        //
        WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig,
            WdfIoQueueDispatchSequential);
        queueConfig.PowerManaged = WdfTrue;
        queueConfig.EvtIoWrite = LoopbackEvtIoWrite;
        queueConfig.EvtIoRead = LoopbackEvtIoRead;
        queueConfig.EvtIoDeviceControl = LoopbackEvtIoDeviceControl;

        status = WdfIoQueueCreate(
            device,
            &queueConfig,
            WDF_NO_OBJECT_ATTRIBUTES,
            &wdfQueue);

        if (!NT_SUCCESS(status)) {
            TraceError("%!FUNC! - WdfIoQueueCreate failed with error: %!STATUS!", status);
            goto Cleanup;
        }

        WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleCanWakeFromS0);
        idleSettings.IdleTimeout = 2000;
        idleSettings.Enabled = WdfTrue;
        idleSettings.DxState = PowerDeviceD1;

        status = WdfDeviceAssignS0IdleSettings(device, &idleSettings);
        if (!NT_SUCCESS(status)) {
            TraceError("%!FUNC! - WdfDeviceAssignS0IdleSettings failed with error: %!STATUS!", status);
            goto Cleanup;
        }

        status = STATUS_SUCCESS;

    Cleanup:

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

        //NT_ASSERT(context->OpenFiles <= 2);

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

    VOID
    LoopbackEvtIoWrite(
        _In_ WDFQUEUE   queue,
        _In_ WDFREQUEST request,
        _In_ size_t     length
        )
    {
        WDFMEMORY memory;
        WDFDEVICE device;
        PLOOPBACK_DEVICE_CONTEXT deviceContext;
        NTSTATUS status;

        //TraceEntry();
        //Trace(TRACE_LEVEL_INFORMATION, "%!FUNC! - Queue 0x%p, Request 0x%p Length %Iu", queue, request, length);

        device = WdfIoQueueGetDevice(queue);
        deviceContext = GetDeviceContext(device);

        if (length > deviceContext->MaxLengthInBytesForRWTransfers) {
            TraceError("%!FUNC! - Buffer Length to big %Iu, Max is %Iu. Status - %!STATUS!",
                length, deviceContext->MaxLengthInBytesForRWTransfers, STATUS_BUFFER_OVERFLOW);

            WdfRequestCompleteWithInformation(request, STATUS_BUFFER_OVERFLOW, 0L);
            return;
        }

        //
        // Retrieve the memory for the incoming request. This is for illustration only.
        // This device does not process the retrieved memory.
        //
        status = WdfRequestRetrieveInputMemory(request, &memory);
        if (!NT_SUCCESS(status)) {
            TraceError("%!FUNC! - Could not get request memory buffer. Status - %!STATUS!",
                status);

            WdfRequestComplete(request, status);
            return;
        }

        ForwardRequestToIoTarget(device, request);
    }

    VOID
    LoopbackEvtIoRead(
        _In_ WDFQUEUE   queue,
        _In_ WDFREQUEST request,
        _In_ size_t     length
        )
    {
        WDFDEVICE device;
        PLOOPBACK_DEVICE_CONTEXT deviceContext;

        TraceEntry();
        Trace(TRACE_LEVEL_INFORMATION, "%!FUNC! - Queue 0x%p, Request 0x%p Length %Iu", queue, request, length);

        device = WdfIoQueueGetDevice(queue);
        deviceContext = GetDeviceContext(device);

        if (length > deviceContext->MaxLengthInBytesForRWTransfers) {
            TraceError("%!FUNC! - Buffer Length to big %Iu, Max is %Iu. Status - %!STATUS!",
                length, deviceContext->MaxLengthInBytesForRWTransfers, STATUS_BUFFER_OVERFLOW);

            WdfRequestCompleteWithInformation(request, STATUS_BUFFER_OVERFLOW, 0L);
            return;
        }

        //
        // For now complete the request. In the next step we will have the underlying
        // bus driver process the request.
        //
        ForwardRequestToIoTarget(device, request);
    }

    VOID
    LoopbackEvtIoDeviceControl(
        _In_ WDFQUEUE   Queue,
        _In_ WDFREQUEST Request,
        _In_ size_t     OutputBufferLength,
        _In_ size_t     InputBufferLength,
        _In_ ULONG      IoControlCode)
    {
        UNREFERENCED_PARAMETER((OutputBufferLength, InputBufferLength));

        TraceEntry();

        NTSTATUS status = STATUS_SUCCESS;
        PUINT32 buffer;
        size_t length;

        if (IoControlCode != LOOPBACK_IOCTL_ALTER_MAX_LENGTH || InputBufferLength != 4)
        {
            status = STATUS_UNSUCCESSFUL;

            goto Cleanup;
        }

        status = WdfRequestRetrieveInputBuffer(Request, 4, (PVOID*)&buffer, &length);

        if (!NT_SUCCESS(status))
        {
            TraceError("%!FUNC! - Unable to read IOCTL input buffer. Status - %!STATUS!", status);
            
            goto Cleanup;
        }

        auto device = WdfIoQueueGetDevice(Queue);
        auto context = GetDeviceContext(device);

        context->MaxLengthInBytesForRWTransfers = *buffer;

    Cleanup:

        WdfRequestComplete(Request, status);
    }

    VOID
    ForwardRequestToIoTarget(
        _In_ WDFDEVICE device, 
        _In_ WDFREQUEST request)
    {
        TraceEntry();

        auto targetDevice = WdfDeviceGetIoTarget(device);

        WdfRequestFormatRequestUsingCurrentType(request);

        WDF_REQUEST_SEND_OPTIONS options;

        WDF_REQUEST_SEND_OPTIONS_INIT(
            &options,
            WDF_REQUEST_SEND_OPTION_SYNCHRONOUS | WDF_REQUEST_SEND_OPTION_TIMEOUT);

        WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&options, WDF_ABS_TIMEOUT_IN_SEC(10));

        auto sendSuccess = WdfRequestSend(request, targetDevice, &options);
        auto status = WdfRequestGetStatus(request);
        if (!sendSuccess || !NT_SUCCESS(status))
        {
            TraceError("%!FUNC! - WdfRequestSend returned %d with status: %!STATUS!", sendSuccess, status);
            WdfRequestCompleteWithInformation(request, status, NULL);
            return;
        }

        WdfRequestComplete(request, status);
    }
}