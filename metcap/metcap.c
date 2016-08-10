/* metcap.c : Defines the entry point for the application.
*
* Copyright 2013, New Tribes Mission Inc, (ntm.org)
*
* All rights reserved.
*
* This file is part of MetCAP. MetCAP is released under
* GNU General Public License, version 2.
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*
*/

/*
* This simple program to setup a uni-directional, user-level bridge to
* create a soft-tap, like daemonlogger.
* It opens two adapters specified by the user and starts a packet
* copying thread. It receives packets from adapter 1 and sends them down
* to adapter 2.
*/

// options:
//        -e: Enumerate devices
//		  -m <in> <out> Mirror traffic from <in> interface to <out> interface
//

#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4127)   // conditional expression is constant

#include <signal.h>
#include <io.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <share.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <winioctl.h>
#include <memory.h>
#include <ctype.h>
#include <malloc.h>

#include <winerror.h>
#include <winsock.h>
#include <intsafe.h>

#include <ntddndis.h>
#include "protuser.h"
#include "kafka.h"

// this is needed to prevent compiler from complaining about
// pragma prefast statements below
#ifndef _PREFAST_
#pragma warning(disable:4068)
#endif

#ifndef NDIS_STATUS
#define NDIS_STATUS     ulong
#endif

#if DBG
#define debug(stmt)    printf stmt
#else
#define debug(stmt)
#endif

#define PRINTF(stmt)    printf stmt

#ifndef MAC_ADDR_LEN
#define MAC_ADDR_LEN                    6
#endif

#define ulong ULONG
#define MAX_NDIS_DEVICE_NAME_LEN        256

char            NdisProtDevice[] = "\\\\.\\\\Metcap";
char *          pNdisProtDevice = &NdisProtDevice[0];

BOOLEAN         DoEnumerate = FALSE;
BOOLEAN         DoReads = FALSE;
int             NumberOfPackets = -1;
ulong           PacketLength = 65536;	// Portion of the packet to capture.
										// 65536 grants that the whole packet will be captured on every link layer.

unsigned char           SrcMacAddr[MAC_ADDR_LEN];
//unsigned char           DstMacAddr[MAC_ADDR_LEN];
unsigned char           FakeSrcMacAddr[MAC_ADDR_LEN] = { 0 };

BOOLEAN         bDstMacSpecified = FALSE;
//char *          pNdisDeviceName = "JUNK";
unsigned short          EthType = 0x8e88;
BOOLEAN         bUseFakeAddress = FALSE;

HANDLE      DeviceHandleIn;
HANDLE      DeviceHandleOut;
int			DeviceIndex1, DeviceIndex2;

#include <pshpack1.h>

typedef struct _ETH_HEADER {
	unsigned char       DstAddr[MAC_ADDR_LEN];
	unsigned char       SrcAddr[MAC_ADDR_LEN];
	unsigned short      EthType;
} ETH_HEADER, *PETH_HEADER;

#include <poppack.h>


/* Storage data structure used to pass parameters to the threads */
typedef struct _REFLECT_PARAMS {
	unsigned int state;        /* Some simple state information */
	HANDLE hNdisDeviceIn;
	char* kafka_topic;
	char* kafka_config_path;
} REFLECT_PARAMS, *PREFLECT_PARAMS;

int wmain(int argc, _TCHAR * argv[]);

/* Prototypes */
DWORD WINAPI CaptureAndForwardThread(LPVOID lpParameter);
void ctrlc_handler(int sig);

/* This prevents the two threads to mess-up when they do printfs */
CRITICAL_SECTION print_cs;

/* Thread handlers. Global because we wait on the threads from the CTRL+C handler */
HANDLE threads[2];

/* This global variable tells the forwarder threads they must terminate */
volatile int kill_forwaders = 0;

/*******************************************************************/

