#include "iconfilelistview.h"
#include "iconview.h"
#include "common.h"
#include "mymodel.h"
#include "mymodelitem.h"

#include <QFontMetrics>
#include <QMouseEvent>
#include <QShowEvent>
#include <QApplication>
#include <QWheelEvent>
#include <QFileInfo>
#include <QAbstractProxyModel>
#include <QSet>

IconFileListView::IconFileListView(QWidget *parent)
    : QListView(parent)
{
    ensureFileDragMode();
}

void IconFileListView::ensureFileDragMode()
{
    // setViewMode(IconMode) resets movement to Free (in-view thumbnail slide).
    setMovement(QListView::Static);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDefaultDropAction(Qt::MoveAction);
    setDragDropMode(QAbstractItemView::DragDrop);
}

void IconFileListView::showEvent(QShowEvent *event)
{
    QListView::showEvent(event);
    ensureFileDragMode();
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
    m_fileDragArmed = false;
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

QModelIndex IconFileListView::cellIndexAt(const QPoint &point) const
{
    return QListView::indexAt(point);
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

QString IconFileListView::filePathFromProxyIndex(const QModelIndex &proxyIndex) const
{
    if (!proxyIndex.isValid() || !model()) {
        return QString();
    }
    QModelIndex use = (proxyIndex.column() == 0)
                          ? proxyIndex
                          : proxyIndex.sibling(proxyIndex.row(), 0);
    if (!use.isValid()) {
        use = proxyIndex;
    }

    QModelIndex src = use;
    const QAbstractItemModel *m = model();
    while (const auto *proxy = qobject_cast<const QAbstractProxyModel *>(m)) {
        src = proxy->mapToSource(src);
        m = proxy->sourceModel();
        if (!src.isValid() || !m) {
            return QString();
        }
    }

    if (auto *fm = const_cast<myModel *>(qobject_cast<const myModel *>(m))) {
        return fm->filePath(src);
    }
    if (src.internalPointer()) {
        return static_cast<myModelItem *>(src.internalPointer())->absoluteFilePath();
    }
    return QString();
}

QList<QUrl> IconFileListView::urlsFromSelectionOrPress() const
{
    QList<QUrl> urls;
    QSet<QString> seen;

    const auto appendPath = [&](const QString &path) {
        if (path.isEmpty() || seen.contains(path)) {
            return;
        }
        seen.insert(path);
        const QUrl url = QUrl::fromLocalFile(QFileInfo(path).absoluteFilePath());
        if (url.isValid()) {
            urls.append(url);
        }
    };

    if (selectionModel()) {
        const QModelIndexList indexes = selectionModel()->selectedIndexes();
        for (const QModelIndex &index : indexes) {
            appendPath(filePathFromProxyIndex(index));
        }
    }

    // Fallback / ensure pressed file is included even if selection lagged.
    if (!m_dragUrlsSnapshot.isEmpty() && urls.isEmpty()) {
        return m_dragUrlsSnapshot;
    }
    return urls;
}

void IconFileListView::armFileDrag(const QModelIndex &cellIndex)
{
    m_fileDragArmed = true;
    m_dragUrlsSnapshot.clear();

    if (selectionModel() && cellIndex.isValid()) {
        if (!selectionModel()->isSelected(cellIndex)) {
            selectionModel()->select(cellIndex, QItemSelectionModel::ClearAndSelect);
            selectionModel()->setCurrentIndex(cellIndex, QItemSelectionModel::Current);
        }
    }

    // Snapshot immediately from selection + pressed cell path.
    QSet<QString> seen;
    const auto appendPath = [&](const QString &path) {
        if (path.isEmpty() || seen.contains(path)) {
            return;
        }
        seen.insert(path);
        const QUrl url = QUrl::fromLocalFile(QFileInfo(path).absoluteFilePath());
        if (url.isValid()) {
            m_dragUrlsSnapshot.append(url);
        }
    };

    appendPath(filePathFromProxyIndex(cellIndex));
    if (selectionModel()) {
        for (const QModelIndex &index : selectionModel()->selectedIndexes()) {
            appendPath(filePathFromProxyIndex(index));
        }
    }

    qInfo("qtfm DnD: armed %d url(s)", m_dragUrlsSnapshot.size());
}

void IconFileListView::mousePressEvent(QMouseEvent *event)
{
    ensureFileDragMode();

    if (m_suppressRubberBandUntilRelease && event->button() == Qt::LeftButton) {
        event->accept();
        return;
    }

    m_pressPos = QPoint();
    m_fileDragArmed = false;
    m_dragUrlsSnapshot.clear();

    if (event->button() != Qt::LeftButton) {
        QListView::mousePressEvent(event);
        return;
    }

    m_pressPos = event->pos();
    const QModelIndex cell = cellIndexAt(event->pos());

    // Plain left on a file cell → arm real file drag (do not call QListView press,
    // which enters DraggingState and can slide icons if movement were Free).
    if ((event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier)) == 0
        && cell.isValid()) {
        armFileDrag(cell);
        setState(NoState);
        // Mouse events are delivered via the viewport — grab it, not the view.
        if (viewport()) {
            viewport()->grabMouse();
        }
        event->accept();
        return;
    }

    QListView::mousePressEvent(event);
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

    if ((event->buttons() & Qt::LeftButton) && m_fileDragArmed && !m_pressPos.isNull()) {
        const int dist = (event->pos() - m_pressPos).manhattanLength();
        if (dist >= QApplication::startDragDistance()) {
            if (m_dragUrlsSnapshot.isEmpty()) {
                m_dragUrlsSnapshot = urlsFromSelectionOrPress();
            }
            if (!m_dragUrlsSnapshot.isEmpty()) {
                if (viewport() && mouseGrabber() == viewport()) {
                    viewport()->releaseMouse();
                }
                startDrag(Qt::CopyAction | Qt::MoveAction);
            } else {
                qWarning("qtfm DnD: armed but no URLs — cannot start drag");
            }
            m_fileDragArmed = false;
        }
        event->accept();
        return;
    }

    QListView::mouseMoveEvent(event);
}

void IconFileListView::mouseReleaseEvent(QMouseEvent *event)
{
    const bool wasSuppressing = m_suppressRubberBandUntilRelease;
    m_fileDragArmed = false;
    m_pressPos = QPoint();
    if (viewport() && mouseGrabber() == viewport()) {
        viewport()->releaseMouse();
    }

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
    setState(NoState);
}

void IconFileListView::mouseDoubleClickEvent(QMouseEvent *event)
{
    const QModelIndex idx = cellIndexAt(event->pos());
    if (idx.isValid() && selectionModel()) {
        selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect);
        selectionModel()->setCurrentIndex(idx, QItemSelectionModel::Current);
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
    ensureFileDragMode();
    QList<QUrl> urls = m_dragUrlsSnapshot;
    if (urls.isEmpty()) {
        urls = urlsFromSelectionOrPress();
    }
    if (urls.isEmpty()) {
        qWarning("qtfm DnD: startDrag with no local URLs");
        setState(NoState);
        return;
    }
    // Never call QListView::startDrag (Free mode = slide thumbnails only).
    Common::startFileUrlDrag(this, urls, supportedActions);
    m_dragUrlsSnapshot.clear();
    m_fileDragArmed = false;
    setState(NoState);
    if (viewport()) {
        viewport()->update();
    }
}
