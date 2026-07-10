#include "filebrowserpane.h"

#include "iconview.h"
#include "mymodel.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QPalette>
#include <QVBoxLayout>

FileBrowserPane::FileBrowserPane(int paneIndex, QWidget *parent)
    : QWidget(parent)
    , m_paneIndex(paneIndex)
{
    setAutoFillBackground(true);
    auto *lay = new QVBoxLayout(this);
    lay->setSpacing(0);
    lay->setContentsMargins(0, 0, 0, 0);

    m_stack = new QStackedWidget(this);
    lay->addWidget(m_stack);

    QWidget *iconPage = new QWidget(m_stack);
    auto *iconLay = new QHBoxLayout(iconPage);
    iconLay->setSpacing(0);
    iconLay->setContentsMargins(0, 0, 0, 0);
    m_list = new IconFileListView(iconPage);
    iconLay->addWidget(m_list);
    m_stack->addWidget(iconPage);

    QWidget *listPage = new QWidget(m_stack);
    auto *listLay = new QHBoxLayout(listPage);
    listLay->setSpacing(0);
    listLay->setContentsMargins(0, 0, 0, 0);
    m_detailTree = new DfmQTreeView(listPage);
    listLay->addWidget(m_detailTree);
    m_stack->addWidget(listPage);

    m_list->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_detailTree->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_detailTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void FileBrowserPane::setupViews(myModel *model, IconViewDelegate *iconDelegate,
                                 IconListDelegate *listDelegate)
{
    Q_UNUSED(listDelegate)
    m_proxy = new viewsSortProxyModel(this);
    m_proxy->setSourceModel(model);
    m_proxy->setSortCaseSensitivity(Qt::CaseInsensitive);

    m_list->setWrapping(true);
    m_list->setWordWrap(true);
    m_list->setModel(m_proxy);
    m_list->setItemDelegate(iconDelegate);
    m_list->setTextElideMode(Qt::ElideNone);

    m_detailTree->setRootIsDecorated(false);
    m_detailTree->setItemsExpandable(false);
    m_detailTree->setUniformRowHeights(true);
    m_detailTree->setAlternatingRowColors(true);
    m_detailTree->setModel(m_proxy);
    m_detailTree->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    m_selectionModel = m_list->selectionModel();
    m_detailTree->setSelectionModel(m_selectionModel);

    m_list->installEventFilter(this);
    m_detailTree->installEventFilter(this);
    m_list->viewport()->installEventFilter(this);
    m_detailTree->viewport()->installEventFilter(this);
}

void FileBrowserPane::setViewStackIndex(int index)
{
    if (m_stack) {
        m_stack->setCurrentIndex(index);
    }
}

void FileBrowserPane::setRootIndex(const QModelIndex &proxyIndex)
{
    if (!m_list || !m_detailTree) {
        return;
    }
    m_list->setRootIndex(proxyIndex);
    m_detailTree->setRootIndex(proxyIndex);
}

void FileBrowserPane::applyChromeTint(const QColor &background)
{
    if (m_list) {
        QPalette pal = m_list->palette();
        pal.setColor(QPalette::Base, background);
        pal.setColor(QPalette::Window, background);
        m_list->setPalette(pal);
        m_list->viewport()->setPalette(pal);
        m_list->setAutoFillBackground(true);
    }
    if (m_detailTree) {
        QPalette pal = m_detailTree->palette();
        pal.setColor(QPalette::Base, background);
        pal.setColor(QPalette::Window, background);
        m_detailTree->setPalette(pal);
        m_detailTree->viewport()->setPalette(pal);
        m_detailTree->setAutoFillBackground(true);
    }
    setAutoFillBackground(false);
}

bool FileBrowserPane::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::FocusIn) {
        Q_UNUSED(watched)
        emit paneActivated(m_paneIndex);
    }
    return QWidget::eventFilter(watched, event);
}
