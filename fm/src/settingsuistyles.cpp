#include "settingsuistyles.h"

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QPushButton>

namespace SettingsUiStyles {

namespace {

QString colorCss(const QColor &c)
{
    return c.name(QColor::HexRgb);
}

/** Input fields sit ~20% away from the module panel background. */
QColor inputBackgroundForModule(const QColor &moduleBg)
{
    if (moduleBg.lightness() < 128) {
        return moduleBg.lighter(120); // ~20% lighter on dark panels
    }
    return moduleBg.darker(120); // ~20% darker on light panels
}

} // namespace

QString moduleStyleSheet()
{
    const QPalette pal = QApplication::palette();
    // Module panel: slightly lifted from the page so cards read as units.
    QColor moduleBg = pal.color(QPalette::AlternateBase);
    if (!moduleBg.isValid()) {
        moduleBg = pal.color(QPalette::Base);
    }
    const QColor inputBg = inputBackgroundForModule(moduleBg);
    const QColor border = pal.color(QPalette::Mid);
    const QColor text = pal.color(QPalette::Text);

    return QStringLiteral(
        "QFrame#settingsModule {"
        " background: %1; border: 1px solid %2;"
        " border-radius: 10px; padding: 8px; }"
        "QFrame#settingsModule QLineEdit {"
        " background: %3; color: %4;"
        " border: 1px solid %2; border-radius: 4px; padding: 4px 6px;"
        " selection-background-color: palette(highlight);"
        " selection-color: palette(highlighted-text); }"
        "QFrame#settingsModule QLineEdit:focus {"
        " border: 1px solid palette(highlight); }"
        "QFrame#settingsCategoryBox { background: palette(alternate-base);"
        " border: 1px solid palette(dark); border-radius: 12px; padding: 8px; }"
        "QFrame#settingsCategoryBox QLineEdit {"
        " background: %3; color: %4;"
        " border: 1px solid %2; border-radius: 4px; padding: 4px 6px; }"
        "QPushButton#settingsAddBtn { background-color: #2da44e; color: #ffffff;"
        " border: 1px solid #1a7f37; border-radius: 6px; padding: 6px 14px; font-weight: 600; }"
        "QPushButton#settingsAddBtn:hover { background-color: #2c974b; }"
        "QPushButton#settingsAddBtn:pressed { background-color: #1a7f37; }"
        "QPushButton#settingsDeleteBtn { background-color: #cf222e; color: #ffffff;"
        " border: 1px solid #a40e26; border-radius: 6px; padding: 6px 14px; font-weight: 600; }"
        "QPushButton#settingsDeleteBtn:hover { background-color: #bc1c2c; }"
        "QPushButton#settingsDeleteBtn:pressed { background-color: #a40e26; }")
        .arg(colorCss(moduleBg), colorCss(border), colorCss(inputBg), colorCss(text));
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

} // namespace SettingsUiStyles