HANDLE
OpenHandle(
	_In_ PSTR pDeviceName
) {
	DWORD   DesiredAccess;
	DWORD   ShareMode;
	LPSECURITY_ATTRIBUTES   lpSecurityAttributes = NULL;

	DWORD   CreationDistribution;
	DWORD   FlagsAndAttributes;
	HANDLE  Handle;
	DWORD   BytesReturned;

	DesiredAccess = GENERIC_READ | GENERIC_WRITE;
	ShareMode = 0;
	CreationDistribution = OPEN_EXISTING;
	FlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;

	Handle = CreateFileA(
		pDeviceName,
		DesiredAccess,
		ShareMode,
		lpSecurityAttributes,
		CreationDistribution,
		FlagsAndAttributes,
		NULL
	);
	if (Handle == INVALID_HANDLE_VALUE) {
		debug(("Creating file failed, error %x\n", GetLastError()));
		return Handle;
	}
	//
	//  Wait for the driver to finish binding.
	//
	if (!DeviceIoControl(
		Handle,
		IOCTL_NDISPROT_BIND_WAIT,
		NULL,
		0,
		NULL,
		0,
		&BytesReturned,
		NULL)) {
		debug(("IOCTL_NDISIO_BIND_WAIT failed, error %x\n", GetLastError()));
		CloseHandle(Handle);
		Handle = INVALID_HANDLE_VALUE;
	}

	return (Handle);
}


BOOL
OpenNdisDevice(
	_In_ HANDLE                     Handle,
	_In_ PCTSTR                      pszDeviceName,
	_In_ SIZE_T						lDeviceNameLength
) {
	DWORD   BytesReturned;
	DWORD	DeviceNameLength;
	HRESULT hr = S_OK;


	PRINTF(("\nTrying to access NDIS Device: %ws \n", pszDeviceName));
	OutputDebugString(_T("metcap: "));
	OutputDebugString(_T("Trying to access NDIS Device: "));
	OutputDebugString(pszDeviceName);
	OutputDebugString(_T("\n"));

	hr = SIZETToDWord(lDeviceNameLength, &DeviceNameLength);
	if (FAILED(hr)) {
		PRINTF(("\nFailed to access NDIS Device: %ws \n", pszDeviceName));
		return FALSE;
	}

	return (DeviceIoControl(
		Handle,
		IOCTL_NDISPROT_OPEN_DEVICE,
		(LPVOID)pszDeviceName,
		DeviceNameLength,
		NULL,
		0,
		&BytesReturned,
		NULL));
}

_Success_(return)
BOOL
GetSrcMac(
	_In_ HANDLE  Handle,
	_Out_writes_bytes_(MAC_ADDR_LEN) PUCHAR  pSrcMacAddr
) {
	DWORD       BytesReturned;
	BOOLEAN     bSuccess;
	unsigned char       QueryBuffer[sizeof(NDISPROT_QUERY_OID) + MAC_ADDR_LEN];
	PNDISPROT_QUERY_OID  pQueryOid;


	debug(("Trying to get src mac address\n"));

	pQueryOid = (PNDISPROT_QUERY_OID)&QueryBuffer[0];
	pQueryOid->Oid = OID_802_3_CURRENT_ADDRESS;
	pQueryOid->PortNumber = 0;

	bSuccess = (BOOLEAN)DeviceIoControl(
		Handle,
		IOCTL_NDISPROT_QUERY_OID_VALUE,
		(LPVOID)&QueryBuffer[0],
		sizeof(QueryBuffer),
		(LPVOID)&QueryBuffer[0],
		sizeof(QueryBuffer),
		&BytesReturned,
		NULL);

	if (bSuccess) {
		debug(("GetSrcMac: IoControl success, BytesReturned = %d\n",
			BytesReturned));

#pragma warning(suppress:6202) // buffer overrun warning - enough space allocated in QueryBuffer
		memcpy(pSrcMacAddr, pQueryOid->Data, MAC_ADDR_LEN);
	}
	else {
		debug(("GetSrcMac: IoControl failed: %d\n", GetLastError()));
	}

	return (bSuccess);
}


