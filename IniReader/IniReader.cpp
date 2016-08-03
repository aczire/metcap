// IniReader.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define WIN32_LEAN_AND_MEAN
// Exclude rarely-used stuff from Windows headers
#include <windows.h>

#include <iostream>
#include <string>
#include <vector>

static inline LPCTSTR NextToken(LPCTSTR pArg)
{
	// find next null with strchr and
	// point to the char beyond that.
	return wcschr(pArg, '\0') + 1;
}

void loadIniFile(const LPCTSTR iniFilePath, const LPCTSTR sectionName) {
	CONST INT bufferSize = 10000;
	WCHAR buffer[bufferSize];

	int charsRead = 0;

	charsRead = GetPrivateProfileSection(sectionName,
		buffer,
		bufferSize,
		iniFilePath);
	// if there isn't enough space, returns bufferSize - 2

	// if we got some data...
	if ((0 < charsRead) && ((bufferSize - 2) > charsRead)) {
		// walk the buffer extracting values

		// start at the beginning (const to remind us not to
		// change the contents of the buffer)
		CONST LPCTSTR pSubstr = buffer;
		for (LPCTSTR pToken = pSubstr; pToken && *pToken; pToken = NextToken(pToken)) {
			// while we have non-empty substrings...
			if ('\0' != *pToken && '#' != *pToken) {
				// length of key-value pair substring
				size_t substrLen = wcslen(pToken);
				// split substring on '=' char
				LPCTSTR pos = wcschr(pToken, '=');
				if (NULL != pos) {
					// todo: remove "magic number" for buffer size 
					WCHAR name[255];
					WCHAR value[255];
					// if you're not using VC++ you'll prob. need to replace
					// _countof(name) with sizeof(name)/sizeof(char) and
					// similarly for value. Plus replace strncpy_s with plain
					// old strncpy.
					wcsncpy_s(name, _countof(name), pToken, pos - pToken);
					wcsncpy_s(value, _countof(value), pos + 1, substrLen - (pos - pToken));
					_putws(name);
					_putws(value);
				}
			}
		}
	}
}

int _tmain(int argc, _TCHAR* argv[]){
if (3 != argc) {
	std::cerr << "ini section dump test\n"
		"\n"
		"test.exe ini-file-path section-name\n"
		"\n"
		"Notes\n"
		"- full path to ini-file must be specified, or Windows will\n"
		"  assume the file is in the Windows folder (typically C:\\Windows)\n"
		"- paths and section names containing spaces must be quoted" << std::endl;
}
 {
	//LPCTSTR iniFilePath = argv[1];
	//LPCTSTR sectionName = argv[2];

	LPCTSTR iniFilePath = L"C:\\conf\\localhost.kafka";
	LPCTSTR sectionName = L"kafka-global";

	loadIniFile(iniFilePath, sectionName);
	char t[255];
	gets_s(t,1);

	sectionName = L"kafka-topic";

	loadIniFile(iniFilePath, sectionName);
	gets_s(t, 1);
}

return 0;
}

