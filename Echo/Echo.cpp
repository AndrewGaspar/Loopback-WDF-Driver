// Echo.cpp : Defines the entry point for the console application.
//

#define INITGUID

#include "stdafx.h"

#include <Windows.h>
#include <strsafe.h>

#include "Loopback.h"

#include <setupapi.h>
#include <cfgmgr32.h>

#include <string>
#include <vector>
#include <iostream>
#include <sstream>

BOOL
GetDevicePath(
    _In_ LPGUID InterfaceGuid,
    _Out_writes_(BufLen) PWCHAR DevicePath,
    _In_ size_t BufLen
    )
{
    CONFIGRET cr = CR_SUCCESS;
    PWSTR deviceInterfaceList = NULL;
    ULONG deviceInterfaceListLength = 0;
    PWSTR nextInterface;
    HRESULT hr = E_FAIL;
    BOOL bRet = TRUE;

    cr = CM_Get_Device_Interface_List_Size(
        &deviceInterfaceListLength,
        InterfaceGuid,
        NULL,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        printf("Error 0x%x retrieving device interface list size.\n", cr);
        goto clean0;
    }

    if (deviceInterfaceListLength <= 1) {
        bRet = FALSE;
        printf("Error: No active device interfaces found.\n"
            " Is the device driver loaded?");
        goto clean0;
    }

    deviceInterfaceList = (PWSTR)malloc(deviceInterfaceListLength * sizeof(WCHAR));
    if (deviceInterfaceList == NULL) {
        printf("Error allocating memory for device interface list.\n");
        goto clean0;
    }
    ZeroMemory(deviceInterfaceList, deviceInterfaceListLength * sizeof(WCHAR));

    cr = CM_Get_Device_Interface_List(
        InterfaceGuid,
        NULL,
        deviceInterfaceList,
        deviceInterfaceListLength,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        printf("Error 0x%x retrieving device interface list.\n", cr);
        goto clean0;
    }

    nextInterface = deviceInterfaceList + wcslen(deviceInterfaceList) + 1;
    if (*nextInterface != UNICODE_NULL) {
        printf("Warning: More than one device interface instance found. \n"
            "Selecting first matching device.\n\n");
    }

    hr = StringCchCopy(DevicePath, BufLen, deviceInterfaceList);
    if (FAILED(hr)) {
        bRet = FALSE;
        printf("Error: StringCchCopy failed with HRESULT 0x%x", hr);
        goto clean0;
    }

clean0:
    if (deviceInterfaceList != NULL) {
        free(deviceInterfaceList);
    }
    if (CR_SUCCESS != cr) {
        bRet = FALSE;
    }

    return bRet;
}

int wmain(int numArgs, WCHAR ** args)
{
    HDEVINFO devInfo;
    HANDLE file;
    WCHAR devicePath[MAX_PATH];
    std::wstring message;
    std::vector<uint8_t> response;
    int newFileSize = -1;

    args++;
    numArgs--;

    std::wcout << numArgs << std::endl;

    if (numArgs > 0)
    {
        std::wstringstream stream(args[0]);
        stream >> newFileSize;
    }

    if (!GetDevicePath((LPGUID)&GUID_DEVINTERFACE_LOOPBACK,
        devicePath,
        sizeof(devicePath) / sizeof(devicePath[0])))
    {
        std::wcout << L"Could not get device path" << std::endl;
        goto Cleanup;
    }

    file = CreateFile(
        devicePath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (file == INVALID_HANDLE_VALUE)
    {
        auto error = GetLastError();

        std::wcout << L"Could not open device for read/write with error " << error << std::endl;
        goto Cleanup;
    }

    if (newFileSize > 0)
    {
        UINT newSize = (UINT)newFileSize;
        std::wcout << L"Changing buffer size to " << newSize << std::endl;


        if (!DeviceIoControl(
            file,
            LOOPBACK_IOCTL_ALTER_MAX_LENGTH,
            (PVOID)&newSize,
            sizeof(newSize),
            NULL,
            0,
            NULL,
            NULL))
        {
            auto error = GetLastError();
            std::wcout << L"Could not send IOCTL with error " << error << std::endl;
            goto Cleanup;
        }
    }

    std::getline(std::wcin, message);

    DWORD written, read;
    if (!WriteFile(
        file,
        (PCVOID)message.data(),
        (message.size() + 1)*sizeof(WCHAR),
        &written,
        NULL))
    {
        auto err = GetLastError();
        std::wcout << L"Failed to write " << message.c_str() << L" with error " << err << std::endl;
        goto Cleanup;
    }

    response.resize(written);

    if (!ReadFile(
        file,
        (LPVOID)response.data(),
        response.size(),
        &read,
        NULL))
    {
        auto err = GetLastError();
        std::wcout << L"Failed to read with error " << err << std::endl;
        goto Cleanup;
    }

    message = (LPCWSTR)response.data();

    std::wcout << L"Response: " << message << std::endl;

Cleanup:

    if (file != INVALID_HANDLE_VALUE)
    {
        CloseHandle(file);
    }

    if (devInfo)
    {
        SetupDiDestroyDeviceInfoList(devInfo);
    }

    return 0;
}

