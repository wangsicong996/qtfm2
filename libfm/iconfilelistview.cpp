#include "iconfilelistview.h"
#include "iconview.h"
#include "common.h"
#include "mymodel.h"
#include "mymodelitem.h"

#include <QFontMetrics>
#include <QMouseEvent>
#include <QApplication>
#include <QWheelEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QAbstractProxyModel>
#include <QSet>

namespace {

QList<QUrl> selectedLocalFileUrls(QAbstractItemView *view)
{
    QList<QUrl> urls;
    if (!view || !view->model() || !view->selectionModel()) {
        return urls;
    }

    QSet<QString> seen;
    const QModelIndexList indexes = view->selectionModel()->selectedIndexes();
    for (const QModelIndex &index : indexes) {
        if (!index.isValid()) {
            continue;
        }
        QModelIndex use = (index.column() == 0) ? index : index.sibling(index.row(), 0);
        if (!use.isValid()) {
            use = index;
        }

        QModelIndex src = use;
        const QAbstractItemModel *m = view->model();
        while (const auto *proxy = qobject_cast<const QAbstractProxyModel *>(m)) {
            src = proxy->mapToSource(src);
            m = proxy->sourceModel();
            if (!src.isValid() || !m) {
                break;
            }
        }
        if (!src.isValid()) {
            continue;
        }

        QString path;
        if (auto *fm = const_cast<myModel *>(qobject_cast<const myModel *>(m))) {
            path = fm->filePath(src);
        } else if (src.internalPointer()) {
            path = static_cast<myModelItem *>(src.internalPointer())->absoluteFilePath();
        }
        if (path.isEmpty() || seen.contains(path)) {
            continue;
        }
        seen.insert(path);
        const QUrl url = QUrl::fromLocalFile(QFileInfo(path).absoluteFilePath());
        if (url.isValid()) {
            urls.append(url);
        }
    }
    return urls;
}

} // namespace

IconFileListView::IconFileListView(QWidget *parent)
    : QListView(parent)
{
    // Static: Free/Snap rearranges icons in-view and never exports file URLs
    // to Thunar / Electron / other apps (QListView::startDrag special-case).
    setMovement(QListView::Static);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDefaultDropAction(Qt::CopyAction);
    setDragDropMode(QAbstractItemView::DragDrop);
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

void IconFileListView::wheelEvent(QWheelEvent *event)
{
    Common::applyFileViewWheelScroll(this, event);
}

void IconFileListView::startDrag(Qt::DropActions supportedActions)
{
    const QList<QUrl> urls = selectedLocalFileUrls(this);
    if (urls.isEmpty()) {
        qWarning("qtfm DnD: icon view startDrag with no local URLs");
        setState(NoState);
        return;
    }
    Common::startFileUrlDrag(this, urls, supportedActions);
    setState(NoState);
}