_Success_(return)
int
EnumerateDevices(
	_In_ HANDLE  Handle
) {
	typedef __declspec(align(MEMORY_ALLOCATION_ALIGNMENT)) QueryBindingCharBuf;
	QueryBindingCharBuf		Buf[1024];
	DWORD       		BufLength = sizeof(Buf);
	DWORD       		BytesWritten;
	DWORD       		i;
	PNDISPROT_QUERY_BINDING 	pQueryBinding;

	pQueryBinding = (PNDISPROT_QUERY_BINDING)Buf;

	i = 0;
	for (pQueryBinding->BindingIndex = i;
		/* NOTHING */;
		pQueryBinding->BindingIndex = ++i) {
		if (DeviceIoControl(
			Handle,
			IOCTL_NDISPROT_QUERY_BINDING,
			pQueryBinding,
			sizeof(NDISPROT_QUERY_BINDING),
			Buf,
			BufLength,
			&BytesWritten,
			NULL)) {
			PRINTF(("%2d. %ws\n     - %ws\n",
				pQueryBinding->BindingIndex,
				(WCHAR *)((PUCHAR)pQueryBinding + pQueryBinding->DeviceNameOffset),
				(WCHAR *)((PUCHAR)pQueryBinding + pQueryBinding->DeviceDescrOffset)));

			memset(Buf, 0, BufLength);
		}
		else {
			ulong   rc = GetLastError();
			if (rc != ERROR_NO_MORE_ITEMS) {
				PRINTF(("\nNo interfaces found! Make sure MetCap driver is installed.\n, error %d\n", rc));
			}
			break;
		}
	}

	return i;
}


BOOL
GetDevice(
	_In_ HANDLE  Handle,
	_In_ int  bindingIndex,
	_Out_ PTSTR pDeviceName,
	_Out_ PSIZE_T pDeviceNameLength
) {
	typedef __declspec(align(MEMORY_ALLOCATION_ALIGNMENT)) QueryBindingCharBuf;
	QueryBindingCharBuf		Buf[1024];

	BOOLEAN				bSuccess;
	DWORD       		BufLength = sizeof(Buf);
	DWORD       		BytesWritten;
	PNDISPROT_QUERY_BINDING 	pQueryBinding;

	pQueryBinding = (PNDISPROT_QUERY_BINDING)Buf;
	pQueryBinding->BindingIndex = bindingIndex;


	bSuccess = (BOOLEAN)DeviceIoControl(
		Handle,
		IOCTL_NDISPROT_QUERY_BINDING,
		pQueryBinding,
		sizeof(NDISPROT_QUERY_BINDING),
		Buf,
		BufLength,
		&BytesWritten,
		NULL);
	if (bSuccess) {
		PRINTF(("%2d. %ws\n     - %ws\n",
			pQueryBinding->BindingIndex,
			(WCHAR *)((PUCHAR)pQueryBinding + pQueryBinding->DeviceNameOffset),
			(WCHAR *)((PUCHAR)pQueryBinding + pQueryBinding->DeviceDescrOffset)));

#pragma warning(suppress:6202) // buffer overrun warning - enough space allocated in QueryBuffer
		memcpy(pDeviceName, ((PUCHAR)pQueryBinding + pQueryBinding->DeviceNameOffset), pQueryBinding->DeviceNameLength);
		*pDeviceNameLength = pQueryBinding->DeviceNameLength;
		memset(Buf, 0, BufLength);

		return bSuccess;
	}
	else {
		ulong   rc = GetLastError();
		if (rc != ERROR_NO_MORE_ITEMS) {
			PRINTF(("\nNo interfaces found! Make sure MetCap driver is installed.\n, error %d\n", rc));
		}
		return bSuccess;
	}
}

