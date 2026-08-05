#ifndef SEARCHHIGHLIGHT_H
#define SEARCHHIGHLIGHT_H

#include <QAbstractItemView>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QString>

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
    // viewsSortProxyModel has no Q_OBJECT — use dynamic_cast, not qobject_cast.
    const auto *proxy = dynamic_cast<const viewsSortProxyModel *>(view->model());
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
 * then pack by character width (no wrap-at-hyphen), with blue highlight on the match.
 */
inline void drawTwoLineSearchHighlightedName(QPainter *painter,
                                             const QRect &rect,
                                             const QString &text,
                                             const QString &needle,
                                             const QFont &font,
                                             const QColor &fg)
{
    if (!painter || text.isEmpty() || rect.width() <= 0) {
        return;
    }

    int matchLen = 0;
    const QString display = searchRevealDisplayName(text, needle, &matchLen);
    if (matchLen <= 0) {
        painter->setPen(fg);
        painter->setFont(font);
        const QFontMetrics fm(font);
        // Character-packed two-line (same as normal icon labels).
        const int maxW = rect.width();
        int line1Count = display.size();
        if (fm.horizontalAdvance(display) > maxW) {
            int lo = 0;
            int hi = display.size();
            while (lo < hi) {
                const int mid = (lo + hi + 1) / 2;
                if (fm.horizontalAdvance(display.left(mid)) <= maxW) {
                    lo = mid;
                } else {
                    hi = mid - 1;
                }
            }
            line1Count = qMax(1, lo);
        }
        if (line1Count >= display.size()) {
            painter->drawText(rect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextSingleLine,
                              fm.elidedText(display, Qt::ElideMiddle, maxW));
            return;
        }
        const QString line1 = display.left(line1Count);
        const QString line2 = fm.elidedText(display.mid(line1Count), Qt::ElideRight, maxW);
        const int y1 = rect.top() + fm.ascent();
        const int y2 = rect.top() + fm.lineSpacing() + fm.ascent();
        painter->drawText(rect.left() + qMax(0, (maxW - fm.horizontalAdvance(line1)) / 2), y1, line1);
        painter->drawText(rect.left() + qMax(0, (maxW - fm.horizontalAdvance(line2)) / 2), y2, line2);
        return;
    }

    painter->setFont(font);
    const QFontMetrics fm(font);
    const int lineHeight = fm.lineSpacing();
    const int maxW = rect.width();

    int line1Count = display.size();
    if (fm.horizontalAdvance(display) > maxW) {
        int lo = 0;
        int hi = display.size();
        while (lo < hi) {
            const int mid = (lo + hi + 1) / 2;
            if (fm.horizontalAdvance(display.left(mid)) <= maxW) {
                lo = mid;
            } else {
                hi = mid - 1;
            }
        }
        line1Count = qMax(1, lo);
    }

    auto drawLineSegment = [&](qreal y, int start, int length, bool elideRest) {
        if (length <= 0 && !elideRest) {
            return;
        }
        QString chunk = display.mid(start, length);
        if (elideRest) {
            chunk = fm.elidedText(display.mid(start), Qt::ElideRight, maxW);
        }

        const int chunkStart = start;
        const int visibleLen = chunk.size();
        const int hlFrom = qMax(0, 0 - chunkStart);
        const int hlTo = qMin(visibleLen, matchLen - chunkStart);

        int x = rect.left() + qMax(0, (maxW - fm.horizontalAdvance(chunk)) / 2);
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
    if (line1Count >= display.size()) {
        drawLineSegment(y, 0, display.size(), false);
        return;
    }
    drawLineSegment(y, 0, line1Count, false);
    y += lineHeight;
    drawLineSegment(y, line1Count, -1, true);
}

#endif // SEARCHHIGHLIGHT_H
