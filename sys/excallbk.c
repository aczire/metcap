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

/*
Abstract: The routines in this module helps to solve driver load order
dependency between this sample and NDISWDM sample. These
routines are not required in a typical protocol driver. By default
this module is not included in the sample. You include these routines
by adding EX_CALLBACK defines to the 'sources' file. Read the
NDISWDM samples readme file for more information on how ExCallback
kernel interfaces are used to solve driver load order issue.
*/

#include "precomp.h"

#ifdef EX_CALLBACK

#define __FILENUMBER 'LCxE'

#define NDISPROT_CALLBACK_NAME  L"\\Callback\\NdisProtCallbackObject"

#define CALLBACK_SOURCE_NDISPROT    0
#define CALLBACK_SOURCE_NDISWDM     1

PCALLBACK_OBJECT                CallbackObject = NULL;
PVOID                           CallbackRegisterationHandle = NULL;

typedef VOID(*NOTIFY_PRESENCE_CALLBACK)(OUT PVOID Source);

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, ndisprotRegisterExCallBack)
#pragma alloc_text(PAGE, ndisprotUnregisterExCallBack)

#endif // ALLOC_PRAGMA

BOOLEAN
ndisprotRegisterExCallBack() {
	OBJECT_ATTRIBUTES   ObjectAttr;
	UNICODE_STRING      CallBackObjectName;
	NTSTATUS            Status;
	BOOLEAN             bResult = TRUE;

	DEBUGP(DL_LOUD, ("--> ndisprotRegisterExCallBack\n"));

	PAGED_CODE();

	do {

		RtlInitUnicodeString(&CallBackObjectName, NDISPROT_CALLBACK_NAME);

		InitializeObjectAttributes(&ObjectAttr,
			&CallBackObjectName,
			OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
			NULL,
			NULL);

		Status = ExCreateCallback(&CallbackObject,
			&ObjectAttr,
			TRUE,
			TRUE);


		if (!NT_SUCCESS(Status)) {

			DEBUGP(DL_ERROR, ("RegisterExCallBack: failed to create callback %lx\n", Status));
			bResult = FALSE;
			break;
		}

		CallbackRegisterationHandle = ExRegisterCallback(CallbackObject,
			ndisprotCallback,
			(PVOID)NULL);
		if (CallbackRegisterationHandle == NULL) {
			DEBUGP(DL_ERROR, ("RegisterExCallBack: failed to register a Callback routine%lx\n", Status));
			bResult = FALSE;
			break;
		}

		ExNotifyCallback(CallbackObject,
			(PVOID)CALLBACK_SOURCE_NDISPROT,
			(PVOID)NULL);


	} while (FALSE);

	if (!bResult) {
		if (CallbackRegisterationHandle) {
			ExUnregisterCallback(CallbackRegisterationHandle);
			CallbackRegisterationHandle = NULL;
		}

		if (CallbackObject) {
			ObDereferenceObject(CallbackObject);
			CallbackObject = NULL;
		}
	}

	DEBUGP(DL_LOUD, ("<-- ndisprotRegisterExCallBack\n"));

	return bResult;

}

VOID
ndisprotUnregisterExCallBack() {
	DEBUGP(DL_LOUD, ("--> ndisprotUnregisterExCallBack\n"));

	PAGED_CODE();

	if (CallbackRegisterationHandle) {
		ExUnregisterCallback(CallbackRegisterationHandle);
		CallbackRegisterationHandle = NULL;
	}

	if (CallbackObject) {
		ObDereferenceObject(CallbackObject);
		CallbackObject = NULL;
	}

	DEBUGP(DL_LOUD, ("<-- ndisprotUnregisterExCallBack\n"));

}

VOID
ndisprotCallback(
	PVOID   CallBackContext,
	PVOID   Source,
	PVOID   CallbackAddr
) {
	NOTIFY_PRESENCE_CALLBACK func;

	DEBUGP(DL_LOUD, ("==>ndisprotoCallback: Source %lx, CallbackAddr %p\n",
		Source, CallbackAddr));

	//
	// if we are the one issuing this notification, just return
	//
	if (Source == CALLBACK_SOURCE_NDISPROT) {
		return;
	}

	//
	// Notification is coming from NDISWDM
	// let it know that you are here
	//
	ASSERT(Source == (PVOID)CALLBACK_SOURCE_NDISWDM);

	if (Source == (PVOID)CALLBACK_SOURCE_NDISWDM) {

		ASSERT(CallbackAddr);

		if (CallbackAddr == NULL) {
			DEBUGP(DL_ERROR, ("Callback called with invalid address %p\n", CallbackAddr));
			return;
		}

		func = CallbackAddr;

		func(CALLBACK_SOURCE_NDISPROT);
	}

	DEBUGP(DL_LOUD, ("<==ndisprotoCallback: Source,  %lx\n", Source));

}

#endif