BOOL OpenTapDevice() {
	DeviceHandleIn = OpenHandle(pNdisProtDevice);
	if (DeviceHandleIn == INVALID_HANDLE_VALUE) {
		PRINTF(("Failed to open source device %s\n", pNdisProtDevice));
		return FALSE;
	}

	return TRUE;
}

BOOL GetSrcDst(
	_In_ int MaxDeviceIndex,
	_In_ BOOL bIndexDefined) {
	/* Get the first interface number*/
	PRINTF(("\nEnter the number of the source interface to use (0-%d):", MaxDeviceIndex - 1));

	if (!bIndexDefined) {
		scanf_s("%d", &DeviceIndex1);
	}
	else {
		printf("%d\n", DeviceIndex1);
	}

	if (DeviceIndex1 < 0 || DeviceIndex1 > MaxDeviceIndex - 1) {
		PRINTF(("\nSource Interface number out of range.\n"));
		if (DeviceHandleIn != INVALID_HANDLE_VALUE || DeviceHandleOut != INVALID_HANDLE_VALUE) {
			CloseHandle(DeviceHandleIn);
			CloseHandle(DeviceHandleOut);
		}
		return FALSE;
	}

	return TRUE;
}

int wmain(int argc, _TCHAR* argv[]) {
	int			MaxDeviceIndex = 0;
	REFLECT_PARAMS	rpSourceAdapter;
	wchar_t       NdisDeviceNameIn[MAX_NDIS_DEVICE_NAME_LEN];
	//wchar_t       NdisDeviceNameOut[MAX_NDIS_DEVICE_NAME_LEN];
	SIZE_T		DeviceNameLength = 0;
	PSIZE_T		pDeviceNameLength = &DeviceNameLength;
	wchar_t		ConsoleTitle[100] = L"";
	BOOL		bIndexDefined = FALSE;
	BOOL		bDevicesDefined = FALSE;
	wchar_t       kafka_topic[MAX_LEN];
	wchar_t       kafka_config_path[MAX_LEN];
	DeviceHandleIn = INVALID_HANDLE_VALUE;
	DeviceHandleOut = INVALID_HANDLE_VALUE;

	// Handle mirroring.
	if (argc > 3 && _tcsicmp(argv[1], _T("/src")) == 0 ) {
		wcscpy_s(NdisDeviceNameIn, _countof(NdisDeviceNameIn), argv[2]);
		wcscpy_s(kafka_topic, _countof(kafka_topic), argv[3]);
		wcscpy_s(kafka_config_path, _countof(kafka_config_path), argv[4]);
		bDevicesDefined = TRUE;

	}

	if (argc > 1 && _tcsicmp(argv[1], _T("/src")) != 0) {
		DeviceIndex1 = _wtoi(argv[1]);
		wcscpy_s(kafka_topic, _countof(kafka_topic), argv[2]);
		wcscpy_s(kafka_config_path, _countof(kafka_config_path), argv[3]);
		bIndexDefined = TRUE;
	}

	if (!OpenTapDevice()) {
		exit(1);
	}

	PRINTF(("\nAvailable Devices:\n"));
	MaxDeviceIndex = EnumerateDevices(DeviceHandleIn);

	//Get source interface.
	if (!bDevicesDefined) {
		if (!GetSrcDst(MaxDeviceIndex, bIndexDefined)) {
			return -1;
		}

		PRINTF(("\nEnter the Kafka Topic to use. [metron]:"));
		wscanf_s(L"%s", kafka_topic, (unsigned)_countof(kafka_topic));
		PRINTF(("\nEnter the Kafka config path to use. [C:\\MetCap\\code\\metcap\\conf\\localhost.kafka]:"));
		wscanf_s(L"%s", kafka_config_path, (unsigned)_countof(kafka_config_path));
	}

	// Open the source adapter.

	if (!bDevicesDefined) {
		if (!GetDevice(DeviceHandleIn, DeviceIndex1, NdisDeviceNameIn, pDeviceNameLength)) {
			PRINTF(("\nUnable to get the source adapter. Device is not supported by MetCap\n"));
			if (DeviceHandleIn != INVALID_HANDLE_VALUE || DeviceHandleOut != INVALID_HANDLE_VALUE) {
				CloseHandle(DeviceHandleIn);
				CloseHandle(DeviceHandleOut);
			}
			return -1;
		}

		NdisDeviceNameIn[DeviceNameLength] = L'\0';
	}
	else {
		DeviceNameLength = min((wcslen(NdisDeviceNameIn) * sizeof(wchar_t)), _countof(NdisDeviceNameIn));
	}

	debug(("Source Adapter %ws with length: %I64u\n", NdisDeviceNameIn, DeviceNameLength));

	if (!OpenNdisDevice(DeviceHandleIn, NdisDeviceNameIn, DeviceNameLength)) {
		PRINTF(("\nUnable to open the source adapter %ws. Device not supported by MetCap\n", NdisDeviceNameIn));
		if (DeviceHandleIn != INVALID_HANDLE_VALUE || DeviceHandleOut != INVALID_HANDLE_VALUE) {
			CloseHandle(DeviceHandleIn);
			CloseHandle(DeviceHandleOut);
		}
		return -1;
	}

	PRINTF(("\nOpened source interface successfully!\n"));

	if (!GetSrcMac(DeviceHandleIn, SrcMacAddr)) {
		PRINTF(("\nUnable to get the source adapter MAC. Device is not supported by MetCap\n"));
		if (DeviceHandleIn != INVALID_HANDLE_VALUE || DeviceHandleOut != INVALID_HANDLE_VALUE) {
			CloseHandle(DeviceHandleIn);
			CloseHandle(DeviceHandleOut);
		}
		return -1;
	}


	PRINTF(("Got source MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
		SrcMacAddr[0],
		SrcMacAddr[1],
		SrcMacAddr[2],
		SrcMacAddr[3],
		SrcMacAddr[4],
		SrcMacAddr[5]));

	wsprintf(ConsoleTitle, L"Reflecting %02x:%02x:%02x:%02x:%02x:%02x ",
		SrcMacAddr[0],
		SrcMacAddr[1],
		SrcMacAddr[2],
		SrcMacAddr[3],
		SrcMacAddr[4],
		SrcMacAddr[5]);
	SetConsoleTitle(ConsoleTitle);

	// Start the threads that will forward the packets to receiver.

	// Initialize the critical section that will be used by the threads for console output.
	InitializeCriticalSection(&print_cs);


	char kafka_topic_mb[MAX_LEN];
	char kafka_config_path_mb[MAX_LEN];
	size_t NumOfCharConverted;

	wcstombs_s(&NumOfCharConverted, kafka_topic_mb, MAX_LEN, kafka_topic, MAX_LEN - 1);
	wcstombs_s(&NumOfCharConverted, kafka_config_path_mb, MAX_LEN, kafka_config_path, MAX_LEN - 1);

	// Init input parameters of the threads.
	rpSourceAdapter.state = 0;
	rpSourceAdapter.hNdisDeviceIn = DeviceHandleIn;
	rpSourceAdapter.kafka_topic = (char*)malloc(MAX_LEN * sizeof(char));
	rpSourceAdapter.kafka_config_path = (char*)malloc(MAX_LEN * sizeof(char));

	strncpy_s(rpSourceAdapter.kafka_topic, MAX_LEN, kafka_topic_mb, MAX_LEN - 1);
	strncpy_s(rpSourceAdapter.kafka_config_path, MAX_LEN, kafka_config_path_mb, MAX_LEN - 1);

	// Start first thread.
	if ((threads[0] = CreateThread(
		NULL,
		0,
		CaptureAndForwardThread,
		&rpSourceAdapter,
		0,
		NULL)) == NULL) {
		fprintf(stderr, "error creating the first forward thread");

		// Close the adapters.
		PRINTF(("\nError creating the forward thread\n"));
		if (DeviceHandleIn != INVALID_HANDLE_VALUE || DeviceHandleOut != INVALID_HANDLE_VALUE) {
			CloseHandle(DeviceHandleIn);
			CloseHandle(DeviceHandleOut);
		}
		return -1;
	}

	// Install a CTRL+C handler that will do the cleanups on exit
	signal(SIGINT, ctrlc_handler);

	// Done!
	printf("\nStarted reflecting the adapter...\n");
	Sleep(INFINITE);
	return 0;
}

