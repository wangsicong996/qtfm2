#include "iconfilelistview.h"
#include "iconview.h"

#include <QApplication>
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

void IconFileListView::beginDeferredListViewPress(const QMouseEvent *event)
{
    Q_UNUSED(event);
    QMouseEvent press(QEvent::MouseButtonPress,
                      m_plainLeftPressPos,
                      Qt::LeftButton,
                      Qt::LeftButton,
                      Qt::NoModifier);
    QListView::mousePressEvent(&press);
}

void IconFileListView::mousePressEvent(QMouseEvent *event)
{
    // Avoid QListView ExtendedSelection range-select after folder navigation on double-click.
    if (viewMode() == QListView::IconMode
        && event->button() == Qt::LeftButton
        && (event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier)) == 0) {
        m_plainLeftPressIndex = indexAt(event->pos());
        m_plainLeftPressActive = m_plainLeftPressIndex.isValid();
        m_plainLeftPressPos = event->pos();
        if (m_plainLeftPressActive && selectionModel()) {
            selectionModel()->setCurrentIndex(m_plainLeftPressIndex,
                                              QItemSelectionModel::ClearAndSelect);
        }
        event->accept();
        return;
    }

    m_plainLeftPressActive = false;
    QListView::mousePressEvent(event);
}

void IconFileListView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_plainLeftPressActive) {
        const int dragDistance = QApplication::startDragDistance();
        if ((event->pos() - m_plainLeftPressPos).manhattanLength() >= dragDistance) {
            m_plainLeftPressActive = false;
            beginDeferredListViewPress(event);
            QListView::mouseMoveEvent(event);
            return;
        }
    }
    QListView::mouseMoveEvent(event);
}

void IconFileListView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_plainLeftPressActive && event->button() == Qt::LeftButton) {
        m_plainLeftPressActive = false;
        if (indexAt(event->pos()) == m_plainLeftPressIndex) {
            emit clicked(m_plainLeftPressIndex);
        }
        event->accept();
        return;
    }
    QListView::mouseReleaseEvent(event);
}

void IconFileListView::mouseDoubleClickEvent(QMouseEvent *event)
{
    m_plainLeftPressActive = false;
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
