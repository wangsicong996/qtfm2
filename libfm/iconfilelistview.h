#ifndef ICONFILELISTVIEW_H
#define ICONFILELISTVIEW_H

#include <QListView>

class IconViewDelegate;

/**
 * Icon-mode file list: hit-testing matches icon+label chrome, not full grid cell.
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

private:
    QRect contentRectForVisualRect(const QRect &cellRect) const;

    bool m_suppressRubberBandUntilRelease = false;
};

#endif
