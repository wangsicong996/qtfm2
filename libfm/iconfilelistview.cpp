#include "iconfilelistview.h"
#include "iconview.h"
#include "common.h"

#include <QFontMetrics>
#include <QMouseEvent>
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QWheelEvent>

IconFileListView::IconFileListView(QWidget *parent)
    : QListView(parent)
{
}

void IconFileListView::suppressRubberBandUntilMouseRelease()
{
    if ((QApplication::mouseButtons() & Qt::LeftButton) == 0) {
        if (selectionModel()) {
            selectionModel()->clearSelection();
            selectionModel()->clearCurrentIndex();
        }
        return;
    }
    m_suppressRubberBandUntilRelease = true;
    if (selectionModel()) {
        selectionModel()->clearSelection();
        selectionModel()->clearCurrentIndex();
    }
    setState(NoState);
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
    if (m_suppressRubberBandUntilRelease && event->button() == Qt::LeftButton) {
        event->accept();
        return;
    }

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

void IconFileListView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_suppressRubberBandUntilRelease) {
        if (selectionModel()) {
            selectionModel()->clearSelection();
        }
        event->accept();
        return;
    }
    QListView::mouseMoveEvent(event);
}

void IconFileListView::mouseReleaseEvent(QMouseEvent *event)
{
    const bool wasSuppressing = m_suppressRubberBandUntilRelease;
    if (wasSuppressing) {
        m_suppressRubberBandUntilRelease = false;
        if (selectionModel()) {
            selectionModel()->clearSelection();
            selectionModel()->clearCurrentIndex();
        }
        setState(NoState);
    }
    QListView::mouseReleaseEvent(event);
    if (wasSuppressing && selectionModel()) {
        selectionModel()->clearSelection();
    }
}

void IconFileListView::mouseDoubleClickEvent(QMouseEvent *event)
{
    const QModelIndex idx = indexAt(event->pos());
    if (idx.isValid() && selectionModel()) {
        selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect);
    }
    if (idx.isValid()) {
        suppressRubberBandUntilMouseRelease();
        emit doubleClicked(idx);
        emit activated(idx);
        event->accept();
        return;
    }
    QListView::mouseDoubleClickEvent(event);
}

void IconFileListView::startDrag(Qt::DropActions supportedActions)
{
    if (!model()) {
        return;
    }
    QModelIndexList indexes;
    if (selectionModel()) {
        indexes = selectionModel()->selectedIndexes();
    }
    if (indexes.isEmpty()) {
        return;
    }

    QMimeData *data = model()->mimeData(indexes);
    if (!data || !data->hasUrls()) {
        delete data;
        return;
    }

    QDrag *drag = new QDrag(this);
    drag->setMimeData(data);
    const QModelIndex first = indexes.constFirst();
    const QIcon icon = qvariant_cast<QIcon>(first.data(Qt::DecorationRole));
    if (!icon.isNull()) {
        const QPixmap pm = icon.pixmap(32, 32);
        drag->setPixmap(pm);
        drag->setHotSpot(QPoint(pm.width() / 2, pm.height() / 2));
    }

    // Offer Copy|Move|Link; default Copy so Electron/GTK targets accept the drop.
    // Destination (or our dropMimeData) decides same-disk move vs cross-disk copy.
    const Qt::DropActions actions =
        supportedActions & (Qt::CopyAction | Qt::MoveAction | Qt::LinkAction);
    drag->exec(actions == 0 ? supportedActions : actions, Qt::CopyAction);
}

void IconFileListView::wheelEvent(QWheelEvent *event)
{
    Common::applyFileViewWheelScroll(this, event);
}
