#ifndef SETTINGSUISTYLES_H
#define SETTINGSUISTYLES_H

#include <QString>

class QPushButton;
class QDialogButtonBox;

namespace SettingsUiStyles {

QString moduleStyleSheet();
void styleAddButton(QPushButton *button);
void styleDeleteButton(QPushButton *button);
void styleSaveCancelDialogButtons(QDialogButtonBox *box);

} // namespace SettingsUiStyles

#endif
