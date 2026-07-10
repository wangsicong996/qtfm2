#ifndef THUMBDIAG_H
#define THUMBDIAG_H

#include <QByteArray>
#include <QString>
#include <QStringList>

/** Thumbnail pipeline logging (written as Warning: [Thumb] … to diagnostic log). */
namespace ThumbDiag {

void setLoggingEnabled(bool enabled);
bool loggingEnabled();

void info(const QString &message);
void warn(const QString &message);

void command(const QString &program, const QStringList &arguments, int exitCode,
             const QByteArray &standardError, const QByteArray &standardOutput = QByteArray());

} // namespace ThumbDiag

#endif
