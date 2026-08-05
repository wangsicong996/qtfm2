#ifndef SEARCHHIGHLIGHT_H
#define SEARCHHIGHLIGHT_H

#include <QAbstractItemView>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QString>
#include <QTextLayout>
#include <QTextLine>
#include <QTextOption>

#include "sortmodel.h"

/** Soft blue background for the matched substring in file names. */
inline QColor searchMatchBackground()
{
    return QColor(64, 156, 255);
}

inline QString nameSearchNeedleFromView(const QStyleOptionViewItem &option)
{
    const auto *view = qobject_cast<const QAbstractItemView *>(option.widget);
    if (!view) {
        return QString();
    }
    const auto *proxy = qobject_cast<const viewsSortProxyModel *>(view->model());
    if (!proxy) {
        return QString();
    }
    return proxy->nameSearchFilter();
}

/**
 * Shift the display name so the first case-insensitive match starts at index 0
 * (hide the prefix, like a marquee scroll), making the hit visible on line 1.
 * Returns the visible string; matchLenOut receives the highlighted length at start.
 */
inline QString searchRevealDisplayName(const QString &text,
                                       const QString &needle,
                                       int *matchLenOut = nullptr)
{
    if (matchLenOut) {
        *matchLenOut = 0;
    }
    if (text.isEmpty() || needle.isEmpty()) {
        return text;
    }
    const int idx = text.indexOf(needle, 0, Qt::CaseInsensitive);
    if (idx < 0) {
        return text;
    }
    if (matchLenOut) {
        *matchLenOut = needle.size();
    }
    // Marquee: drop the left prefix so the match opens the label.
    return text.mid(idx);
}

inline void fillMatchBackground(QPainter *painter,
                                int x,
                                int textY,
                                int width,
                                const QFontMetrics &fm)
{
    if (width <= 0) {
        return;
    }
    painter->fillRect(QRect(x, textY - fm.ascent(), width, fm.height()),
                      searchMatchBackground());
}

/**
 * Draw single-line text with search reveal + blue highlight on the match
 * (always at the start of the visible string after shifting).
 */
inline void drawTextWithSearchHighlight(QPainter *painter,
                                        const QRect &rect,
                                        const QString &text,
                                        const QString &needle,
                                        const QFont &font,
                                        const QColor &fg,
                                        int alignFlags)
{
    if (!painter || text.isEmpty()) {
        return;
    }

    painter->setFont(font);
    const QFontMetrics fm(font);

    int matchLen = 0;
    QString display = searchRevealDisplayName(text, needle, &matchLen);

    if (matchLen <= 0 || needle.isEmpty()) {
        painter->setPen(fg);
        painter->drawText(rect, alignFlags, fm.elidedText(display, Qt::ElideRight, rect.width()));
        return;
    }

    // Keep match at the start; elide only the trailing part if needed.
    if (fm.horizontalAdvance(display) > rect.width()) {
        display = fm.elidedText(display, Qt::ElideRight, rect.width());
        matchLen = qMin(matchLen, display.size());
    }

    // Match is always at the start of `display` after reveal shift.
    const QString match = display.left(matchLen);
    const QString after = display.mid(matchLen);

    const int matchW = fm.horizontalAdvance(match);
    const int afterW = fm.horizontalAdvance(after);
    const int totalW = matchW + afterW;

    int x = rect.left();
    if (alignFlags & Qt::AlignHCenter) {
        x = rect.left() + qMax(0, (rect.width() - totalW) / 2);
    } else if (alignFlags & Qt::AlignRight) {
        x = rect.right() - totalW;
    }

    int textY;
    if (alignFlags & Qt::AlignTop) {
        textY = rect.top() + fm.ascent();
    } else if (alignFlags & Qt::AlignBottom) {
        textY = rect.bottom() - fm.descent();
    } else {
        textY = rect.top() + (rect.height() + fm.ascent() - fm.descent()) / 2;
    }

    fillMatchBackground(painter, x, textY, matchW, fm);

    painter->setPen(Qt::white);
    if (!match.isEmpty()) {
        painter->drawText(x, textY, match);
    }
    painter->setPen(fg);
    if (!after.isEmpty()) {
        painter->drawText(x + matchW, textY, after);
    }
}

