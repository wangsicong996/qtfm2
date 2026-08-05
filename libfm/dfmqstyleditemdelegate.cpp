#include "dfmqstyleditemdelegate.h"
#include "searchhighlight.h"

#include <QAbstractItemModel>
#include <QAbstractProxyModel>
#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QStyle>

DfmQStyledItemDelegate::DfmQStyledItemDelegate(QObject* parent) :
    QStyledItemDelegate(parent),
    m_hasMinimizedNameColumnSelection(false)
{
}

DfmQStyledItemDelegate::~DfmQStyledItemDelegate()
{
}

void DfmQStyledItemDelegate::paint(QPainter* painter,
                                    const QStyleOptionViewItem& option,
                                    const QModelIndex& index) const
{
    const QString needle = nameSearchNeedleFromView(option);
    const bool highlightName = !needle.isEmpty() && index.column() == COLUMN_NAME;

    if (highlightName) {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);
        if (m_hasMinimizedNameColumnSelection) {
            const QString filename = index.data(Qt::DisplayRole).toString();
            opt.rect.setWidth(nameColumnWidth(filename, opt));
        }

        const QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
        style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

        const QString filename = index.data(Qt::DisplayRole).toString();
        const bool selected = opt.state & QStyle::State_Selected;
        const QColor fg = selected ? opt.palette.highlightedText().color()
                                   : opt.palette.text().color();
        QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
        if (!textRect.isValid()) {
            textRect = opt.rect.adjusted(4, 0, -4, 0);
        }
        drawTextWithSearchHighlight(painter, textRect, filename, needle, opt.font, fg,
                                    Qt::AlignLeft | Qt::AlignVCenter);
    } else if (m_hasMinimizedNameColumnSelection && (index.column() == COLUMN_NAME)) {
        QStyleOptionViewItem opt(option);

        QString filename = index.data(Qt::DisplayRole).toString();
        if (index.isValid()) {
            const int width = nameColumnWidth(filename, opt);
            opt.rect.setWidth(width);
        }
        QStyledItemDelegate::paint(painter, opt, index);
    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }

    if (m_drawColumnSeparators && index.isValid() && index.column() < COLUMN_FOLDER) {
        const QColor line = option.palette.color(QPalette::Mid);
        painter->save();
        painter->setPen(line);
        const int x = option.rect.right();
        painter->drawLine(x, option.rect.top(), x, option.rect.bottom());
        painter->restore();
    }
}

int DfmQStyledItemDelegate::nameColumnWidth(const QString& name, const QStyleOptionViewItem& option)
{
    QFontMetrics fontMetrics(option.font);
    return option.decorationSize.width() + fontMetrics.horizontalAdvance(name) + 16;
}
