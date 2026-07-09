/****************************************************************************
* Diagnostic log file (persists across restarts for post-freeze inspection).
****************************************************************************/

#include "diagnosticlog.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QMutex>
#include <QMutexLocker>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QTextCursor>
#include <QTimer>
#include <QVBoxLayout>

namespace DiagnosticLog {

namespace {

QMutex *logMutex()
{
    static QMutex m;
    return &m;
}

QFile *logFileHandle()
{
    static QFile file;
    return &file;
}

const qint64 kMaxLogBytes = 5 * 1024 * 1024;
const qint64 kTruncateKeepBytes = 2 * 1024 * 1024;

void trimLogFileIfNeeded()
{
    const QString path = filePath();
    QFile f(path);
    if (!f.exists() || f.size() <= kMaxLogBytes) {
        return;
    }
    if (!f.open(QIODevice::ReadOnly)) {
        return;
    }
    const qint64 size = f.size();
    const qint64 skip = qMax<qint64>(0, size - kTruncateKeepBytes);
    if (!f.seek(skip)) {
        f.close();
        return;
    }
    QByteArray tail = f.readAll();
    f.close();
    const int nl = tail.indexOf('\n');
    if (nl >= 0) {
        tail = tail.mid(nl + 1);
    }
    const QByteArray banner = QByteArrayLiteral(
        "\n--- QtFM log truncated (kept last portion) "
        ) + QDateTime::currentDateTime().toString(Qt::ISODate).toUtf8() + "---\n";
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        f.write(banner);
        f.write(tail);
        f.flush();
    }
}

void ensureLogFileOpenUnlocked()
{
    QFile *file = logFileHandle();
    if (file->isOpen()) {
        return;
    }
    const QString path = filePath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    trimLogFileIfNeeded();
    file->setFileName(path);
    if (file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        file->write(QStringLiteral("\n--- QtFM log %1 ---\n")
                        .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
                        .toUtf8());
        file->flush();
    }
}

} // namespace

QString filePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    return dir + QStringLiteral("/qtfm.log");
}

void openSession()
{
    QMutexLocker lock(logMutex());
    ensureLogFileOpenUnlocked();
}

void appendLine(const QByteArray &line)
{
    QMutexLocker lock(logMutex());
    ensureLogFileOpenUnlocked();
    QFile *file = logFileHandle();
    if (!file->isOpen()) {
        return;
    }
    file->write(line);
    if (!line.endsWith('\n')) {
        file->write("\n");
    }
    file->flush();
}

QString readLogText(const qint64 maxBytes)
{
    QFile f(filePath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    const qint64 size = f.size();
    if (size > maxBytes) {
        f.seek(size - maxBytes);
        const QByteArray chunk = f.readAll();
        const int nl = chunk.indexOf('\n');
        const QByteArray trimmed = nl >= 0 ? chunk.mid(nl + 1) : chunk;
        return QString::fromUtf8(trimmed);
    }
    return QString::fromUtf8(f.readAll());
}

QString filteredIssueText(const QString &fullLog)
{
    const QStringList lines = fullLog.split(QLatin1Char('\n'));
    QStringList out;
    QString pendingSession;
    for (const QString &line : lines) {
        if (line.contains(QStringLiteral("--- QtFM log"))) {
            pendingSession = line;
            continue;
        }
        if (line.startsWith(QStringLiteral("Warning:"))
            || line.startsWith(QStringLiteral("Critical:"))
            || line.startsWith(QStringLiteral("Fatal:"))) {
            if (!pendingSession.isEmpty() && !out.contains(pendingSession)) {
                out.append(pendingSession);
            }
            out.append(line);
            pendingSession.clear();
        }
    }
    return out.join(QLatin1Char('\n'));
}

void showViewerDialog(QWidget *parent)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(QApplication::translate("DiagnosticLog", "Diagnostic log"));
    dlg.resize(720, 480);

    auto *layout = new QVBoxLayout(&dlg);

    auto *hint = new QLabel(
        QApplication::translate(
            "DiagnosticLog",
            "Logs are saved to disk continuously. If QtFM stopped responding, quit and "
            "restart, then open this window again to read messages from the previous session."),
        &dlg);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    const QString path = filePath();
    auto *pathLabel = new QLabel(
        QApplication::translate("DiagnosticLog", "Log file: %1").arg(path), &dlg);
    pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    pathLabel->setWordWrap(true);
    layout->addWidget(pathLabel);

    auto *mode = new QComboBox(&dlg);
    mode->addItem(QApplication::translate("DiagnosticLog", "Warnings and errors"),
                  QStringLiteral("issues"));
    mode->addItem(QApplication::translate("DiagnosticLog", "Full log"),
                  QStringLiteral("full"));
    layout->addWidget(mode);

    auto *editor = new QPlainTextEdit(&dlg);
    editor->setReadOnly(true);
    editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    {
        QFont mono = editor->font();
        mono.setStyleHint(QFont::Monospace);
        mono.setFamily(QStringLiteral("Monospace"));
        editor->setFont(mono);
    }
    layout->addWidget(editor, 1);

    const QString fullText = readLogText();

    auto reload = [&]() {
        const QString full = readLogText();
        const bool issuesOnly = mode->currentData().toString() == QLatin1String("issues");
        if (issuesOnly) {
            QString issues = filteredIssueText(full);
            if (issues.trimmed().isEmpty()) {
                issues = QApplication::translate(
                    "DiagnosticLog",
                    "(No warnings or errors in the log file yet.)\n\n"
                    "Switch to “Full log” to see all messages, or reproduce the problem "
                    "and refresh.");
            }
            editor->setPlainText(issues);
        } else {
            if (full.isEmpty()) {
                editor->setPlainText(QApplication::translate(
                    "DiagnosticLog", "(Log file is empty or not created yet.)"));
            } else {
                editor->setPlainText(full);
            }
        }
        editor->moveCursor(QTextCursor::End);
    };
    reload();
    QObject::connect(mode, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     [&](int) { reload(); });

    auto *buttons = new QHBoxLayout();
    auto *refreshBtn = new QPushButton(QApplication::translate("DiagnosticLog", "Refresh"), &dlg);
    auto *copyBtn = new QPushButton(
        QApplication::translate("DiagnosticLog", "Copy to clipboard"), &dlg);
    QObject::connect(refreshBtn, &QPushButton::clicked, [&](bool) { reload(); });
    QObject::connect(copyBtn, &QPushButton::clicked, [&](bool) {
        QApplication::clipboard()->setText(editor->toPlainText());
        copyBtn->setText(QApplication::translate("DiagnosticLog", "Copied"));
        QTimer::singleShot(2000, copyBtn, [copyBtn]() {
            copyBtn->setText(QApplication::translate("DiagnosticLog", "Copy to clipboard"));
        });
    });
    buttons->addWidget(refreshBtn);
    buttons->addWidget(copyBtn);
    buttons->addStretch();
    auto *closeBox = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    QObject::connect(closeBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    buttons->addWidget(closeBox);
    layout->addLayout(buttons);

    Q_UNUSED(fullText);
    dlg.exec();
}

} // namespace DiagnosticLog
