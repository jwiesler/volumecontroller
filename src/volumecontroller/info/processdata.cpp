#include "processdata.h"
#include <Shobjidl.h>
#include <Shlobj.h>
#include <shellapi.h>
#include <Knownfolders.h>
#include <QtWinExtras/QtWinExtras>
#include "winver.h"

#include "volumecontroller/hresulterrors.h"
#include "volumecontroller/comptr.h"

namespace ProcessData {

std::optional<QPixmap> GetIcon(PCWSTR path, bool isDesktopApp, int cx, int cy) {
	IShellItem2 *item;
	const auto err = isDesktopApp ? SHCreateItemFromParsingName(path, nullptr, __uuidof(IShellItem2), (void**)&item)
										 : SHCreateItemInKnownFolder(FOLDERID_AppsFolder, KF_FLAG_DONT_VERIFY, path, __uuidof(IShellItem2), (void**)&item);
	RET_EMPTY(err);

	IShellItemImageFactory *factory;
	RET_EMPTY(item->QueryInterface(__uuidof(IShellItemImageFactory), (void**)&factory));
	HBITMAP bitmap;
	factory->GetImage(SIZE{cx, cy}, SIIGBF_RESIZETOFIT, &bitmap);

	QPixmap map = QtWin::fromHBITMAP(bitmap, QtWin::HBitmapAlpha);
	DeleteObject(bitmap);
	return map;
}

struct handle_data {
	 unsigned long process_id;
	 HWND window_handle;
};

BOOL IsMainWindow(HWND handle)
{
	 return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
}

BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
{
	 handle_data& data = *(handle_data*)lParam;
	 unsigned long process_id = 0;
	 GetWindowThreadProcessId(handle, &process_id);
	 if (data.process_id != process_id || !IsMainWindow(handle))
		  return TRUE;
	 data.window_handle = handle;
	 return FALSE;
}

HWND FindMainWindow(DWORD process_id)
{
	 handle_data data;
	 data.process_id = process_id;
	 data.window_handle = 0;
	 EnumWindows(EnumWindowsCallback, (LPARAM)&data);
	 return data.window_handle;
}

QString GetWindowTitle(HWND window) {
	wchar_t buffer[260];
	int length = GetWindowTextW(window, buffer, 260);
	return QString::fromWCharArray(buffer, length);
}

struct HandleRelease {
	void operator()(HANDLE ptr) const {
		CloseHandle(ptr);
	}
};

using UniqueHandle = std::unique_ptr<std::remove_pointer_t<HANDLE>, HandleRelease>;

struct ModuleRelease {
	void operator()(HMODULE ptr) const {
		FreeLibrary(ptr);
	}
};

using UniqueModule = std::unique_ptr<std::remove_pointer_t<HMODULE>, ModuleRelease>;

std::optional<QPixmap> GetImageFromFile(LPCWSTR path, int offset, int cx, int cy) {
	UniqueModule handle(LoadLibraryW(path));
	if(handle.get() == nullptr)
		return {};
	auto groupResInfo = FindResource(handle.get(), MAKEINTRESOURCE(offset), RT_GROUP_ICON);
	if(groupResInfo == nullptr)
		return {};
	auto groupResData = (PBYTE)LockResource(LoadResource(handle.get(), groupResInfo));
	if(groupResData == nullptr)
		return {};
	auto iconId = LookupIconIdFromDirectoryEx(groupResData, true, cx, cy, LR_DEFAULTCOLOR);
	if(iconId == 0)
		return {};

	auto iconResInfo = FindResource(handle.get(), MAKEINTRESOURCE(iconId), RT_ICON);
	auto iconResData = (PBYTE)LockResource(LoadResource(handle.get(), iconResInfo));
	auto iconResSize = SizeofResource(handle.get(), iconResInfo);
	auto iconHandle = CreateIconFromResourceEx(iconResData, iconResSize, true, 0x00030000, cx, cy, LR_DEFAULTCOLOR);
	return QtWin::fromHICON(iconHandle);
}

bool IsValid(HANDLE handle) {
	return handle != INVALID_HANDLE_VALUE && handle != nullptr;
}

std::optional<QPixmap> GetProcessImage(DWORD pid, int cx, int cy) {
	if(pid == 0) {
		qDebug() << "Getting image for sytem sounds";
		constexpr auto imagePath = L"%windir%\\system32\\audiosrv.dll";
		constexpr int offset = 203;
		constexpr DWORD bufferSize = 260;
		WCHAR buffer[bufferSize];
		auto size = ExpandEnvironmentStringsW(imagePath, buffer, bufferSize);
		if(size != 0 && size <= bufferSize)
			return GetImageFromFile(buffer, offset, cx, cy);
	} else {
		qDebug() << "Getting image for process" << pid;
		UniqueHandle handle = UniqueHandle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid));
		if(IsValid(handle.get())) {
			wchar_t buffer[260];
			DWORD size = 260;
			if(!FAILED(QueryFullProcessImageNameW(handle.get(), 0, buffer, &size))) {
				return GetIcon(buffer, true, cx, cy);
			}
		}
	}

	qDebug() << "Failed to get image for process" << pid;
	return {};
}

std::optional<QString> GetDisplayName(const DWORD pid)
{
	UniqueHandle handle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
	if(!IsValid(handle.get()))
		return {};
	WCHAR path[MAX_PATH];
	DWORD size = MAX_PATH;
	if(!QueryFullProcessImageNameW(handle.get(), 0, path, &size))
		return {};

	const DWORD versionInfoSize = GetFileVersionInfoSizeW(path, nullptr);
	std::vector<WCHAR> versionInfo(versionInfoSize);
	if(!GetFileVersionInfoW(path, 0, versionInfoSize, versionInfo.data()))
		return {};

	UINT translateLength;
	struct LANGANDCODEPAGE {
	  WORD Language;
	  WORD CodePage;
	} *pTranslate;
	if(!VerQueryValueW(versionInfo.data(), L"\\VarFileInfo\\Translation", (void**)&pTranslate, &translateLength))
		return {};

	// Read the file description for each language and code page.
	constexpr size_t bufferSize = 256;
	WCHAR buffer[bufferSize];

	if(translateLength < sizeof(struct LANGANDCODEPAGE))
		return {};

	if(swprintf_s(buffer, bufferSize, L"\\StringFileInfo\\%04x%04x\\FileDescription",
				  pTranslate->Language,
				  pTranslate->CodePage) <= 0)
		return {};

	// Retrieve file description for language and code page "i".
	void *out;
	UINT sizeOut;
	if(!VerQueryValueW(versionInfo.data(),
					  buffer,
					  &out,
					  &sizeOut))
		return {};
	LPCWSTR str = (LPCWSTR)out;
	QString qstr = QString::fromWCharArray(str);

	return std::move(qstr);
}

}
