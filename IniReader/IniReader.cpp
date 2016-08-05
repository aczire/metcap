// IniReader.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define WIN32_LEAN_AND_MEAN
// Exclude rarely-used stuff from Windows headers
#include <windows.h>

#include <iostream>
#include <string>
#include <vector>

#define MAX_LEN 255

static inline char* NextToken(char* pArg)
{
	// find next null with strchr and
	// point to the char beyond that.
	return strchr(pArg, '\0') + 1;
}

void loadIniFile(const char* iniFilePath, const char* sectionName) {
	const int bufferSize = 10000;
	char buffer[bufferSize];

	int charsRead = 0;

	charsRead = GetPrivateProfileSectionA(sectionName,
		buffer,
		bufferSize,
		iniFilePath);

	if ((0 < charsRead) && ((bufferSize - 2) > charsRead)) {
		char* pSubstr = buffer;
		for (char* pToken = pSubstr; pToken && *pToken; pToken = NextToken(pToken)) {
			if ('\0' != *pToken && '#' != *pToken) {
				size_t substrLen = strlen(pToken);
				char* pos = strchr(pToken, '=');
				if (NULL != pos) {
					char name[MAX_LEN];
					char value[MAX_LEN];

					strncpy_s(name, _countof(name), pToken, pos - pToken);
					strncpy_s(value, _countof(value), pos + 1, substrLen - (pos - pToken));
					std::cout << name << " = " << value << "\n";
				}
			}
		}
	}
}

int main(int argc, char* argv[]) {
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
		//char* iniFilePath = argv[1];
		//char* sectionName = argv[2];

		char* iniFilePath = "C:\\conf\\localhost.kafka";
		char* sectionName = "kafka-global";

		loadIniFile(iniFilePath, sectionName);
		char t[255];
		gets_s(t, 1);

		sectionName = "kafka-topic";

		loadIniFile(iniFilePath, sectionName);
		gets_s(t, 1);
	}

	return 0;
}

