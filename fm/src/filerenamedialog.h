#ifndef FILERENAMEDIALOG_H
#define FILERENAMEDIALOG_H

#include <QDialog>

class QPlainTextEdit;
class QDialogButtonBox;

class FileRenameDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FileRenameDialog(QWidget *parent = nullptr);

    void setFileName(const QString &name);
    QString fileName() const;

private:
    QPlainTextEdit *m_editor = nullptr;
};

#endif
