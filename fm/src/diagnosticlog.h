/****************************************************************************
* Diagnostic log file (persists across restarts for post-freeze inspection).
****************************************************************************/

#ifndef DIAGNOSTICLOG_H
#define DIAGNOSTICLOG_H

#include <QString>

class QWidget;

namespace DiagnosticLog {

QString filePath();
void openSession();
void appendLine(const QByteArray &line);

/** Read tail of log file (UTF-8), at most maxBytes from end. */
QString readLogText(qint64 maxBytes = 3 * 1024 * 1024);

/** Warning / Critical / Fatal lines with session markers. */
QString filteredIssueText(const QString &fullLog);

QString filteredThumbText(const QString &fullLog);

void showViewerDialog(QWidget *parent);

} // namespace DiagnosticLog

#endif
