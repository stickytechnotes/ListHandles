// ListHandles.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Utils.h"
#include "atlconv.h"

#define BUFSIZE 512

typedef DWORD (NTAPI *pNtQuerySystemInformation)(DWORD info_class, void *out, DWORD size, DWORD *out_size);

SYSTEM_HANDLE_INFORMATION buf[100000];


int _tmain(int argc, _TCHAR* argv[])
{
	HMODULE hModule = GetModuleHandle(_T("ntdll.dll"));		

    PNtQuerySystemInformation NtQuerySystemInformation = (PNtQuerySystemInformation)GetProcAddress(hModule, "NtQuerySystemInformation");
    PNtDuplicateObject NtDuplicateObject = (PNtDuplicateObject)GetProcAddress(hModule, "NtDuplicateObject");
    PNtQueryObject NtQueryObject = (PNtQueryObject)GetProcAddress(hModule, "NtQueryObject");

    NTSTATUS status;
    PSYSTEM_HANDLE_INFORMATION handleInfo;
    ULONG handleInfoSize = 0x10000;
    ULONG pid;
    HANDLE processHandle;
	HANDLE processInfo;
    ULONG i;

	LONG begin = GetTickCount();

    if (argc < 2)
    {
        printf("Usage: handles [pid]\n");
        return 1;
    }

    pid = _wtoi(argv[1]);

     DWORD type_char = 0, 
      type_disk = 0, 
      type_pipe = 0, 
      type_remote = 0, 
      type_unknown = 0,
      handles_count = 0;

 
	//===============================================================================
	// Obtain handles list from number to address (is simple 0x4 * i)
    //===============================================================================
	if (!(processInfo = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid)))
    {
        printf("Could not open PID %d! (Don't try to open a system process.)\n", pid);
        return 1;
    }
	//===============================================================================
	// Duplicate handle
    //===============================================================================
	if (!(processHandle = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pid)))
    {
        printf("Could not open PID %d! (Don't try to open a system process.)\n", pid);
        return 1;
    }

	if(GetProcessHandleCount(processInfo, &handles_count))
	{
		handles_count *= 4;
		for (DWORD handle = 0x4; handle < handles_count; handle += 4) 
		{
			LONG beginHandle = GetTickCount();

			HANDLE dupHandle;
			POBJECT_TYPE_INFORMATION objectTypeInfo = NULL;
			PVOID objectNameInfo = NULL;
			UNICODE_STRING objectName;
			ULONG returnLength;

			if(DuplicateHandle(processHandle, (HANDLE)handle,GetCurrentProcess(),&dupHandle, 0,FALSE,DUPLICATE_SAME_ACCESS))
			{

				// Query the object type. 
				objectTypeInfo = (POBJECT_TYPE_INFORMATION)malloc(0x1000);
				if (!NT_SUCCESS(NtQueryObject( dupHandle,ObjectTypeInformation,objectTypeInfo,0x1000,NULL)))
				{
					printf("[%#x] Error!\n", handle);
					CloseHandle(dupHandle);
					continue;
				}

				// Only choose file type.
				USES_CONVERSION;
				if(strcmp(W2A(objectTypeInfo->Name.Buffer),"File")==0)
				{
					objectNameInfo = malloc(0x1000);
					if (!NT_SUCCESS(NtQueryObject( dupHandle,ObjectNameInformation,objectNameInfo,0x1000,&returnLength)))
					{
					   // Reallocate the buffer and try again. 
						objectNameInfo = realloc(objectNameInfo, returnLength);
						if (!NT_SUCCESS(NtQueryObject(dupHandle,ObjectNameInformation,objectNameInfo,returnLength,NULL)))
						{
							 // We have the type name, so just display that. 
							printf("[%#x] %.*S: (could not get name)\n",handle,objectTypeInfo->Name.Length / 2,objectTypeInfo->Name.Buffer);
							free(objectTypeInfo);
							free(objectNameInfo);
							CloseHandle(dupHandle);
							continue;
						}
					}

					// Cast our buffer into an UNICODE_STRING. 
					objectName = *(PUNICODE_STRING)objectNameInfo;

					// Print the information! 
					if (objectName.Length)
					{
						
						LONG endHandle = GetTickCount();
						printf(" %s \n",W2A(objectName.Buffer));
						
					}
				}
				if(objectTypeInfo) {free(objectTypeInfo);}
				if(objectNameInfo) {free(objectNameInfo);}
				CloseHandle(dupHandle);
			}
		}
	}

	CloseHandle(processInfo);
	CloseHandle(processHandle);

	LONG end = GetTickCount();

	printf("Total Time [%ld]",(LONG)(end-begin));
    return 0;
}

