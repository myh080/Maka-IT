#include "toggleswitch.h"

#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QFont>
#include <QEasingCurve>

ToggleSwitch::ToggleSwitch(QWidget *parent)
    : QAbstractButton(parent)
{
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFixedSize(sizeHint());

    m_anim.setDuration(450);
    m_anim.setEasingCurve(QEasingCurve::OutCubic);

    // Mise à jour de la position de la bulle pendant l'animation
    connect(&m_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_pos = v.toDouble();
        update();
    });

    // Glissement de la bulle à chaque clic
    connect(this, &QAbstractButton::toggled, this, [this](bool on) {
        m_anim.stop();
        m_anim.setStartValue(m_pos);
        m_anim.setEndValue(on ? 1.0 : 0.0);
        m_anim.start();
    });
}

QSize ToggleSwitch::sizeHint() const
{
    return QSize(110, 30);
}

void ToggleSwitch::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const QRectF full = QRectF(rect()).adjusted(1, 1, -1, -1);
    const double h = full.height();
    const double radius = h / 2.0;
    const double margin = h * 0.08;
    const double knobD = h - 2 * margin;

    // Capsule sombre, légèrement dégradée
    QLinearGradient bg(full.topLeft(), full.bottomLeft());
    bg.setColorAt(0.0, QColor(40, 40, 40));
    bg.setColorAt(1.0, QColor(12, 12, 12));
    p.setPen(QPen(QColor(255, 255, 255, 35), 1.2));
    p.setBrush(bg);
    p.drawRoundedRect(full, radius, radius);

    // Position de la bulle
    const double knobX = full.left() + margin
                         + m_pos * (full.width() - knobD - 2 * margin);
    const QRectF knob(knobX, full.top() + margin, knobD, knobD);

    // Texte affiché sur le côté libre de la bulle
    QFont font;
    font.setFamilies({ "Segoe UI Semibold", "Segoe UI", "Arial" });
    font.setPixelSize(static_cast<int>(h * 0.32));
    p.setFont(font);
    p.setPen(Qt::white);

    const bool darkSide = m_pos > 0.5;
    const QString text = darkSide ? "Dark" : "Light";
    const QRectF textRect = darkSide
                                ? QRectF(full.left(), full.top(), knob.left() - full.left(), full.height())
                                : QRectF(knob.right(), full.top(), full.right() - knob.right(), full.height());
    p.drawText(textRect, Qt::AlignCenter, text);

    // Bulle en verre, avec reflet
    QRadialGradient kg(knob.center() + QPointF(-knobD * 0.2, -knobD * 0.25), knobD * 0.7);
    kg.setColorAt(0.0, QColor(95, 95, 95));
    kg.setColorAt(0.6, QColor(45, 45, 45));
    kg.setColorAt(1.0, QColor(18, 18, 18));
    p.setPen(QPen(QColor(255, 255, 255, 70), 1.5));
    p.setBrush(kg);
    p.drawEllipse(knob);

    // Lune en croissant au centre de la bulle
    const double r = knobD * 0.26;
    const QPointF c = knob.center();

    QPainterPath moon;
    moon.addEllipse(c, r, r);

    QPainterPath cut;
    cut.addEllipse(c + QPointF(r * 0.45, -r * 0.35), r * 0.85, r * 0.85);

    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawPath(moon.subtracted(cut));
}