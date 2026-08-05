#include "openwithsettingswidget.h"
#include "openwithconfig.h"
#include "settingsuistyles.h"

#include <QApplication>
#include <QClipboard>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QPalette>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QVector>

namespace {

void makeWidthFlexible(QWidget *w)
{
    if (!w) {
        return;
    }
    w->setMinimumWidth(0);
    QSizePolicy sp = w->sizePolicy();
    sp.setHorizontalPolicy(QSizePolicy::Expanding);
    w->setSizePolicy(sp);
}

QFrame *makeModuleFrame(QWidget *parent)
{
    auto *frame = new QFrame(parent);
    frame->setObjectName(QStringLiteral("settingsModule"));
    frame->setFrameShape(QFrame::StyledPanel);
    makeWidthFlexible(frame);
    return frame;
}

QFormLayout *makeFlexibleForm(QWidget *parent = nullptr)
{
    Q_UNUSED(parent)
    auto *form = new QFormLayout();
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    form->setHorizontalSpacing(10);
    form->setVerticalSpacing(8);
    return form;
}

} // namespace

void OpenWithSettingsWidget::fillEntryForm(QFormLayout *form, OpenWithEntry *entry,
                                           const QStringList &suffixesHint)
{
    // One command field in the UI; fold legacy line-2 into line-1 when present.
    if (!entry->commandLine2.trimmed().isEmpty()) {
        const QString extra = entry->commandLine2.trimmed();
        if (entry->commandLine1.trimmed().isEmpty()) {
            entry->commandLine1 = extra;
        } else {
            entry->commandLine1 = entry->commandLine1.trimmed() + QLatin1Char(' ') + extra;
        }
        entry->commandLine2.clear();
    }

    auto *nameEdit = new QLineEdit(entry->name);
    auto *cmdEdit = new QLineEdit(entry->commandLine1);
    auto *iconEdit = new QLineEdit(entry->iconPath);
    auto *pasteBtn = new QPushButton(tr("Paste"));
    makeWidthFlexible(nameEdit);
    makeWidthFlexible(cmdEdit);
    makeWidthFlexible(iconEdit);

    form->addRow(tr("Application name"), nameEdit);
    if (!suffixesHint.isEmpty()) {
        auto *extLabel = new QLabel(suffixesHint.join(QStringLiteral(", ")));
        extLabel->setWordWrap(true);
        extLabel->setMinimumWidth(0);
        extLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        form->addRow(tr("Extensions"), extLabel);
    }
    form->addRow(tr("Command"), cmdEdit);

    auto *iconRowWidget = new QWidget();
    makeWidthFlexible(iconRowWidget);
    auto *iconRow = new QHBoxLayout(iconRowWidget);
    iconRow->setContentsMargins(0, 0, 0, 0);
    iconRow->addWidget(iconEdit, 1);
    iconRow->addWidget(pasteBtn, 0);
    form->addRow(tr("Icon path"), iconRowWidget);

    connect(nameEdit, &QLineEdit::textChanged, [entry](const QString &t) {
        entry->name = t;
    });
    connect(cmdEdit, &QLineEdit::textChanged, [entry](const QString &t) {
        entry->commandLine1 = t;
        entry->commandLine2.clear();
    });
    connect(iconEdit, &QLineEdit::textChanged, [entry](const QString &t) {
        entry->iconPath = t;
    });
    connect(pasteBtn, &QPushButton::clicked, [iconEdit]() {
        const QMimeData *mime = QGuiApplication::clipboard()->mimeData();
        if (mime && mime->hasText()) {
            iconEdit->setText(mime->text().trimmed());
        }
    });
}

