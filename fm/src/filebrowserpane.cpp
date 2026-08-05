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
    setObjectName(QStringLiteral("fileBrowserPane%1").arg(paneIndex));
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
    m_list->setObjectName(QStringLiteral("filePaneIconView%1").arg(paneIndex));
    iconLay->addWidget(m_list);
    m_stack->addWidget(iconPage);

    QWidget *listPage = new QWidget(m_stack);
    auto *listLay = new QHBoxLayout(listPage);
    listLay->setSpacing(0);
    listLay->setContentsMargins(0, 0, 0, 0);
    m_detailTree = new DfmQTreeView(listPage);
    m_detailTree->setObjectName(QStringLiteral("filePaneDetailView%1").arg(paneIndex));
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

void FileBrowserPane::setDetailItemStyleSheet(const QString &itemQss)
{
    m_detailItemQss = itemQss;
    applyChromeStyles();
}

void FileBrowserPane::applyChromeTint(const QColor &background)
{
    m_chromeBg = background;
    applyChromeStyles();
}

void FileBrowserPane::applyChromeStyles()
{
    const bool hasBg = m_chromeBg.isValid();
    QColor alt = m_chromeBg;
    if (hasBg) {
        const int lum = (m_chromeBg.red() * 299 + m_chromeBg.green() * 587
                         + m_chromeBg.blue() * 114) / 1000;
        alt = (lum < 128) ? m_chromeBg.lighter(112) : m_chromeBg.darker(106);
    }
    const QString bg = hasBg ? m_chromeBg.name(QColor::HexRgb) : QString();
    const QString altBg = hasBg ? alt.name(QColor::HexRgb) : QString();

    const auto applyPal = [hasBg, this, &alt](QWidget *w) {
        if (!w || !hasBg) {
            return;
        }
        QPalette pal = w->palette();
        pal.setColor(QPalette::Base, m_chromeBg);
        pal.setColor(QPalette::Window, m_chromeBg);
        pal.setColor(QPalette::AlternateBase, alt);
        w->setPalette(pal);
        w->setAutoFillBackground(true);
    };

    applyPal(this);
    if (m_stack) {
        applyPal(m_stack);
        for (int i = 0; i < m_stack->count(); ++i) {
            applyPal(m_stack->widget(i));
        }
    }

    if (m_list) {
        applyPal(m_list);
        if (m_list->viewport()) {
            applyPal(m_list->viewport());
        }
        // Explicit hex colors override MainWindow / system theme stylesheets.
        if (hasBg) {
            m_list->setStyleSheet(QStringLiteral(
                "QListView {"
                " background-color: %1;"
                " alternate-background-color: %2;"
                " border: none; }"
                "QListView::viewport { background-color: %1; }"
            ).arg(bg, altBg));
        }
        m_list->viewport()->update();
    }

    if (m_detailTree) {
        applyPal(m_detailTree);
        if (m_detailTree->viewport()) {
            applyPal(m_detailTree->viewport());
        }
        QString qss = m_detailItemQss;
        if (hasBg) {
            qss += QStringLiteral(
                "\nQTreeView {"
                " background-color: %1;"
                " alternate-background-color: %2;"
                " border: none; }"
                "QTreeView::viewport { background-color: %1; }"
            ).arg(bg, altBg);
        }
        m_detailTree->setStyleSheet(qss);
        m_detailTree->viewport()->update();
    }

    update();
}

bool FileBrowserPane::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::FocusIn) {
        Q_UNUSED(watched)
        emit paneActivated(m_paneIndex);
    }
    return QWidget::eventFilter(watched, event);
}
