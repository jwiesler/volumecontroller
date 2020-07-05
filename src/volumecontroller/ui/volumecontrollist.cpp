#include "volumecontrollist.h"
#include "volumecontroller/info/processdata.h"
#include <QDebug>

static std::vector<std::unique_ptr<SessionVolumeItem>>::iterator FindItem(std::vector<std::unique_ptr<SessionVolumeItem>> &items, const SessionVolumeItem &sessionVolume) {
	return std::find_if(items.begin(), items.end(), [&](const std::unique_ptr<SessionVolumeItem> &item) {
		return &sessionVolume == item.get();
	});
}

VolumeControlList::VolumeControlList(QWidget *parent, AudioSessionGroups &sessionGroups)
	: QWidget(parent),
	  layout(this),
	  sessionGroups(sessionGroups)
{
	QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
	sizePolicy1.setHorizontalStretch(0);
	sizePolicy1.setVerticalStretch(0);
	sizePolicy1.setHeightForWidth(sizePolicy().hasHeightForWidth());
	setSizePolicy(sizePolicy1);

//	setAutoFillBackground(true);
//	auto p = palette();
//	p.setColor(QPalette::Window, QColor(Qt::yellow));
//	setPalette(p);

	layout.setSpacing(12);
	layout.setSizeConstraint(QLayout::SetDefaultConstraint);
	layout.setAlignment(Qt::AlignTop);
	layout.setContentsMargins(0, 0, 0, 0);

	createItems();
}

void VolumeControlList::updatePeaks() {
	std::for_each(volumeItems.begin(), volumeItems.end(), [](std::unique_ptr<SessionVolumeItem> &item) {
		item->updatePeak();
	});
}

void VolumeControlList::addSession(std::unique_ptr<AudioSession> &&sessionPtr) {
	auto &session = *sessionPtr;
	DWORD pid;
	if(FAILED(session.control().GetProcessId(&pid)))
		return;

	auto cx = 32 * logicalDpiX() / 96.0;
	auto cy = 32 * logicalDpiY() / 96.0;

	auto &pidGroup = sessionGroups.findPidGroupOrCreate(pid);
	pidGroup.info = ProgrammInformation::forProcess(pidGroup.pid, pidGroup.isSystemSound(), QSize(cx, cy));

	GUID guid;
	if(FAILED(session.control().GetGroupingParam(&guid)))
		return;

	auto &group = pidGroup.findGroupOrCreate(guid);
	group.insert(std::move(sessionPtr));

	removeAllItems();
	auto item = createItem(session, pidGroup);
	volumeItems.emplace_back(std::move(item));
	sortItems();
	reinsertAllItems();
}

void VolumeControlList::addItem(QGridLayout &layout, VolumeItemBase &item, int row) {
	layout.addWidget(item.descriptionButton(), row, 0);
	layout.addWidget(item.volumeSlider(), row, 1);
	layout.addWidget(item.volumeLabel(), row, 2);
}

void VolumeControlList::resizeEvent(QResizeEvent *) {
//	qDebug() << "Resize VolumeControlList";
}

void VolumeControlList::setShowInactive(bool value) {
	if(value == _showInactive)
		return;
	_showInactive = value;
	if(_showInactive) {
		if(volumeItemsInactive.empty())
			return;
		removeAllItems();

		for(const auto &item : volumeItemsInactive) {
			item->show();
		}

		volumeItems.insert(volumeItems.end(), std::move_iterator(volumeItemsInactive.begin()), std::move_iterator(volumeItemsInactive.end()));
		volumeItemsInactive.clear();
		sortItems();
		reinsertAllItems();
	} else {
		removeAllItems();
		for(auto &item : volumeItems) {
			const auto state = item->control().state();
			if(state != AudioSessionState::AudioSessionStateInactive)
				continue;
			item->hide();
			volumeItemsInactive.push_back(std::move(item));
		}

		const auto firstRemoved = std::remove_if(volumeItems.begin(), volumeItems.end(), [](const auto &item) {
			return !item;
		});
		volumeItems.erase(firstRemoved, volumeItems.end());
		reinsertAllItems();
	}
}

std::unique_ptr<SessionVolumeItem> VolumeControlList::createItem(AudioSession &sessionControl, const AudioSessionPidGroup &group) {
	std::unique_ptr<SessionVolumeItem> item = std::make_unique<SessionVolumeItem>(this, sessionControl);

	Q_ASSERT(group.info);
	item->setInfo(group.info->icon(), group.info->title());

	connect(&sessionControl, &AudioSession::volumeChanged, item.get(), &SessionVolumeItem::setVolumeFAndMute, Qt::ConnectionType::QueuedConnection);
	connect(&sessionControl, &AudioSession::stateChanged, item.get(), [this, &control = *item](int state) {
		qDebug() << "Session state of"  << control.identifier() << "changed" << state;
		if(state == AudioSessionState::AudioSessionStateActive)
			onSessionActive(control);
		if(state == AudioSessionState::AudioSessionStateInactive)
			onSessionInactive(control);
		else if(state == AudioSessionState::AudioSessionStateExpired)
			onSessionExpire(control);
	}, Qt::ConnectionType::QueuedConnection);

	qDebug() << "Created" << item->identifier() << "pid" << group.pid;
	return item;
}