// Forwarding thread.
DWORD WINAPI CaptureAndForwardThread(LPVOID lpParameter) {
	PREFLECT_PARAMS psaReflectArgs = lpParameter;
	ulong n_fwd = 0;

	PUCHAR      pReadBuf = NULL;
	int         ReadCount = 0;
	BOOLEAN     bSuccess = FALSE;
	ulong       BytesRead;

	struct app_params app; 	// Contains all application parameters.

	app.kafka_config_path = psaReflectArgs->kafka_config_path;
	app.kafka_topic = psaReflectArgs->kafka_topic;

	if (kaf_init(1, app) != 0) {
		return 0;
	}

	// Loop receiving packets from the source adapter
	while ((!kill_forwaders)) {
		do {
			ReadCount = 0;
			pReadBuf = malloc(PacketLength);

			if (pReadBuf == NULL) {
				PRINTF(("DoReadProc: failed to alloc %d bytes\n", PacketLength));
				break;
			}

			bSuccess = (BOOLEAN)ReadFile(
				psaReflectArgs->hNdisDeviceIn,
				(LPVOID)pReadBuf,
				PacketLength,
				&BytesRead,
				NULL);

			if (!bSuccess) {
				EnterCriticalSection(&print_cs);
				PRINTF(("DoReadProc: ReadFile failed on Handle %p, error %x\n",
					psaReflectArgs->hNdisDeviceIn, GetLastError()));
				LeaveCriticalSection(&print_cs);
				break;
			}
			else {
				ReadCount++;

#if _DEBUG
				/*
				* Print something, just to show when we have activity.
				* BEWARE: acquiring a critical section and printing strings with printf
				* is something inefficient that you seriously want to avoid in your packet loop!
				* However, since this DEBUG mode, we privilege visual output to efficiency.
				*/
				EnterCriticalSection(&print_cs);

				if (psaReflectArgs->state == 0)
					debug(("Sending pkt - %d bytes\n", BytesRead));
				else
					debug(("Reflecting pkt - %d bytes\n", BytesRead));

				LeaveCriticalSection(&print_cs);
#endif
				bSuccess = (BYTE)kaf_send(pReadBuf, BytesRead, 0); // Send the just received packet to the receiver.

				if (!bSuccess) {
					EnterCriticalSection(&print_cs);
					PRINTF(("DoWriteProc: Failed to send the packet!\n"));
					LeaveCriticalSection(&print_cs);
					break;
				}
				else {
					n_fwd++;
				}

				if (pReadBuf) {
					free(pReadBuf);
				}
			}
		} while (FALSE);
	}

	// We're out of the main loop. Check the reason.

	if (!bSuccess) {
		EnterCriticalSection(&print_cs);

		printf("Error capturing the packets\n");
		fflush(stdout);

		LeaveCriticalSection(&print_cs);
	}
	else {
		EnterCriticalSection(&print_cs);

		printf("End of bridging on interface %u. Forwarded packets:%u\n",
			psaReflectArgs->state,
			n_fwd);
		fflush(stdout);

		LeaveCriticalSection(&print_cs);
	}

	return 0;
}

void ctrlc_handler(int sig) {

	(VOID)(sig); // unused.

	kill_forwaders = 1;

	WaitForMultipleObjects(2,
		threads,
		TRUE,        // Wait for all the handles
		5000);       // Timeout

	exit(0);
}