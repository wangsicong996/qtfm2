#ifndef ICONFILELISTVIEW_H
#define ICONFILELISTVIEW_H

#include <QListView>
#include <QModelIndex>
#include <QPoint>

class IconViewDelegate;

/**
 * Icon-mode file list: hit-testing matches icon+label chrome, not full grid cell.
 */
class IconFileListView : public QListView
{
    Q_OBJECT
public:
    explicit IconFileListView(QWidget *parent = nullptr);

    QModelIndex indexAt(const QPoint &point) const override;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QRect contentRectForVisualRect(const QRect &cellRect) const;
    void beginDeferredListViewPress(const QMouseEvent *event);

    bool m_plainLeftPressActive = false;
    QPoint m_plainLeftPressPos;
    QModelIndex m_plainLeftPressIndex;
};

#endif
