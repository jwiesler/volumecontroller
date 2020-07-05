#ifndef PROCESSDATA_H
#define PROCESSDATA_H
#include <Windows.h>

#include <optional>
#include <QString>
#include <QPixmap>

namespace ProcessData {
	HWND FindMainWindow(DWORD process_id);

	QString GetWindowTitle(HWND window);

	std::optional<QString> GetDisplayName(DWORD pid);

	std::optional<QPixmap> GetImageFromFile(LPCWSTR path, int offset, int cx, int cy);

	std::optional<QPixmap> GetProcessImage(DWORD pid, int cx, int cy);
};

#endif // PROCESSDATA_H
