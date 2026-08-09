#ifndef ICONFILELISTVIEW_H
#define ICONFILELISTVIEW_H

#include <QListView>
#include <QPoint>
#include <QUrl>

class IconViewDelegate;

/**
 * Icon-mode file list. Outbound DnD uses real QDrag (file URLs).
 * Movement is always Static — Free mode only slides thumbnails in-view.
 */
class IconFileListView : public QListView
{
    Q_OBJECT
public:
    explicit IconFileListView(QWidget *parent = nullptr);

    void suppressRubberBandUntilMouseRelease();
    void ensureFileDragMode();

    QModelIndex indexAt(const QPoint &point) const override;

protected:
    void showEvent(QShowEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void startDrag(Qt::DropActions supportedActions) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QRect contentRectForVisualRect(const QRect &cellRect) const;
    QModelIndex cellIndexAt(const QPoint &point) const;
    QString filePathFromProxyIndex(const QModelIndex &proxyIndex) const;
    QList<QUrl> urlsFromSelectionOrPress() const;
    void armFileDrag(const QModelIndex &cellIndex);

    bool m_suppressRubberBandUntilRelease = false;
    QPoint m_pressPos;
    bool m_fileDragArmed = false;
    QList<QUrl> m_dragUrlsSnapshot;
};

#endif
