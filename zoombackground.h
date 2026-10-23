#pragma once

#include <QWidget>
#include <QTimer>
#include <QPixmap>
#include <vector>

class QPainter;
class QWheelEvent;
class QMouseEvent;

class ZoomBackground : public QWidget
{
    Q_OBJECT

public:
    explicit ZoomBackground(QWidget *parent = nullptr);

    void setMapImage(const QString &path);

public slots:
    void setInverted(bool on);

signals:
    // Émis au 4e cran (show = true) et quand on revient en arrière (show = false)
    void dashboardVisible(bool show);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    struct Point3D { double x, y, z; };

    void step();
    void setNotches(int notches);
    void drawSphere(QPainter &painter);
    void drawMap(QPainter &painter);
    void drawWelcome(QPainter &painter);

    std::vector<Point3D> m_points;
    QPixmap m_map;
    QPixmap m_mapInverted;
    QTimer m_timer;

    int m_notches = 0;          // nombre de crans de molette
    double m_zoom = 1.0;        // taille de la sphère
    double m_angle = 0.0;
    double m_speed = 0.003;
    double m_textAlpha = 1.0;   // opacité du texte de bienvenue
    double m_mapAlpha = 0.0;    // opacité de la carte (la sphère prend le reste)
    bool m_inverted = false;
    bool m_dashShown = false;
};