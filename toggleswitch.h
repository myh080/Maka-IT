#ifndef TOGGLESWITCH_H
#define TOGGLESWITCH_H

#pragma once

#include <QAbstractButton>
#include <QVariantAnimation>

class ToggleSwitch : public QAbstractButton
{
    Q_OBJECT

public:
    explicit ToggleSwitch(QWidget *parent = nullptr);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVariantAnimation m_anim;
    double m_pos = 0.0;   // 0 = bulle à gauche, 1 = bulle à droite
};

#endif // TOGGLESWITCH_H