OpenWithSettingsWidget::OpenWithSettingsWidget(QWidget *parent) : QWidget(parent)
{
    setStyleSheet(SettingsUiStyles::moduleStyleSheet());
    setMinimumWidth(0);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setMinimumWidth(0);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    contentWidget = new QWidget(scrollArea);
    contentWidget->setMinimumWidth(0);
    contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto *mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setSpacing(10);
    // Keep category cards clear of the vertical scrollbar.
    mainLayout->setContentsMargins(0, 0, 10, 0);

    mainLayout->addWidget(buildSuffixSection());

    auto *catLabel = new QLabel(tr("Unified categories (first module = double-click default)"));
    catLabel->setWordWrap(true);
    catLabel->setMinimumWidth(0);
    mainLayout->addWidget(catLabel);
    mainLayout->addWidget(buildCategorySection(QStringLiteral("image")));
    mainLayout->addWidget(buildCategorySection(QStringLiteral("video")));
    mainLayout->addWidget(buildCategorySection(QStringLiteral("text")));
    mainLayout->addWidget(buildCategorySection(QStringLiteral("archive")));
    mainLayout->addStretch(1);

    scrollArea->setWidget(contentWidget);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
#ifdef Q_OS_MAC
    auto *hint = new QLabel(tr("These handlers override system defaults when set. "
                               "Use %f or %F for the file path. "
                               "On macOS, command-line tools installed with Homebrew (e.g. mpv %f, "
                               "/opt/homebrew/bin/mpv %f) are often more reliable than open -a for apps "
                               "that are not a .app bundle. "
                               "For real .app bundles use /Applications/App.app %f or open -a \"Exact App Name\" %f ."));
#else
    auto *hint = new QLabel(tr("These handlers override the \"Mime Types\" tab. "
                               "Use %f or %F for file path in commands."));
#endif
    hint->setWordWrap(true);
    hint->setMinimumWidth(0);
    outer->addWidget(hint);
    outer->addWidget(scrollArea, 1);
}

QWidget *OpenWithSettingsWidget::buildSuffixSection()
{
    auto *box = new QGroupBox(tr("Specific extensions"), contentWidget);
    makeWidthFlexible(box);
    auto *layout = new QVBoxLayout(box);

    auto *desc = new QLabel(tr("Each module applies to listed extensions (comma-separated), "
                                "e.g. pdf or glb,gltf. Higher priority than category rules."));
    desc->setWordWrap(true);
    desc->setMinimumWidth(0);
    layout->addWidget(desc);

    suffixModulesLayout = new QVBoxLayout();
    suffixModulesLayout->setSpacing(10);
    layout->addLayout(suffixModulesLayout);

    auto *addBtn = new QPushButton(tr("Add extension module"));
    SettingsUiStyles::styleAddButton(addBtn);
    connect(addBtn, &QPushButton::clicked, this, &OpenWithSettingsWidget::addSuffixModule);
    layout->addWidget(addBtn, 0, Qt::AlignLeft);
    return box;
}

QWidget *OpenWithSettingsWidget::buildCategorySection(const QString &categoryId)
{
    auto *outer = new QFrame(contentWidget);
    outer->setObjectName(QStringLiteral("settingsCategoryBox"));
    makeWidthFlexible(outer);
    auto *layout = new QVBoxLayout(outer);

    auto *title = new QLabel(
        tr("Category: %1")
            .arg(categoryId == QLatin1String("image") ? tr("Image")
                 : categoryId == QLatin1String("video") ? tr("Video")
                 : categoryId == QLatin1String("text") ? tr("Text and code")
                 : categoryId == QLatin1String("archive") ? tr("Archive")
                 : categoryId));
    title->setObjectName(QStringLiteral("settingsCategoryTitle"));
    {
        QFont titleFont = title->font();
        titleFont.setPointSize(titleFont.pointSize() + 3);
        title->setFont(titleFont);
        const QColor accent = QApplication::palette().color(QPalette::Highlight);
        title->setStyleSheet(QStringLiteral("color: %1;").arg(accent.name()));
    }
    title->setToolTip(OpenWithConfig::suffixesForCategory(categoryId).join(QStringLiteral(", ")));
    title->setWordWrap(true);
    title->setMinimumWidth(0);
    layout->addWidget(title);

    auto *modulesLayout = new QVBoxLayout();
    modulesLayout->setSpacing(10);
    categoryLayouts.insert(categoryId, modulesLayout);
    layout->addLayout(modulesLayout);

    auto *addBtn = new QPushButton(tr("Add application module"));
    SettingsUiStyles::styleAddButton(addBtn);
    connect(addBtn, &QPushButton::clicked, this, [this, categoryId]() {
        OpenWithConfig::categoryModules(categoryId).append(OpenWithEntry());
        loadFromConfig();
    });
    layout->addWidget(addBtn, 0, Qt::AlignLeft);
    return outer;
}

