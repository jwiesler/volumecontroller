#include "volumecontrollist.h"
#include "volumecontroller/info/processdata.h"
#include <QDebug>

#include "volumecontroller/collections.h"
#include <volumecontroller/joiner.h>

static std::vector<std::unique_ptr<SessionVolumeItem>>::iterator FindItem(std::vector<std::unique_ptr<SessionVolumeItem>> &items, const SessionVolumeItem &sessionVolume) {
	return std::find_if(items.begin(), items.end(), [&](const std::unique_ptr<SessionVolumeItem> &item) {
		return &sessionVolume == item.get();
	});
}

constexpr auto sessionVolumeItemComparator = [](const SessionVolumeItem &a, const SessionVolumeItem &b) {
	return a.identifier() < b.identifier();
};

constexpr auto sessionVolumeItemPtrComparator = [](const SessionVolumeItemPtr &a, const SessionVolumeItemPtr &b) {
	return sessionVolumeItemComparator(*a, *b);
};

VolumeControlList::VolumeControlList(QWidget *parent, AudioSessionGroups &sessionGroups, const VolumeItemTheme &itemTheme, bool showInactive)
	: QWidget(parent),
	  layout(this),
	  sessionGroups(sessionGroups),
	  itemTheme(itemTheme)
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

	layout.addColumn(GridLayout::ColumnStyle::Fixed);
	layout.addColumn(GridLayout::ColumnStyle::Fill, 1);
	layout.addColumn(GridLayout::ColumnStyle::Fixed);
	layout.setSpacing(12);
	layout.setSizeConstraint(QLayout::SetDefaultConstraint);
	layout.setAlignment(Qt::AlignTop);
	layout.setContentsMargins(0, 0, 0, 0);

	createItems(showInactive);
}

void VolumeControlList::updatePeaks() {
	std::for_each(volumeItems.begin(), volumeItems.end(), [](std::unique_ptr<SessionVolumeItem> &item) {
		item->updatePeak();
	});
}

void VolumeControlList::addSession(std::unique_ptr<AudioSession> &&sessionPtr) {
	auto &session = *sessionPtr;
	if(session.state().value_or(AudioSession::State::Expired) == AudioSession::State::Expired)
		return;

	auto pidOpt = session.pid();
	if(!pidOpt)
		return;

	auto cx = 32 * logicalDpiX() / 96.0;
	auto cy = 32 * logicalDpiY() / 96.0;

	auto &pidGroup = sessionGroups.findPidGroupOrCreate(*pidOpt);
	if(!pidGroup.infoPtr())
		pidGroup.setInfoPtr(ProgrammInformation::forProcess(pidGroup.pid(), pidGroup.isSystemSound(), QSize(cx, cy)));

	GUID guid;
	if(FAILED(session.control().GetGroupingParam(&guid)))
		return;

	auto &group = pidGroup.findGroupOrCreate(guid);
	group.insert(std::move(sessionPtr));

	addNewItem(createItem(session, pidGroup));
}

void VolumeControlList::addItem(QGridLayout &layout, VolumeItemBase &item, int row) {
	layout.addWidget(item.descriptionButton(), row, 0);
	layout.addWidget(item.volumeSlider(), row, 1);
	layout.addWidget(item.volumeLabel(), row, 2);
}

void VolumeControlList::addItem(GridLayout &layout, VolumeItemBase &item, int row) {
	qDebug() << "Inserting" << item.identifier() << "into row" << row;
	layout.setWidget(item.descriptionButton(), row, 0);
	layout.setWidget(item.volumeSlider(), row, 1);
	layout.setWidget(item.volumeLabel(), row, 2);
}

void VolumeControlList::setShowInactive(bool value) {
	if(value == _showInactive)
		return;
	_showInactive = value;
	qDebug().nospace() << "Show inactive changed to " << _showInactive << ".";
	if(_showInactive) {
		qDebug().nospace() << "Showing " << volumeItemsInactive.size() << " hidden inactive items";
		if(volumeItemsInactive.empty())
			return;

		for(auto &item : volumeItemsInactive) {
			item->show();
			addItem(layout, *item, int(volumeItems.size()));
			volumeItems.emplace_back(std::move(item));
		}
		volumeItemsInactive.clear();

		sortItems();
	} else {
		std::vector<int> deletedRows;
		for(auto it = volumeItems.begin(); it != volumeItems.end(); ++it) {
			auto &item = *it;
			const auto state = item->control().state();
			if(state != AudioSession::State::Inactive)
				continue;
			item->hide();
			deletedRows.emplace_back(int(std::distance(volumeItems.begin(), it)));
			volumeItemsInactive.emplace_back(std::move(item));
		}
		qDebug().nospace() << "Hiding inactive items " << Joiner(deletedRows, ", ", [](auto &str, const auto &value) {
			str << value;
		});

		layout.removeRows(deletedRows);

		const auto it = RemoveIndices(volumeItems.begin(), volumeItems.end(), deletedRows.cbegin(), deletedRows.cend());
		volumeItems.erase(it, volumeItems.end());
	}
}

