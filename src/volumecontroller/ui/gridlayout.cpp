#include "gridlayout.h"

#include <QDebug>

#include <volumecontroller/joiner.h>

GridLayout::GridLayout(QWidget *parent) : QLayout(parent) {}

void GridLayout::addColumn(GridLayout::ColumnStyle style, int weight) {
	Q_ASSERT(rows.empty());
	Q_ASSERT(0 <= weight);
	Q_ASSERT(style != ColumnStyle::Fill || 0 < weight);
	auto &c = columns.emplace_back(style, weight);
	onColumnAdd(c);
}

void GridLayout::removeColumn(int index) {
	Q_ASSERT(rows.empty());
	Q_ASSERT(0 <= index && index < columnCount());
	auto it = columns.begin() + index;
	onColumnRemove(*it);
	if(it->style == ColumnStyle::Fill)
		totalWeight -= it->styleInfo;
	columns.erase(it);
}

void GridLayout::insertRow(int index) {
	Q_ASSERT(0 <= index && index < rowCount());
	const auto it = rows.begin() + index;
	rows.emplace(it, columnCount());
	invalidate();
}

void GridLayout::removeRow(int index) {
	Q_ASSERT(0 <= index && index < rowCount());
	const auto it = rows.begin() + index;
	clearRow(*it);
	rows.erase(it);
	invalidate();
}

void GridLayout::clearRowItems(const int index) {
	Q_ASSERT(0 <= index && index < rowCount());
	const auto it = rows.begin() + index;
	clearRow(*it);
	invalidate();
}

void GridLayout::clearRows() {
	rows.clear();
	invalidate();
}

void GridLayout::swapRows(int a, int b) {
	Q_ASSERT(0 <= a && a < rowCount());
	Q_ASSERT(0 <= b && b < rowCount());
	auto &rowA = rows[a];
	auto &rowB = rows[b];
	std::swap(rowA.items, rowB.items);
	std::swap(rowA.height, rowB.height);
	invalidate();
}

int GridLayout::rowCount() const {
	return static_cast<int>(rows.size());
}

int GridLayout::columnCount() const {
	return static_cast<int>(columns.size());
}

void GridLayout::addItem(QLayoutItem *) {
	Q_ASSERT(false);
}

void GridLayout::setWidget(QWidget *widget, int row, int column) {
	ensureRows(row + 1);
	auto &ptr = itemPtrAt(row, column);
	Q_ASSERT(!ptr);

	addChildWidget(widget);
	ptr = std::unique_ptr<QLayoutItem>(new QWidgetItem(widget));
	++itemCount;
	invalidate();
}

int GridLayout::count() const {
	return itemCount;
}

QLayoutItem *GridLayout::itemAt(int index) const {
	for(const auto &row : rows) {
		for(const auto &v : row.items) {
			if(v && index-- == 0)
				return v.get();
		}
	}
	return nullptr;
}

QLayoutItem *GridLayout::takeAt(int index) {
	for(auto &row : rows) {
		for(auto &v : row.items) {
			if(v && index-- == 0) {
				--itemCount;
				return v.release();
			}
		}
	}
	return nullptr;
}

template<typename F> QSize CalculateStat(const std::vector<GridLayout::Row> &rows, int columns, int spacing, F &&f) {
	QSize total(0, 0);
	std::vector<int> columnWidths(columns);

	for(size_t j = 0; j < rows.size(); ++j) {
		const auto &row = rows[j];
		int rowHeight = 0;
		for(int i = 0; i < columns; ++i) {
			const auto &itemPtr = row.items[i];
			if(!itemPtr)
				continue;

			const auto &item = *itemPtr;
			const QSize value = f(item);
			rowHeight = std::max(rowHeight, value.height());
			columnWidths[i] = std::max(columnWidths[i], value.width());
		}
		if(rowHeight != 0 && j != 0)
			total.rheight() += spacing;
		total.rheight() += rowHeight;
	}

	for(size_t i = 0; i < columnWidths.size(); ++i) {
		const auto &width = columnWidths[i];
		total.rwidth() += width;
		if(width != 0 && i != 0)
			total.rwidth() += spacing;
	}

	return total;
}

QSize GridLayout::minimumSize() const {
	const auto ms = CalculateStat(rows, columnCount(), spacing(), [](const QLayoutItem &item) {
		return item.minimumSize();
	});
//	qDebug() << "minimumSize" << ms;
	return ms;
}

