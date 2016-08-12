/**
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef _PROTINSTALL_H_
#define _PROTINSTALL_H_

////////////////////////////////////////////////////////////////////////////
//// Device Naming String Definitions
//

//
// "Friendly" Name
//
#define NDISPROT_FRIENDLY_NAME_A          "MetCap Protocol Driver"
#define NDISPROT_FRIENDLY_NAME_W          L"MetCap Protocol Driver"

#ifdef UNICODE
#define NDISPROT_FRIENDLY_NAME            NDISPROT_FRIENDLY_NAME_W
#else
#define NDISPROT_FRIENDLY_NAME            NDISPROT_FRIENDLY_NAME_A
#endif

//
// Protocol Name
// -------------
// This is the name of the protocol, and is a parameter passed to
// NdisRegisterProtocol().
//
#define NDISPROT_PROTOCOL_NAME_W  L"METCAP"
#define NDISPROT_PROTOCOL_NAME_A  "METCAP"

#ifdef UNICODE
#define NDISPROT_PROTOCOL_NAME NDISPROT_PROTOCOL_NAME_W
#else
#define NDISPROT_PROTOCOL_NAME NDISPROT_PROTOCOL_NAME_A
#endif

//
// Driver WDM Device Object Name
// -----------------------------
// This is the name of the NDISPROT driver WDM device object.
//
#define NDISPROT_WDM_DEVICE_NAME_W  L"\\Device\\METCAP"
#define NDISPROT_WDM_DEVICE_NAME_A  "\\Device\\METCAP"

#ifdef UNICODE
#define NDISPROT_WDM_DEVICE_NAME NDISPROT_WDM_DEVICE_NAME_W
#else
#define NDISPROT_WDM_DEVICE_NAME NDISPROT_WDM_DEVICE_NAME_A
#endif

//
// Driver Device WDM Symbolic Link
// -------------------------------
// This is the name of the NDISPROT driver device WDM symbolic link. This
// is a user-visible name that can be used by Win32 applications to access
// the NDISPROT driver WDM interface.
//
#define NDISPROT_WDM_SYMBOLIC_LINK_W  L"\\DosDevices\\METCAP"
#define NDISPROT_WDM_SYMBOLIC_LINK_A  "\\DosDevices\\METCAP"

#ifdef UNICODE
#define NDISPROT_WDM_SYMBOLIC_LINK  NDISPROT_WDM_SYMBOLIC_LINK_W
#else
#define NDISPROT_WDM_SYMBOLIC_LINK  NDISPROT_WDM_SYMBOLIC_LINK_A
#endif

//
// Driver WDM Device Filename
// --------------------------
// This is the name that Win32 applications pass to CreateFile to open
// the TPA-REDIR symbolic link.
//
#define NDISPROT_WDM_DEVICE_FILENAME_W  L"\\\\.\\METCAP"
#define NDISPROT_WDM_DEVICE_FILENAME_A  "\\\\.\\METCAP"

#ifdef UNICODE
#define NDISPROT_WDM_DEVICE_FILENAME NDISPROT_WDM_DEVICE_FILENAME_W
#else
#define NDISPROT_WDM_DEVICE_FILENAME NDISPROT_WDM_DEVICE_FILENAME_A
#endif

//
// Driver INF File and PnP ID Names
//
#define NDISPROT_SERVICE_PNP_DEVICE_ID_A      "METCAP_TAPROTO"
#define NDISPROT_SERVICE_PNP_DEVICE_ID_W      L"METCAP_TAPROTO"

#define NDISPROT_SERVICE_INF_FILE_A           "METCAPDRV"
#define NDISPROT_SERVICE_INF_FILE_W           L"METCAPDRV"

#ifdef UNICODE
#define NDISPROT_SERVICE_PNP_DEVICE_ID        NDISPROT_SERVICE_PNP_DEVICE_ID_W
#define NDISPROT_SERVICE_INF_FILE             NDISPROT_SERVICE_INF_FILE_W
#else
#define NDISPROT_SERVICE_PNP_DEVICE_ID        NDISPROT_SERVICE_PNP_DEVICE_ID_A
#define NDISPROT_SERVICE_INF_FILE             NDISPROT_SERVICE_INF_FILE_A
#endif

/////////////////////////////////////////////////////////////////////////////
//// Registry Path Strings
//

#define NDISPROT_REGSTR_PATH_PARAMETERS_W   L"METCAP\\Parameters"
#define NDISPROT_REGSTR_PATH_PARAMETERS_A   "METCAP\\Parameters"

#ifdef UNICODE
#define NDISPROT_REGSTR_PATH_PARAMETERS     NDISPROT_REGSTR_PATH_PARAMETERS_W
#else
#define NDISPROT_REGSTR_PATH_PARAMETERS     NDISPROT_REGSTR_PATH_PARAMETERS_A
#endif


/////////////////////////////////////////////////////////////////////////////
//// Registry Key Strings
//

#define NDISPROT_REGSTR_KEY_PARAMETERS_W   L"Parameters"
#define NDISPROT_REGSTR_KEY_PARAMETERS_A   "Parameters"

#ifdef UNICODE
#define NDISPROT_REGSTR_KEY_PARAMETERS     NDISPROT_REGSTR_KEY_PARAMETERS_W
#else
#define NDISPROT_REGSTR_KEY_PARAMETERS     NDISPROT_REGSTR_KEY_PARAMETERS_A
#endif

#endif // _PROTINSTALL_H_