void OpenWithSettingsWidget::loadFromConfig()
{
    while (QLayoutItem *item = suffixModulesLayout->takeAt(0)) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    for (auto it = categoryLayouts.constBegin(); it != categoryLayouts.constEnd(); ++it) {
        QVBoxLayout *lay = it.value();
        while (QLayoutItem *item = lay->takeAt(0)) {
            if (item->widget()) {
                item->widget()->deleteLater();
            }
            delete item;
        }
    }

    QWidget *moduleParent = contentWidget ? contentWidget : this;

    for (int i = 0; i < OpenWithConfig::suffixModules().size(); ++i) {
        SuffixOpenModule *mod = &OpenWithConfig::suffixModules()[i];
        auto *frame = makeModuleFrame(moduleParent);
        auto *vlay = new QVBoxLayout(frame);
        auto *form = makeFlexibleForm();
        auto *suffixEdit = new QLineEdit(mod->suffixesText);
        makeWidthFlexible(suffixEdit);
        form->addRow(tr("Extensions"), suffixEdit);
        connect(suffixEdit, &QLineEdit::textChanged, [mod](const QString &t) {
            mod->suffixesText = t;
        });
        fillEntryForm(form, &mod->entry, QStringList());
        vlay->addLayout(form);
        const int moduleIndex = i;
        auto *del = new QPushButton(tr("Delete module"));
        SettingsUiStyles::styleDeleteButton(del);
        connect(del, &QPushButton::clicked, this, [this, moduleIndex]() {
            OpenWithConfig::suffixModules().removeAt(moduleIndex);
            loadFromConfig();
        });
        vlay->addWidget(del, 0, Qt::AlignRight);
        suffixModulesLayout->addWidget(frame);
    }

    for (const QString &cat : OpenWithConfig::categoryIds()) {
        QVBoxLayout *lay = categoryLayouts.value(cat);
        if (!lay) {
            continue;
        }
        QVector<OpenWithEntry> &list = OpenWithConfig::categoryModules(cat);
        for (int i = 0; i < list.size(); ++i) {
            auto *frame = makeModuleFrame(moduleParent);
            auto *vlay = new QVBoxLayout(frame);
            if (i == 0) {
                auto *defLabel = new QLabel(tr("Default for double-click (first module)"));
                defLabel->setWordWrap(true);
                defLabel->setMinimumWidth(0);
                vlay->addWidget(defLabel);
            }
            auto *form = makeFlexibleForm();
            fillEntryForm(form, &list[i], OpenWithConfig::suffixesForCategory(cat));
            vlay->addLayout(form);
            const int moduleIndex = i;
            auto *del = new QPushButton(tr("Delete module"));
            SettingsUiStyles::styleDeleteButton(del);
            connect(del, &QPushButton::clicked, this, [this, cat, moduleIndex]() {
                OpenWithConfig::categoryModules(cat).removeAt(moduleIndex);
                loadFromConfig();
            });
            vlay->addWidget(del, 0, Qt::AlignRight);
            lay->addWidget(frame);
        }
    }
}

void OpenWithSettingsWidget::saveToConfig()
{
    // Data already live in OpenWithConfig vectors via signal edits
}

void OpenWithSettingsWidget::addSuffixModule()
{
    SuffixOpenModule mod;
    OpenWithConfig::suffixModules().append(mod);
    loadFromConfig();
}
