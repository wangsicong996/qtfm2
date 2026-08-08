#ifndef ICONFILELISTVIEW_H
#define ICONFILELISTVIEW_H

#include <QListView>
#include <QPoint>
#include <QUrl>

class IconViewDelegate;

/**
 * Icon-mode file list: hit-testing matches icon+label chrome, not full grid cell.
 * Outbound DnD mirrors the working PyQt FileGridWidget (manual startDrag + Move/cut).
 */
class IconFileListView : public QListView
{
    Q_OBJECT
public:
    explicit IconFileListView(QWidget *parent = nullptr);

    /** After changing folder, ignore drag/rubber-band until the mouse is released. */
    void suppressRubberBandUntilMouseRelease();

    QModelIndex indexAt(const QPoint &point) const override;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void startDrag(Qt::DropActions supportedActions) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QRect contentRectForVisualRect(const QRect &cellRect) const;
    QList<QUrl> selectedLocalFileUrls() const;
    void snapshotDragFromSelection();

    bool m_suppressRubberBandUntilRelease = false;
    QPoint m_pressPos;
    bool m_pressOnItem = false;
    bool m_pressOnSelectedItem = false;
    bool m_pressPlainLeftOnItem = false;
    QList<QUrl> m_dragUrlsSnapshot;
};

#endif
