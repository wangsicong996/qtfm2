#include "iconfilelistview.h"
#include "iconview.h"

#include <QFontMetrics>
#include <QMouseEvent>

IconFileListView::IconFileListView(QWidget *parent)
    : QListView(parent)
{
}

QRect IconFileListView::contentRectForVisualRect(const QRect &cellRect) const
{
    if (!cellRect.isValid()) {
        return QRect();
    }
    const auto *iv = dynamic_cast<const IconViewDelegate *>(itemDelegate());
    int gapH = 4;
    int gapV = 4;
    if (iv) {
        gapH = iv->cellGapH();
        gapV = iv->cellGapV();
    }
    const int zoom = qMax(iconSize().width(), IconViewDelegate::iconZoomMin);
    return IconViewDelegate::itemHighlightRect(cellRect, zoom, gapH, gapV, fontMetrics());
}

QModelIndex IconFileListView::indexAt(const QPoint &point) const
{
    if (viewMode() != QListView::IconMode) {
        return QListView::indexAt(point);
    }
    const QModelIndex idx = QListView::indexAt(point);
    if (!idx.isValid()) {
        return QModelIndex();
    }
    const QRect hit = contentRectForVisualRect(visualRect(idx));
    if (!hit.contains(point)) {
        return QModelIndex();
    }
    return idx;
}

void IconFileListView::mousePressEvent(QMouseEvent *event)
{
    // ExtendedSelection: a double-click's first press can range-select from a stale
    // anchor (often row 0) to the clicked item. Single-click-open avoids this path.
    if (viewMode() == QListView::IconMode
        && event->button() == Qt::LeftButton
        && (event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier)) == 0) {
        const QModelIndex idx = indexAt(event->pos());
        if (idx.isValid() && selectionModel()) {
            selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect);
        }
    }
    QListView::mousePressEvent(event);
    if (viewMode() == QListView::IconMode
        && event->button() == Qt::LeftButton
        && (event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier)) == 0) {
        const QModelIndex idx = indexAt(event->pos());
        if (idx.isValid() && selectionModel() && selectionModel()->selectedRows().size() > 1) {
            selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect);
        }
    }
}

void IconFileListView::mouseDoubleClickEvent(QMouseEvent *event)
{
    const QModelIndex idx = indexAt(event->pos());
    if (idx.isValid() && selectionModel()) {
        selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect);
    }
    if (idx.isValid()) {
        emit doubleClicked(idx);
        emit activated(idx);
        event->accept();
        return;
    }
    QListView::mouseDoubleClickEvent(event);
}
