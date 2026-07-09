#include "filerenamedialog.h"
#include "settingsuistyles.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>

FileRenameDialog::FileRenameDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Rename file"));
    setModal(true);
    resize(420, 200);

    auto *layout = new QVBoxLayout(this);
    auto *hint = new QLabel(tr("File name:"), this);
    layout->addWidget(hint);

    m_editor = new QPlainTextEdit(this);
    m_editor->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_editor->setTabChangesFocus(true);
    layout->addWidget(m_editor, 1);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    SettingsUiStyles::styleSaveCancelDialogButtons(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void FileRenameDialog::setFileName(const QString &name)
{
    if (m_editor) {
        m_editor->setPlainText(name);
        m_editor->selectAll();
    }
}

QString FileRenameDialog::fileName() const
{
    return m_editor ? m_editor->toPlainText() : QString();
}