QSize GridLayout::maximumSize() const {
	const auto ms = CalculateStat(rows, columnCount(), spacing(), [](const QLayoutItem &item) {
		return item.maximumSize();
	});
//	qDebug() << "maximumSize" << ms;
	return ms;
}

QSize GridLayout::sizeHint() const {
	const auto sh = CalculateStat(rows, columnCount(), spacing(), [](const QLayoutItem &item) {
		return item.sizeHint();
	});
//	qDebug() << "sizeHint" << sh;
	return sh;
}

Qt::Orientations GridLayout::expandingDirections() const {
	return Qt::Orientation::Horizontal;
}

void GridLayout::setGeometry(const QRect &rect) {
//	qDebug() << "Setting geometry to" << rect;
	for(auto &column : columns) {
		column.width = 0;
	}

	for(auto it = rows.begin(); it != rows.end(); ++it) {
		auto &row = *it;
		row.height = 0;
//		auto debug = qDebug();
//		debug << "Row" << std::distance(rows.begin(), it) << "with width" << row.items.size();
//		bool empty = true;

		ForeachTied([&](std::unique_ptr<QLayoutItem> &itemPtr, ColumnInfo &column) {
			if(!itemPtr) {
//				debug << QSize(0, 0);
				return;
			}
//			empty = false;

			auto &item = *itemPtr;
			const QSize value = item.sizeHint();
//			debug << value;

			column.width = std::max(column.width, value.width());
			row.height = std::max(row.height, value.height());
		}, row.items, columns);
//		debug << "final height" << row.height;
//		if(empty)
//			debug << "!!row was empty!!";
	}

	int totalWidth = 0;
	for(size_t i = 0; i < columns.size(); ++i) {
		const auto &c = columns[i];
		totalWidth += c.width;
		if(c.width != 0 && i != 0)
			totalWidth += spacing();
	}

	const int remainingWidth = rect.width() - totalWidth;
	Q_ASSERT(0 <= remainingWidth);

	for(int i = 0; i < columnCount(); ++i) {
		auto &column = columns[i];
		if(column.style != ColumnStyle::Fill)
			continue;

		const int addW = remainingWidth * column.styleInfo / totalWeight;
		column.width += addW;
	}

//	qDebug().nospace().noquote() << "column widths: " << Joiner(columns, ' ', [](auto &str, const auto &column) {
//		str << column.width;
//	});

	int y = rect.y();
	for(size_t j = 0; j < rows.size(); ++j) {
		const auto &row = rows[j];
		const auto height = row.height;

		if(height != 0 && j != 0)
			y += spacing();

		int x = rect.x();
		for(int i = 0; i < columnCount(); ++i) {
			const auto &itemPtr = row.items[i];
			if(!itemPtr)
				continue;

			auto &item = *itemPtr;
			const auto &column = columns[i];
			if(column.width != 0 && i != 0)
				x += spacing();
			const auto width = column.width;

			const auto itemRect = QRect(x, y, width, height);
//			qDebug() << i << j << itemRect;
			item.setGeometry(itemRect);
			x += width;
		}
		y += height;
	}
}

void GridLayout::clearRow(GridLayout::Row &row) {
	for(auto &v : row.items) {
		if(!v)
			continue;
		--itemCount;
		v.reset();
	}
}

bool GridLayout::hasHeightForWidth() const {
	return false;
}

void GridLayout::ensureRows(int rowCount) {
	Q_ASSERT(0 <= rowCount);
	rows.reserve(rowCount);
	while(rows.size() < size_t(rowCount)) {
		rows.emplace_back(columnCount());
	}
}

std::unique_ptr<QLayoutItem> &GridLayout::itemPtrAt(int row, int column) {
	Q_ASSERT(row < rowCount());
	Q_ASSERT(column < columnCount());
	return rows[row].items[column];
}

QLayoutItem *GridLayout::itemPtrAt(int row, int column) const {
	Q_ASSERT(row < rowCount());
	Q_ASSERT(column < columnCount());
	return rows[row].items[column].get();
}

void GridLayout::onColumnAdd(const GridLayout::ColumnInfo &c) {
	if(c.style == ColumnStyle::Fill)
		totalWeight += c.styleInfo;
}

void GridLayout::onColumnRemove(const GridLayout::ColumnInfo &c) {
	if(c.style == ColumnStyle::Fill)
		totalWeight -= c.styleInfo;
}
