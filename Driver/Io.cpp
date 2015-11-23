#include "Io.h"
#include "Io.tmh"

#include "Device.h"

NTSTATUS
LoopbackDeviceCreateIoQueue(_In_ WDFDEVICE device)
{
    TraceEntry();
    
    NTSTATUS status = STATUS_SUCCESS;
    WDF_IO_QUEUE_CONFIG config;

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
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&config, WdfIoQueueDispatchSequential);
    config.PowerManaged = WdfTrue;
    config.EvtIoWrite = LoopbackEvtIoWrite;
    config.EvtIoRead = LoopbackEvtIoRead;
    config.EvtIoDeviceControl = LoopbackEvtIoDeviceControl;

    status = WdfIoQueueCreate(
        device,
        &config,
        WDF_NO_OBJECT_ATTRIBUTES,
        nullptr);

    TraceStatusAndReturn(status);
}

VOID
ForwardRequestToIoTarget(
    _In_ WDFQUEUE   queue,
    _In_ WDFREQUEST request,
    _In_ size_t     length)
{
    TraceEntry();
    Trace(TRACE_LEVEL_INFORMATION, "%!FUNC! - Queue 0x%p, Request 0x%p Length %Iu", queue, request, length);

    auto device = WdfIoQueueGetDevice(queue);
    auto context = GetDeviceContext(device);

    if (length > context->MaxLengthInBytesForRWTransfers) {
        TraceError("%!FUNC! - Buffer Length to big %Iu, Max is %Iu. Status - %!STATUS!",
            length, context->MaxLengthInBytesForRWTransfers, STATUS_BUFFER_OVERFLOW);

        WdfRequestCompleteWithInformation(request, STATUS_BUFFER_OVERFLOW, NULL);
        return;
    }

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

VOID
LoopbackEvtIoWrite(
    _In_ WDFQUEUE   queue,
    _In_ WDFREQUEST request,
    _In_ size_t     length
    )
{
    TraceEntry();

    ForwardRequestToIoTarget(queue, request, length);
}

VOID
LoopbackEvtIoRead(
    _In_ WDFQUEUE   queue,
    _In_ WDFREQUEST request,
    _In_ size_t     length
    )
{
    TraceEntry();

    ForwardRequestToIoTarget(queue, request, length);
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

    if (IoControlCode != LOOPBACK_IOCTL_ALTER_MAX_LENGTH || InputBufferLength != 4)
    {
        status = STATUS_UNSUCCESSFUL;

        goto Cleanup;
    }

    PUINT32 buffer;
    status = WdfRequestRetrieveInputBuffer(Request, 4, (PVOID*)&buffer, nullptr);

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
