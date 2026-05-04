#include <winsock2.h>

#define NOMINMAX
#include <windows.h>

#include <iphlpapi.h>
#include <ntddndis.h>

#include <cstdio>
#include <cstdlib>

#include <string>

#include <cstdint>

#define OID_802_3_PERMANENT_ADDRESS 0x01010101

BOOL GetPermanentMacAddress(HANDLE hNetAdapter, BYTE mac[6]) {
    DWORD OID = OID_802_3_PERMANENT_ADDRESS;
    DWORD bytesReturned = 0;

    BOOL ok = DeviceIoControl(
        hNetAdapter,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &OID,
        sizeof(OID),
        mac,
        sizeof(mac),
        &bytesReturned,
        nullptr
    );

    return ok;
}

BOOL GetAdaptersInfoWrapper(PIP_ADAPTER_INFO &pAdapterInfo) {
    {
        ULONG ulOutBufLen = 0;

        {
            ULONG ok = GetAdaptersInfo(NULL, &ulOutBufLen);
            if (ok == ERROR_BUFFER_OVERFLOW) {
                pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulOutBufLen);
                if (!pAdapterInfo) {
                    printf("Out of memory\n");
                    (void)getchar();
                    return FALSE;
                }
                GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
            }
        }
    }
    return TRUE;
}

int main() {
    PIP_ADAPTER_INFO pAdapterInfo{};

    {
        BOOL ok = GetAdaptersInfoWrapper(pAdapterInfo);
        if (!ok) {
            return 1;
        }
    }

    char adapterPath[MAX_PATH];

    for (PIP_ADAPTER_INFO pAdapter = pAdapterInfo; pAdapter != nullptr; pAdapter = pAdapter->Next) {
        if (pAdapter->AddressLength == 0) {
            continue;
        }

        const char *adapterName = pAdapter->AdapterName;

        memset(adapterPath, 0, sizeof(adapterPath));

        sprintf_s(adapterPath, "\\\\.\\%s", adapterName);

        HANDLE hAdapter = CreateFileA(
            adapterPath,
            GENERIC_READ | GENERIC_WRITE,
            NULL,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (hAdapter == INVALID_HANDLE_VALUE) {
            continue;
        }

        BYTE driverMac[6];

        {
            BOOL ok = GetPermanentMacAddress(hAdapter, driverMac);
            if (!ok) {
                continue;
            }
        }

        printf("%-40s", "MAC address reported by the system: ");
        for (uint8_t i = 0; i < pAdapter->AddressLength; ++i) {
            printf("%02X ", pAdapter->Address[i]);
        }

        printf("\n");

        printf("%-40s", "MAC address reported by driver: ");
        for (int i = 0; i < 6; ++i) {
            printf("%02X ", driverMac[i]);
        }

        printf("\n");

        if (pAdapter->Next != nullptr) {
            printf("\n");
        }
    }

    free(pAdapterInfo);

    (void)getchar();
}