#pragma once

#include <QWidget>
#include <QTimer>
#include <QPixmap>
#include <vector>

class QPainter;
class QWheelEvent;
class QMouseEvent;
class QResizeEvent;
class QGraphicsOpacityEffect;
class ToggleSwitch;

class ZoomBackground : public QWidget
{
    Q_OBJECT

public:
    explicit ZoomBackground(const QString &mapImagePath,
                            int pointCount = 5000,
                            QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    struct Point3D { double x, y, z; };

    void drawSphere(QPainter &painter);
    void drawMap(QPainter &painter);
    void drawWelcome(QPainter &painter);
    void setInverted(bool on);
    double mapAmount() const;   // 0 = sphère seule, 1 = carte seule

    std::vector<Point3D> m_points;   // points sur la sphère unitaire
    QPixmap m_map;                   // carte en binaire (normale)
    QPixmap m_mapInverted;           // carte en binaire (couleurs inversées)
    QTimer m_timer;

    ToggleSwitch *m_toggle = nullptr;          // interrupteur Light / Dark
    QGraphicsOpacityEffect *m_toggleFx = nullptr;

    double m_angle = 0.0;            // angle de rotation courant
    double m_speed = 0.003;          // vitesse de rotation
    double m_zoom  = 1.0;            // facteur de zoom (1.0 = taille de départ)
    double m_flyProgress = 0.0;      // 0 = texte en place, 1 = texte disparu
    bool m_flying = false;           // le texte est-il en train de disparaître ?
    bool m_inverted = false;         // mode couleurs inversées
};