void VolumeControlList::createItems() {
	auto cx = 32 * logicalDpiX() / 96.0;
	auto cy = 32 * logicalDpiY() / 96.0;

	std::for_each(sessionGroups.groups.begin(), sessionGroups.groups.end(), [&](std::unique_ptr<AudioSessionPidGroup> &g) {
		g->info = ProgrammInformation::forProcess(g->pid, g->isSystemSound(), QSize(cx, cy));
	});

	for(auto &g : sessionGroups.groups) {
		for(auto &gl : g->groups) {
			for(auto &sessionControl : gl->members) {
				const auto state = sessionControl->state();
				if(state == AudioSessionState::AudioSessionStateExpired)
					continue;

				auto item = createItem(*sessionControl, *g);
				if(state == AudioSessionState::AudioSessionStateActive) {
					volumeItems.emplace_back(std::move(item));
				} else if(state == AudioSessionStateInactive) {
					item->hide();
					volumeItemsInactive.emplace_back(std::move(item));
				}
			}
		}
	}

	sortItems();
	reinsertAllItems();
}

void VolumeControlList::addNewItem(std::unique_ptr<SessionVolumeItem> &&item) {
	const auto state = item->control().state().value_or(AudioSessionState::AudioSessionStateExpired);
	if(state == AudioSessionState::AudioSessionStateActive) {
		insertActiveItem(std::move(item));
	} else if(state == AudioSessionStateInactive) {
		volumeItemsInactive.emplace_back(std::move(item));
	}
}

std::unique_ptr<SessionVolumeItem> VolumeControlList::removeActiveItem(std::vector<std::unique_ptr<SessionVolumeItem>>::iterator it) {
	removeAllItems();
	auto item = std::move(*it);
	qDebug() << "Removing active item" << item->identifier();
	volumeItems.erase(it);
	reinsertAllItems();
	return item;
}

void VolumeControlList::insertActiveItem(std::unique_ptr<SessionVolumeItem> &&item) {
	removeAllItems();
	qDebug() << "Inserting active item" << item->identifier();
	volumeItems.emplace_back(std::move(item));
	sortItems();
	reinsertAllItems();
}

void VolumeControlList::sortItems() {
	std::sort(volumeItems.begin(), volumeItems.end(), [](const SessionVolumeItemPtr &a, const SessionVolumeItemPtr &b) {
		return a->identifier() < b->identifier();
	});
}

void VolumeControlList::removeAllItems() {
	for(auto &item : volumeItems) {
		layout.removeWidget(item->descriptionButton());
		layout.removeWidget(item->volumeSlider());
		layout.removeWidget(item->volumeLabel());
	}
}

void VolumeControlList::reinsertAllItems() {
	for(size_t i = 0; i < volumeItems.size(); ++i) {
		addItem(layout, *volumeItems[i], static_cast<int>(i));
	}
}

void VolumeControlList::onSessionActive(SessionVolumeItem &sessionVolume) {
	if(showInactive()) {
		qDebug() << "Show inactive activated, ignoring onSessionActive of" << sessionVolume.identifier();
		return;
	}

	const auto it = FindItem(volumeItemsInactive, sessionVolume);

	if(it == volumeItemsInactive.end()) {
		const auto it = FindItem(volumeItems, sessionVolume);
		if(it == volumeItems.end())
			qDebug() << "Tried to reactivate not existing item" << sessionVolume.identifier();
		else
			qDebug() << "Tried to reactivate already active item" << sessionVolume.identifier();
		return;
	}

	auto &item = **it;
	item.show();
	insertActiveItem(std::move(*it));
	volumeItemsInactive.erase(it);
}

void VolumeControlList::onSessionInactive(SessionVolumeItem &sessionVolume) {
	if(showInactive()) {
		qDebug() << "Show inactive activated, ignoring onSessionInactive of" << sessionVolume.identifier();
		return;
	}

	const auto it = FindItem(volumeItems, sessionVolume);
	if(it == volumeItems.end()) {
		qDebug() << "Failed to find inactive item" << sessionVolume.identifier();
		return;
	}

	auto item = removeActiveItem(it);
	qDebug() << "Hiding inactive item" << sessionVolume.identifier();
	item->hide();
	volumeItemsInactive.emplace_back(std::move(item));
}

void VolumeControlList::onSessionExpire(SessionVolumeItem &sessionVolume) {
	const auto it = FindItem(volumeItems, sessionVolume);
	if(it == volumeItems.end()) {
		// maybe it's already deactivated
		const auto it = FindItem(volumeItemsInactive, sessionVolume);
		if(it == volumeItemsInactive.end()) {
			qDebug() << "Failed to find item that expired" << sessionVolume.identifier();
			return;
		}

		auto &item = *it;
		qDebug() << "Removing expired item" << item->identifier();
		volumeItemsInactive.erase(it);
	} else {
		auto item = removeActiveItem(it);
		qDebug() << "Removing expired item" << item->identifier();
	}
}
