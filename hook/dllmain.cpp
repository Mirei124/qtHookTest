#include "pch.h"
#include <Windows.h>
#include <detours.h>
#pragma comment(lib, "detours.lib")
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QDebug>
#pragma comment(lib, "Qt5Core.lib")
#include <QtWidgets/QFileDialog>
#pragma comment(lib, "Qt5Widgets.lib")
#include <iostream>
#include <filesystem>
#include <fstream>

#define TARGET_DLL L"Qt5Widgets.dll"

std::string refImgPath{};
std::string refConfPath{};
std::string allImgPath{};
int count = 0;

using Type_getOpenFileName = QString(*)(QWidget*, QString const&, QString const&, QString const&, QString*, QFlags<enum QFileDialog::Option>);
using Type_getOpenFileNameList = QStringList(*)(QWidget*, QString const&, QString const&, QString const&, QString*, QFlags<enum QFileDialog::Option>);

Type_getOpenFileName true_getOpenFileName = nullptr;
Type_getOpenFileNameList true_getOpenFileNameList = nullptr;


__declspec(dllexport) QString my_getOpenFileName(
	QWidget* parent,
	const  QString& caption,
	const  QString& dir,
	const  QString& filter,
	QString* selectedFilter,
	QFileDialog::Options options
) {
	if (count == 0) {
		count++;
		return QString::fromUtf8(refImgPath.c_str());
	}
	else {
		return QString::fromUtf8(refConfPath.c_str());
	}
}

__declspec(dllexport) QStringList my_getOpenFileNameList(
	QWidget* parent,
	const QString& caption,
	const QString& dir,
	const QString& filter,
	QString* selectedFilter,
	QFileDialog::Options options
) {
	const std::filesystem::path allImgDir(allImgPath.c_str());
	QStringList imgList;
	for (auto const& img : std::filesystem::directory_iterator(allImgDir)) {
		std::cout << img.path().string() << std::endl;
		imgList.append(QString::fromUtf8(img.path().u8string().c_str()));
	}
	return imgList;
}

void hook_funcs() {
	CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)[](LPVOID lpThreadParam)->DWORD{
		HMODULE hDll = GetModuleHandleW(TARGET_DLL);
		while (hDll == nullptr) {
			std::cout << "Dll not load, waitting..." << std::endl;
			Sleep(100);
			hDll = GetModuleHandleW(TARGET_DLL);
		}

		true_getOpenFileName = &QFileDialog::getOpenFileName;
		std::cout << "getOpenFileName: " << true_getOpenFileName << std::endl;
		if (true_getOpenFileName == nullptr) {
			qDebug() << "Get getOpenFileName address failed. Exit.";
			return FALSE;
		}
		true_getOpenFileNameList = &QFileDialog::getOpenFileNames;
		std::cout << "getOpenFileNames: " << true_getOpenFileNameList << std::endl;
		if (true_getOpenFileNameList == nullptr) {
			qDebug() << "Get getOpenFileNames address failed. Exit.";
			return FALSE;
		}

		DetourRestoreAfterWith();

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		LONG error = DetourAttach(&(PVOID&)true_getOpenFileName, my_getOpenFileName);
		if (error != NO_ERROR) {
			qDebug() << "Hook getOpenFileName failed, error: " << error;
		}
		else {
			qDebug() << "getOpenFileName hooked.";
		}

		error = DetourAttach(&(PVOID&)true_getOpenFileNameList, my_getOpenFileNameList);
		if (error != NO_ERROR) {
			qDebug() << "Hook getOpenFileNames failed, error: " << error;
		}
		else {
			qDebug() << "getOpenFileNameList hooked.";
		}

		DetourTransactionCommit();
		std::cout << "Hook finished" << std::endl;
		return TRUE;
	}, NULL, 0, NULL);


}

void unhook_funcs() {
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourDetach(&(PVOID&)true_getOpenFileName, my_getOpenFileName);
	DetourDetach(&(PVOID&)true_getOpenFileNameList, my_getOpenFileNameList);

	DetourTransactionCommit();
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
	if (DetourIsHelperProcess()) {
		return TRUE;
	}

	if (dwReason == DLL_PROCESS_ATTACH) {
		setlocale(LC_ALL, ".utf-8");
		std::ifstream config("hook.conf");
		if (!config) {
			std::cout << "Read hook.conf failed." << std::endl;
			return FALSE;
		}
		std::getline(config, refImgPath, '\n');
		std::getline(config, refConfPath, '\n');
		std::getline(config, allImgPath, '\n');
		config.close();
		if (!std::filesystem::exists(refImgPath)) {
			std::cout << refImgPath << " not exists." << std::endl;
			return FALSE;
		}
		if (!std::filesystem::exists(refConfPath)) {
			std::cout << refConfPath << " not exists." << std::endl;
			return FALSE;
		}
		if (!std::filesystem::exists(allImgPath)) {
			std::cout << allImgPath << " not exists." << std::endl;
			return FALSE;
		}
		hook_funcs();
	}
	else if (dwReason == DLL_PROCESS_DETACH) {
		unhook_funcs();
	}
	return TRUE;
}