#ifndef GRIDLAYOUT_H
#define GRIDLAYOUT_H

#include <QLayout>
#include "volumecontroller/collections.h"

class GridLayout : public QLayout {
public:
	enum class ColumnStyle {
		Fixed = 0,
		Fill
	};

	struct ColumnInfo {
		ColumnStyle style;
		int styleInfo;
		int width = 0;

		ColumnInfo(ColumnStyle style, int styleInfo)
			: style(style),
			  styleInfo(styleInfo) {}
	};

	class LayoutItem : public QLayoutItem {
	public:
		using QLayoutItem::QLayoutItem;
	};

	struct Row {
		Q_DISABLE_COPY(Row);
		Row(size_t size) : items(size) {}
		Row(Row &&) = default;
		Row &operator=(Row &&) = default;

		std::vector<std::unique_ptr<QLayoutItem>> items;
		int height = 0;
	};

	GridLayout(QWidget *parent);

	void addColumn(ColumnStyle style, int weight = 0);
	void removeColumn(int index);

	// void addRow();
	void insertRow(int index);

	// begin, end have to be sorted
	template<typename Iterator>
	void removeRows(const Iterator begin, const Iterator end) {
		const auto it = RemoveIndices(rows.begin(), rows.end(), begin, end);
		rows.erase(it, rows.end());
	}

	template<typename Collection>
	void removeRows(const Collection &collection) {
		removeRows(std::begin(collection), std::end(collection));
	}

	void removeRow(int index);
	void clearRowItems(int index);
	void clearRows();

	void swapRows(int a, int b);
	void ensureRows(int row);

	int rowCount() const;
	int columnCount() const;

	void addItem(QLayoutItem *) override;
	void setWidget(QWidget *widget, int row, int column);

	int count() const override;
	QLayoutItem *itemAt(int index) const override;
	QLayoutItem *takeAt(int index) override;

	QSize minimumSize() const override;
	QSize maximumSize() const override;
	QSize sizeHint() const override;

	Qt::Orientations expandingDirections() const override;
	bool hasHeightForWidth() const override;

	void setGeometry(const QRect &rect) override;

private:
	void clearRow(Row &row);

	std::unique_ptr<QLayoutItem> &itemPtrAt(int row, int column);
	QLayoutItem *itemPtrAt(int row, int column) const;

	void onColumnAdd(const ColumnInfo &c);
	void onColumnRemove(const ColumnInfo &c);

	std::vector<ColumnInfo> columns;
	std::vector<Row> rows;
	int totalWeight = 0;
	int itemCount = 0;
};

#endif // GRIDLAYOUT_H
