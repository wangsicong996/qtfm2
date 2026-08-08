#include "iconfilelistview.h"
#include "iconview.h"
#include "common.h"
#include "mymodel.h"
#include "mymodelitem.h"

#include <QFontMetrics>
#include <QMouseEvent>
#include <QApplication>
#include <QWheelEvent>
#include <QFileInfo>
#include <QAbstractProxyModel>
#include <QSet>

IconFileListView::IconFileListView(QWidget *parent)
    : QListView(parent)
{
    // Static: Free/Snap rearranges icons instead of exporting file URLs.
    setMovement(QListView::Static);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    // Default Move like the working PyQt FileGridWidget (Thunar expects move/cut).
    setDefaultDropAction(Qt::MoveAction);
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
    m_pressPlainLeftOnItem = false;
    m_pressOnItem = false;
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

QList<QUrl> IconFileListView::selectedLocalFileUrls() const
{
    QList<QUrl> urls;
    if (!model() || !selectionModel()) {
        return urls;
    }

    QSet<QString> seen;
    const QModelIndexList indexes = selectionModel()->selectedIndexes();
    for (const QModelIndex &index : indexes) {
        if (!index.isValid()) {
            continue;
        }
        QModelIndex use = (index.column() == 0) ? index : index.sibling(index.row(), 0);
        if (!use.isValid()) {
            use = index;
        }

        QModelIndex src = use;
        const QAbstractItemModel *m = model();
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

void IconFileListView::snapshotDragFromSelection()
{
    m_dragUrlsSnapshot = selectedLocalFileUrls();
}

void IconFileListView::mousePressEvent(QMouseEvent *event)
{
    if (m_suppressRubberBandUntilRelease && event->button() == Qt::LeftButton) {
        event->accept();
        return;
    }

    m_pressPos = QPoint();
    m_pressOnItem = false;
    m_pressOnSelectedItem = false;
    m_pressPlainLeftOnItem = false;
    m_dragUrlsSnapshot.clear();

    if (event->button() != Qt::LeftButton) {
        QListView::mousePressEvent(event);
        return;
    }

    m_pressPos = event->pos();
    const QModelIndex idx = indexAt(event->pos());
    m_pressOnItem = idx.isValid();
    m_pressOnSelectedItem = idx.isValid() && selectionModel() && selectionModel()->isSelected(idx);

    // Match PyQt FileGridWidget: plain left on item → select + snapshot URLs,
    // do NOT call super (avoids rubber-band / broken QListView drag state).
    if ((event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier)) == 0
        && idx.isValid()) {
        m_pressPlainLeftOnItem = true;
        if (selectionModel() && !selectionModel()->isSelected(idx)) {
            selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect);
        }
        m_pressOnSelectedItem = selectionModel() && selectionModel()->isSelected(idx);
        snapshotDragFromSelection();
        event->accept();
        return;
    }

    // Empty area or modified click: normal selection / rubber band.
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

    // Match PyQt: plain left drag on item → manual startDrag(Move), never rubber-band.
    if ((event->buttons() & Qt::LeftButton)
        && m_pressPlainLeftOnItem
        && !m_pressPos.isNull()) {
        const int dist = (event->pos() - m_pressPos).manhattanLength();
        if (dist >= QApplication::startDragDistance() && m_pressOnSelectedItem) {
            if (m_dragUrlsSnapshot.isEmpty()) {
                snapshotDragFromSelection();
            }
            startDrag(Qt::MoveAction);
        }
        event->accept();
        return;
    }

    // Press started on an item with modifiers: do not start rubber band.
    if ((event->buttons() & Qt::LeftButton) && m_pressOnItem) {
        QListView::mouseMoveEvent(event);
        return;
    }

    QListView::mouseMoveEvent(event);
}

void IconFileListView::mouseReleaseEvent(QMouseEvent *event)
{
    const bool wasSuppressing = m_suppressRubberBandUntilRelease;
    m_pressPlainLeftOnItem = false;
    m_pressPos = QPoint();
    m_pressOnItem = false;
    m_pressOnSelectedItem = false;

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
    Q_UNUSED(supportedActions);
    QList<QUrl> urls = m_dragUrlsSnapshot;
    if (urls.isEmpty()) {
        urls = selectedLocalFileUrls();
    }
    if (urls.isEmpty()) {
        qWarning("qtfm DnD: icon view startDrag with no local URLs");
        setState(NoState);
        return;
    }
    Common::startFileUrlDrag(this, urls, Qt::MoveAction);
    m_dragUrlsSnapshot.clear();
    m_pressPlainLeftOnItem = false;
    setState(NoState);
}
