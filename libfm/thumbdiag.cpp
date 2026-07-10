#include "thumbdiag.h"

#include <QMutex>
#include <QMutexLocker>
#include <QtGlobal>

namespace ThumbDiag {

namespace {

QMutex *logMutex()
{
    static QMutex m;
    return &m;
}

bool &loggingFlag()
{
    static bool enabled = true;
    return enabled;
}

QString formatCommandLine(const QString &program, const QStringList &arguments)
{
    QStringList parts;
    parts << program;
    for (const QString &arg : arguments) {
        if (arg.contains(QLatin1Char(' ')) || arg.contains(QLatin1Char('"'))) {
            parts << QStringLiteral("\"%1\"").arg(QString(arg).replace(QLatin1Char('"'),
                                                                       QStringLiteral("\\\"")));
        } else {
            parts << arg;
        }
    }
    return parts.join(QLatin1Char(' '));
}

void writeLine(const QByteArray &line)
{
    if (!loggingFlag()) {
        return;
    }
    QMutexLocker lock(logMutex());
    qWarning("%s", line.constData());
}

} // namespace

void setLoggingEnabled(bool enabled)
{
    loggingFlag() = enabled;
}

bool loggingEnabled()
{
    return loggingFlag();
}

void info(const QString &message)
{
    if (!loggingFlag() || message.isEmpty()) {
        return;
    }
    writeLine(QByteArray("[Thumb] ") + message.toUtf8());
}

void warn(const QString &message)
{
    if (!loggingFlag() || message.isEmpty()) {
        return;
    }
    writeLine(QByteArray("[Thumb] WARN ") + message.toUtf8());
}

void command(const QString &program, const QStringList &arguments, int exitCode,
             const QByteArray &standardError, const QByteArray &standardOutput)
{
    if (!loggingFlag()) {
        return;
    }
    info(QStringLiteral("cmd exit=%1: %2")
             .arg(exitCode)
             .arg(formatCommandLine(program, arguments)));
    if (!standardOutput.isEmpty()) {
        const QByteArray out = standardOutput.left(4000);
        writeLine(QByteArray("[Thumb] stdout: ") + out);
    }
    if (!standardError.isEmpty()) {
        const QByteArray err = standardError.left(4000);
        writeLine(QByteArray("[Thumb] stderr: ") + err);
    }
}

} // namespace ThumbDiag
