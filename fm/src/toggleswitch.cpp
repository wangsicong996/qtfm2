#include "toggleswitch.h"

#include <QEasingCurve>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>

ToggleSwitch::ToggleSwitch(QWidget *parent)
    : QAbstractButton(parent)
{
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_anim = new QPropertyAnimation(this, "knobPosition", this);
    m_anim->setDuration(160);
    m_anim->setEasingCurve(QEasingCurve::InOutCubic);

    connect(this, &QAbstractButton::toggled, this, [this](bool on) {
        animateToState(on);
    });
}

QSize ToggleSwitch::sizeHint() const
{
    return QSize(42, 26);
}

void ToggleSwitch::setKnobPosition(qreal pos)
{
    m_knobPos = qBound(0.0, pos, 1.0);
    update();
}

qreal ToggleSwitch::trackWidth() const
{
    return qMax(36.0, width() - 2.0);
}

qreal ToggleSwitch::trackHeight() const
{
    return qMax(20.0, height() - 4.0);
}

qreal ToggleSwitch::knobDiameter() const
{
    return trackHeight() - 4.0;
}

void ToggleSwitch::animateToState(bool on)
{
    m_anim->stop();
    m_anim->setStartValue(m_knobPos);
    m_anim->setEndValue(on ? 1.0 : 0.0);
    m_anim->start();
}

void ToggleSwitch::nextCheckState()
{
    QAbstractButton::nextCheckState();
}

void ToggleSwitch::checkStateSet()
{
    QAbstractButton::checkStateSet();
    if (m_anim && m_anim->state() == QAbstractAnimation::Running) {
        return;
    }
    m_knobPos = isChecked() ? 1.0 : 0.0;
    update();
}

void ToggleSwitch::resizeEvent(QResizeEvent *event)
{
    QAbstractButton::resizeEvent(event);
    update();
}

void ToggleSwitch::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const qreal tw = trackWidth();
    const qreal th = trackHeight();
    const qreal kd = knobDiameter();
    const qreal trackX = (width() - tw) / 2.0;
    const qreal trackY = (height() - th) / 2.0;
    const QRectF track(trackX, trackY, tw, th);

    const QColor offTrack(189, 189, 194);
    const QColor onTrack(52, 199, 89); // iOS green
    QColor trackColor = offTrack;
    {
        const int r = int(offTrack.red() + (onTrack.red() - offTrack.red()) * m_knobPos);
        const int g = int(offTrack.green() + (onTrack.green() - offTrack.green()) * m_knobPos);
        const int b = int(offTrack.blue() + (onTrack.blue() - offTrack.blue()) * m_knobPos);
        trackColor = QColor(r, g, b);
    }
    if (!isEnabled()) {
        trackColor = trackColor.lighter(120);
    }

    QPainterPath trackPath;
    trackPath.addRoundedRect(track, th / 2.0, th / 2.0);
    p.fillPath(trackPath, trackColor);

    const qreal margin = 2.0;
    const qreal travel = tw - kd - margin * 2.0;
    const qreal knobX = trackX + margin + travel * m_knobPos;
    const qreal knobY = trackY + (th - kd) / 2.0;
    const QRectF knob(knobX, knobY, kd, kd);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 28));
    p.drawEllipse(knob.translated(0, 0.8));
    p.setBrush(Qt::white);
    p.drawEllipse(knob);

    if (hasFocus()) {
        QPen pen(QColor(0, 122, 255, 140));
        pen.setWidthF(1.5);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(track.adjusted(-1.5, -1.5, 1.5, 1.5), th / 2.0 + 1.5, th / 2.0 + 1.5);
    }
}
