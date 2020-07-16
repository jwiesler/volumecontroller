#ifndef VOLUMECONTROLLIST_H
#define VOLUMECONTROLLIST_H

#include <QGridLayout>
#include <QWidget>

#include "volumecontroller/audio/audiosessions.h"
#include "volumecontroller/ui/volumelistitem.h"
#include "volumecontroller/ui/gridlayout.h"
#include "volumecontroller/ui/theme.h"

using SessionVolumeItemPtr = std::unique_ptr<SessionVolumeItem>;

class VolumeControlList : public QWidget
{
	Q_OBJECT

public:
	VolumeControlList(QWidget *parent, AudioSessionGroups &sessionGroups, const VolumeItemTheme &item);

	void updatePeaks();

	void addSession(std::unique_ptr<AudioSession> &&ptr);

	static void addItem(QGridLayout &layout, VolumeItemBase &item, int row);
	static void addItem(GridLayout &layout, VolumeItemBase &item, int row);

	bool showInactive() const { return _showInactive; }

	void setShowInactive(bool value);

private:
	std::unique_ptr<SessionVolumeItem> createItem(AudioSession &sessionControl, const AudioSessionPidGroup &group);
	void createItems();

	void addNewItem(std::unique_ptr<SessionVolumeItem> &&item);
	void insertActiveItem(std::unique_ptr<SessionVolumeItem> &&item);
	std::unique_ptr<SessionVolumeItem> removeActiveItem(std::vector<std::unique_ptr<SessionVolumeItem>>::iterator it);

//	void extractRow(int row, SessionVolumeItem &target);
//	void insertRow(int row, SessionVolumeItem &source);
//	void fillGap(int row);

	void sortItems();

	void onSessionActive(SessionVolumeItem &sessionVolume);
	void onSessionInactive(SessionVolumeItem &sessionVolume);
	void onSessionExpire(SessionVolumeItem &sessionVolume);

	GridLayout layout;
	AudioSessionGroups &sessionGroups;
	std::vector<SessionVolumeItemPtr> volumeItems;
	std::vector<SessionVolumeItemPtr> volumeItemsInactive;

	bool _showInactive = false;
	const VolumeItemTheme &itemTheme;
};

#endif // VOLUMECONTROLLIST_H
