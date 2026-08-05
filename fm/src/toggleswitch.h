#ifndef TOGGLESWITCH_H
#define TOGGLESWITCH_H

#include <QAbstractButton>
#include <QPropertyAnimation>

/**
 * iOS-style sliding toggle. Only the switch chrome itself is clickable
 * (not a full-row label), so place labels beside it in the layout.
 */
class ToggleSwitch : public QAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(qreal knobPosition READ knobPosition WRITE setKnobPosition)

public:
    explicit ToggleSwitch(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    qreal knobPosition() const { return m_knobPos; }
    void setKnobPosition(qreal pos);

protected:
    void paintEvent(QPaintEvent *event) override;
    void nextCheckState() override;
    void checkStateSet() override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void animateToState(bool on);
    qreal trackWidth() const;
    qreal trackHeight() const;
    qreal knobDiameter() const;

    qreal m_knobPos = 0.0; // 0 = off, 1 = on
    QPropertyAnimation *m_anim = nullptr;
};

#endif // TOGGLESWITCH_H
