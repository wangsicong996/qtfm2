#include "settingsuistyles.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include "bundledicons.h"

namespace SettingsUiStyles {

QString moduleStyleSheet()
{
    return QStringLiteral(
        "QFrame#settingsModule { background: palette(base); border: 1px solid palette(mid);"
        " border-radius: 10px; padding: 8px; }"
        "QFrame#settingsCategoryBox { background: palette(alternate-base);"
        " border: 1px solid palette(dark); border-radius: 12px; padding: 8px; }"
        "QPushButton#settingsAddBtn { background-color: #2da44e; color: #ffffff;"
        " border: 1px solid #1a7f37; border-radius: 6px; padding: 6px 14px; font-weight: 600; }"
        "QPushButton#settingsAddBtn:hover { background-color: #2c974b; }"
        "QPushButton#settingsAddBtn:pressed { background-color: #1a7f37; }"
        "QPushButton#settingsDeleteBtn { background-color: #cf222e; color: #ffffff;"
        " border: 1px solid #a40e26; border-radius: 6px; padding: 6px 14px; font-weight: 600; }"
        "QPushButton#settingsDeleteBtn:hover { background-color: #bc1c2c; }"
        "QPushButton#settingsDeleteBtn:pressed { background-color: #a40e26; }");
}

void styleAddButton(QPushButton *button)
{
    if (!button) {
        return;
    }
    button->setObjectName(QStringLiteral("settingsAddBtn"));
    button->setCursor(Qt::PointingHandCursor);
}

void styleDeleteButton(QPushButton *button)
{
    if (!button) {
        return;
    }
    button->setObjectName(QStringLiteral("settingsDeleteBtn"));
    button->setCursor(Qt::PointingHandCursor);
}

void styleSaveCancelDialogButtons(QDialogButtonBox *box)
{
    if (!box) {
        return;
    }
    const QIcon saveIcon = BundledIcons::settingsIcon(QStringLiteral("save"));
    const QIcon cancelIcon = BundledIcons::settingsIcon(QStringLiteral("cancel"));
    if (QPushButton *saveBtn = box->button(QDialogButtonBox::Save)) {
        if (!saveIcon.isNull()) {
            saveBtn->setIcon(saveIcon);
        }
    }
    if (QPushButton *cancelBtn = box->button(QDialogButtonBox::Cancel)) {
        if (!cancelIcon.isNull()) {
            cancelBtn->setIcon(cancelIcon);
        }
    }
}

} // namespace SettingsUiStyles
