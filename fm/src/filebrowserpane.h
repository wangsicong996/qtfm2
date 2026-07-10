#ifndef FILEBROWSERPANE_H
#define FILEBROWSERPANE_H

#include "dfmqtreeview.h"
#include "iconfilelistview.h"
#include "sortmodel.h"

#include <QItemSelectionModel>
#include <QStackedWidget>
#include <QStringList>
#include <QWidget>

class IconListDelegate;
class IconViewDelegate;
class myModel;

/** One file browsing column (icon or list view) with its own scroll position and nav history. */
class FileBrowserPane : public QWidget {
    Q_OBJECT
public:
    explicit FileBrowserPane(int paneIndex, QWidget *parent = nullptr);

    int paneIndex() const { return m_paneIndex; }

    QStackedWidget *stackWidget() const { return m_stack; }
    IconFileListView *listView() const { return m_list; }
    DfmQTreeView *detailTree() const { return m_detailTree; }
    viewsSortProxyModel *proxyModel() const { return m_proxy; }
    QItemSelectionModel *selectionModel() const { return m_selectionModel; }

    QString currentPath() const { return m_currentPath; }
    void setCurrentPath(const QString &path) { m_currentPath = path; }

    QStringList pathHistory() const { return m_pathHistory; }
    void setPathHistory(const QStringList &history) { m_pathHistory = history; }

    QStringList forwardStack() const { return m_forward; }
    void setForwardStack(const QStringList &stack) { m_forward = stack; }
    void clearForward() { m_forward.clear(); }

    void setupViews(myModel *model, IconViewDelegate *iconDelegate, IconListDelegate *listDelegate);

    void setViewStackIndex(int index);
    void setRootIndex(const QModelIndex &proxyIndex);
    void applyChromeTint(const QColor &background);

signals:
    void paneActivated(int paneIndex);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    int m_paneIndex;
    QStackedWidget *m_stack = nullptr;
    IconFileListView *m_list = nullptr;
    DfmQTreeView *m_detailTree = nullptr;
    viewsSortProxyModel *m_proxy = nullptr;
    QItemSelectionModel *m_selectionModel = nullptr;

    QString m_currentPath;
    QStringList m_pathHistory;
    QStringList m_forward;
};

#endif
