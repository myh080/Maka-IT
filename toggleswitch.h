#pragma once

#include <QAbstractButton>
#include <QTimer>

class ToggleSwitch : public QAbstractButton
{
    Q_OBJECT

public:
    explicit ToggleSwitch(QWidget *parent = nullptr);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void step();

    QTimer m_timer;          // pilote le glissement de la bulle
    double m_pos = 0.0;      // position actuelle : 0 = gauche, 1 = droite
    double m_target = 0.0;   // position visée
};