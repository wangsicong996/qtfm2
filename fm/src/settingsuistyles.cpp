#include "settingsuistyles.h"

#include <QApplication>
#include <QColor>
#include <QLineEdit>
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

/** Darken a module card background by N steps (~8% each). */
QColor deepenModuleBg(const QColor &base, int steps)
{
    QColor c = base;
    for (int i = 0; i < steps; ++i) {
        c = c.darker(108);
    }
    return c;
}

QString lineEditRules(const QString &frameSelector, const QColor &inputBg,
                      const QColor &border, const QColor &text)
{
    return QStringLiteral(
        "%1 QLineEdit {"
        " background: %2; color: %3;"
        " border: 1px solid %4; border-radius: 4px; padding: 4px 6px;"
        " selection-background-color: palette(highlight);"
        " selection-color: palette(highlighted-text); }"
        "%1 QLineEdit:focus {"
        " border: 1px solid palette(highlight); }")
        .arg(frameSelector, colorCss(inputBg), colorCss(text), colorCss(border));
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
    const QColor moduleBg1 = deepenModuleBg(moduleBg, 1);
    const QColor moduleBg2 = deepenModuleBg(moduleBg, 2);
    const QColor inputBg = inputBackgroundForModule(moduleBg);
    const QColor inputBg1 = inputBackgroundForModule(moduleBg1);
    const QColor inputBg2 = inputBackgroundForModule(moduleBg2);
    const QColor border = pal.color(QPalette::Mid);
    const QColor text = pal.color(QPalette::Text);

    QString sheet = QStringLiteral(
        "QFrame#settingsModule {"
        " background: %1; border: 1px solid %2;"
        " border-radius: 10px; padding: 8px; }"
        "QFrame#settingsModuleSub1 {"
        " background: %3; border: 1px solid %2;"
        " border-radius: 10px; padding: 8px; }"
        "QFrame#settingsModuleSub2 {"
        " background: %4; border: 1px solid %2;"
        " border-radius: 10px; padding: 8px; }")
        .arg(colorCss(moduleBg), colorCss(border),
             colorCss(moduleBg1), colorCss(moduleBg2));

    sheet += lineEditRules(QStringLiteral("QFrame#settingsModule"),
                           inputBg, border, text);
    sheet += lineEditRules(QStringLiteral("QFrame#settingsModuleSub1"),
                           inputBg1, border, text);
    sheet += lineEditRules(QStringLiteral("QFrame#settingsModuleSub2"),
                           inputBg2, border, text);

    sheet += QStringLiteral(
        "QFrame#settingsCategoryBox { background: palette(alternate-base);"
        " border: 1px solid palette(dark); border-radius: 12px; padding: 8px; }"
        "QFrame#settingsCategoryBox QLineEdit {"
        " background: %1; color: %2;"
        " border: 1px solid %3; border-radius: 4px; padding: 4px 6px; }"
        "QPushButton#settingsAddBtn { background-color: #2da44e; color: #ffffff;"
        " border: 1px solid #1a7f37; border-radius: 6px; padding: 6px 14px; font-weight: 600; }"
        "QPushButton#settingsAddBtn:hover { background-color: #2c974b; }"
        "QPushButton#settingsAddBtn:pressed { background-color: #1a7f37; }"
        "QPushButton#settingsDeleteBtn { background-color: #cf222e; color: #ffffff;"
        " border: 1px solid #a40e26; border-radius: 6px; padding: 6px 14px; font-weight: 600; }"
        "QPushButton#settingsDeleteBtn:hover { background-color: #bc1c2c; }"
        "QPushButton#settingsDeleteBtn:pressed { background-color: #a40e26; }")
        .arg(colorCss(inputBg), colorCss(text), colorCss(border));

    return sheet;
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

void stylePlaceholderHint(QLineEdit *edit)
{
    if (!edit) {
        return;
    }
    // Match disabled menu text: light ≈ #CCC, dark ≈ #333.
    const QPalette appPal = QApplication::palette();
    QColor hint = appPal.color(QPalette::Disabled, QPalette::Text);
    if (!hint.isValid()) {
        const QColor text = appPal.color(QPalette::Text);
        hint = (text.lightness() > 128) ? QColor(0x33, 0x33, 0x33)
                                        : QColor(0xCC, 0xCC, 0xCC);
    }
    QPalette pal = edit->palette();
    pal.setColor(QPalette::PlaceholderText, hint);
    edit->setPalette(pal);
}

} // namespace SettingsUiStyles