/**
 * Icon-view two-line label: reveal-shift so the match starts on line 1,
 * then wrap like the normal two-line name layout, with blue highlight on the match.
 */
inline void drawTwoLineSearchHighlightedName(QPainter *painter,
                                             const QRect &rect,
                                             const QString &text,
                                             const QString &needle,
                                             const QFont &font,
                                             const QColor &fg)
{
    if (!painter || text.isEmpty()) {
        return;
    }

    int matchLen = 0;
    const QString display = searchRevealDisplayName(text, needle, &matchLen);
    if (matchLen <= 0) {
        // Fallback: plain two-line without highlight (caller usually handles this).
        painter->setPen(fg);
        painter->setFont(font);
        const QFontMetrics fm(font);
        painter->drawText(rect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, display);
        return;
    }

    painter->setFont(font);
    const QFontMetrics fm(font);
    const int lineHeight = fm.lineSpacing();

    QTextLayout layout(display, font);
    QTextOption opt;
    opt.setAlignment(Qt::AlignLeft);
    opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    layout.setTextOption(opt);

    layout.beginLayout();
    QTextLine line1 = layout.createLine();
    if (!line1.isValid()) {
        layout.endLayout();
        return;
    }
    line1.setLineWidth(rect.width());

    QTextLine line2 = layout.createLine();
    bool truncated = false;
    if (line2.isValid()) {
        line2.setLineWidth(rect.width());
        truncated = layout.createLine().isValid();
    }
    layout.endLayout();

    auto drawLineSegment = [&](qreal y, int start, int length, bool elideRest) {
        if (length <= 0 && !elideRest) {
            return;
        }
        QString chunk = display.mid(start, length);
        if (elideRest) {
            chunk = fm.elidedText(display.mid(start), Qt::ElideRight, rect.width());
        }

        // Highlight overlap of [start, start+chunk.size()) with [0, matchLen).
        const int chunkStart = start;
        const int visibleLen = chunk.size();
        const int hlFrom = qMax(0, 0 - chunkStart);
        const int hlTo = qMin(visibleLen, matchLen - chunkStart);

        int x = rect.left();
        // Center the line content within the label width.
        const int lineW = fm.horizontalAdvance(chunk);
        x = rect.left() + qMax(0, (rect.width() - lineW) / 2);

        const int textY = int(y) + fm.ascent();

        if (hlTo > hlFrom) {
            const QString beforeHl = chunk.left(hlFrom);
            const QString hlText = chunk.mid(hlFrom, hlTo - hlFrom);
            const QString afterHl = chunk.mid(hlTo);
            const int beforeW = fm.horizontalAdvance(beforeHl);
            const int hlW = fm.horizontalAdvance(hlText);

            fillMatchBackground(painter, x + beforeW, textY, hlW, fm);

            painter->setPen(fg);
            if (!beforeHl.isEmpty()) {
                painter->drawText(x, textY, beforeHl);
            }
            painter->setPen(Qt::white);
            if (!hlText.isEmpty()) {
                painter->drawText(x + beforeW, textY, hlText);
            }
            painter->setPen(fg);
            if (!afterHl.isEmpty()) {
                painter->drawText(x + beforeW + hlW, textY, afterHl);
            }
        } else {
            painter->setPen(fg);
            painter->drawText(x, textY, chunk);
        }
    };

    qreal y = rect.top();
    drawLineSegment(y, line1.textStart(), line1.textLength(), false);
    y += lineHeight;

    if (!line2.isValid()) {
        return;
    }
    if (truncated) {
        drawLineSegment(y, line2.textStart(), -1, true);
    } else {
        drawLineSegment(y, line2.textStart(), line2.textLength(), false);
    }
}

#endif // SEARCHHIGHLIGHT_H
