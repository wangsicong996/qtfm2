#ifndef SETTINGSUISTYLES_H
#define SETTINGSUISTYLES_H

#include <QString>

class QPushButton;
class QLineEdit;

namespace SettingsUiStyles {

QString moduleStyleSheet();
void styleAddButton(QPushButton *button);
void styleDeleteButton(QPushButton *button);
/** Placeholder text ≈ disabled menu item gray. */
void stylePlaceholderHint(QLineEdit *edit);

} // namespace SettingsUiStyles

#endif