std::unique_ptr<SessionVolumeItem> VolumeControlList::createItem(AudioSession &sessionControl, const AudioSessionPidGroup &group) {
	std::unique_ptr<SessionVolumeItem> item = std::make_unique<SessionVolumeItem>(this, sessionControl, itemTheme);

	Q_ASSERT(group.infoPtr());
	item->setInfo(group.infoPtr()->icon(), group.infoPtr()->title());

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

	qDebug() << "Created session" << item->identifier() << "pid" << group.pid();
	return item;
}

void VolumeControlList::createItems(bool showInactive) {
	auto cx = 32 * logicalDpiX() / 96.0;
	auto cy = 32 * logicalDpiY() / 96.0;

	std::for_each(sessionGroups.groups().begin(), sessionGroups.groups().end(), [&](std::unique_ptr<AudioSessionPidGroup> &g) {
		g->setInfoPtr(ProgrammInformation::forProcess(g->pid(), g->isSystemSound(), QSize(cx, cy)));
	});

	for(auto &g : sessionGroups.groups()) {
		for(auto &gl : g->groups()) {
			for(auto &sessionControl : gl->members()) {
				const auto state = sessionControl->state();
				if(state == AudioSession::State::Expired)
					continue;

				auto item = createItem(*sessionControl, *g);
				if(state == AudioSession::State::Expired)
					continue;

				if(state == AudioSession::State::Active || showInactive) {
					volumeItems.emplace_back(std::move(item));
				} else if(state == AudioSession::State::Inactive) {
					qDebug() << "Hiding inactive item" << item->identifier();
					item->hide();
					volumeItemsInactive.emplace_back(std::move(item));
				}
			}
		}
	}

	for(size_t i = 0; i < volumeItems.size(); ++i) {
		addItem(layout, *volumeItems[i], int(i));
	}

	sortItems();
}

void VolumeControlList::addNewItem(std::unique_ptr<SessionVolumeItem> &&item) {
	const auto state = item->control().state().value_or(AudioSession::State::Expired);
	qDebug() << "Item" << item->identifier() << "is" << ToString(state);
	if(state == AudioSession::State::Expired)
		return;

	if(state == AudioSession::State::Active || showInactive()) {
		insertActiveItem(std::move(item));
	} else if(state == AudioSession::State::Inactive) {
		qDebug() << "Hiding inactive item" << item->identifier();
		item->hide();
		volumeItemsInactive.emplace_back(std::move(item));
	}
}

std::unique_ptr<SessionVolumeItem> VolumeControlList::removeActiveItem(std::vector<std::unique_ptr<SessionVolumeItem>>::iterator it) {
	auto item = std::move(*it);
	qDebug() << "Removing active item" << item->identifier();
	layout.removeRow(std::distance(volumeItems.begin(), it));
	volumeItems.erase(it);
	return item;
}

void VolumeControlList::insertActiveItem(std::unique_ptr<SessionVolumeItem> &&item) {
	qDebug() << "Inserting active item" << item->identifier();
	addItem(layout, *item, int(volumeItems.size()));
	volumeItems.emplace_back(std::move(item));
	sortItems();
}

void VolumeControlList::sortItems() {
	Q_ASSERT(int(volumeItems.size()) == layout.rowCount());
	auto perm = CreateSortedPermutation<int>(volumeItems.begin(), volumeItems.end(), sessionVolumeItemPtrComparator);
	ApplyPermutation(perm.begin(), perm.end(), [&](const size_t a, const size_t b) {
		std::swap(volumeItems[a], volumeItems[b]);
		layout.swapRows(int(a), int(b));
	});

	Q_ASSERT(std::is_sorted(volumeItems.begin(), volumeItems.end(), sessionVolumeItemPtrComparator));
	Q_ASSERT(int(volumeItems.size()) == layout.rowCount());
	qDebug().noquote().nospace() << "Sorted items are: " << Join(volumeItems, ", ", [](auto &str, const auto &item) {
		str << item->identifier();
	});
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
