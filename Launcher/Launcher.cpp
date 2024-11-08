#include <Windows.h>
#include <detours.h>
#include <tchar.h>
#include <iostream>
#include <string>

int wmain(int argc, TCHAR* argv[]) {
	STARTUPINFO si{ 0 };
	PROCESS_INFORMATION pi{ 0 };
	si.cb = sizeof(si);

	if (argc < 3) {
		std::cout << "usage: " << __FILE__ << " exePath dllPath [workDir]" << std::endl;
		return 0;
	}
	TCHAR* cmdline = argv[1];
	CHAR buf[256];
	sprintf_s(buf, "%ws", argv[2]);
	CHAR* dllPath = buf;
	TCHAR* workDir = nullptr;
	if (argc >= 4) {
		workDir = argv[3];
	}

	//TCHAR cmdline[] = L"test.exe";
	//TCHAR workDir[] = L"D:\\wkdir";
	//CHAR dllPath[] = "D:\\hook64.dll";

	BOOL bSuccess = DetourCreateProcessWithDllW(
		nullptr,
		cmdline,
		nullptr,
		nullptr,
		FALSE,
		CREATE_DEFAULT_ERROR_MODE,
		nullptr,
		nullptr, // lpCurrentDirectory
		&si,
		&pi,
		dllPath,
		nullptr
	);

	if (!bSuccess) {
		DWORD error = GetLastError();
		std::cout << "Failed to create process. Error: " << error << std::endl;
		std::string buf;
		std::getline(std::cin, buf);
		return 1;
	}
	else {
		std::cout << "Hook success, waiting..." << std::endl;
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	return 0;
}
