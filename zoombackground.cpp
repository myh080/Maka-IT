#include "zoombackground.h"
#include "toggleswitch.h"

#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QImage>
#include <QRandomGenerator>
#include <QFont>
#include <QFontMetrics>
#include <QStringList>
#include <QGraphicsOpacityEffect>
#include <algorithm>
#include <cmath>

namespace {
constexpr double PI         = 3.14159265358979323846;
constexpr double MIN_ZOOM   = 1.0;   // taille de départ = taille minimale
constexpr double MAX_ZOOM   = 12.0;
constexpr double FADE_START = 4.0;   // à partir de ce zoom, la carte apparaît
constexpr double FADE_END   = 6.0;   // à ce zoom, seule la carte reste visible
constexpr double FLY_SPEED  = 0.008; // vitesse de disparition du texte

double smooth(double x)
{
    x = std::clamp(x, 0.0, 1.0);
    return x * x * (3.0 - 2.0 * x);
}
}

ZoomBackground::ZoomBackground(const QString &mapImagePath, int pointCount, QWidget *parent)
    : QWidget(parent), m_map(mapImagePath)
{
    // Version inversée de la carte
    QImage img = m_map.toImage();
    img.invertPixels(QImage::InvertRgb);
    m_mapInverted = QPixmap::fromImage(img);

    // Points de la sphère
    m_points.reserve(pointCount);
    auto *rng = QRandomGenerator::global();
    for (int i = 0; i < pointCount; ++i) {
        double z = 2.0 * rng->generateDouble() - 1.0;
        double theta = 2.0 * PI * rng->generateDouble();
        double r = std::sqrt(1.0 - z * z);
        m_points.push_back({ r * std::cos(theta), r * std::sin(theta), z });
    }

    // Interrupteur Light / Dark, invisible au départ
    m_toggle = new ToggleSwitch(this);
    m_toggleFx = new QGraphicsOpacityEffect(m_toggle);
    m_toggleFx->setOpacity(0.0);
    m_toggle->setGraphicsEffect(m_toggleFx);
    m_toggle->hide();
    connect(m_toggle, &QAbstractButton::toggled, this, &ZoomBackground::setInverted);

    // Animation
    connect(&m_timer, &QTimer::timeout, this, [this] {
        m_angle += m_speed;

        // Dès que la carte commence à apparaître, le texte commence à disparaître
        if (!m_flying && m_zoom >= FADE_START)
            m_flying = true;

        if (m_flying && m_flyProgress < 1.0)
            m_flyProgress = std::min(1.0, m_flyProgress + FLY_SPEED);

        // L'interrupteur apparaît en fondu avec la carte
        const double t = mapAmount();
        m_toggleFx->setOpacity(t);
        m_toggle->setVisible(t > 0.0);

        update();
    });
    m_timer.start(16);
}

double ZoomBackground::mapAmount() const
{
    return std::clamp((m_zoom - FADE_START) / (FADE_END - FADE_START), 0.0, 1.0);
}

void ZoomBackground::setInverted(bool on)
{
    m_inverted = on;
    update();
}

void ZoomBackground::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // Interrupteur en haut à droite
    m_toggle->move(width() - m_toggle->width() - 25, 25);
}

void ZoomBackground::wheelEvent(QWheelEvent *event)
{
    double factor = event->angleDelta().y() > 0 ? 1.1 : 1.0 / 1.1;
    m_zoom = std::clamp(m_zoom * factor, MIN_ZOOM, MAX_ZOOM);
    update();
    event->accept();
}

void ZoomBackground::mouseDoubleClickEvent(QMouseEvent *event)
{
    m_zoom = 1.0;
    update();
    event->accept();
}

void ZoomBackground::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.fillRect(rect(), m_inverted ? Qt::white : Qt::black);

    // t = 0 : sphère seule  |  t = 1 : carte seule
    const double t = mapAmount();

    if (t < 1.0) {
        painter.setOpacity(1.0 - t);
        drawSphere(painter);
    }
    if (t > 0.0) {
        painter.setOpacity(t);
        drawMap(painter);
    }

    // Texte d'accueil, dessiné par-dessus
    painter.setOpacity(1.0);
    drawWelcome(painter);
}

void ZoomBackground::drawSphere(QPainter &painter)
{
    const double cx = width() / 2.0;
    const double cy = height() / 2.0;
    // Taille de départ : la sphère touche presque le haut et le bas de la fenêtre
    const double radius = std::min(width(), height()) * 0.5 * m_zoom;

    const double cosA = std::cos(m_angle), sinA = std::sin(m_angle);
    const double tilt = 0.3;
    const double cosT = std::cos(tilt), sinT = std::sin(tilt);
    const double camera = 3.0;

    for (const auto &p : m_points) {
        // Rotation autour de Y
        double x  =  p.x * cosA + p.z * sinA;
        double y  =  p.y;
        double z  = -p.x * sinA + p.z * cosA;

        // Inclinaison autour de X
        double y2 =  y * cosT - z * sinT;
        double z2 =  y * sinT + z * cosT;

        // Projection en perspective
        double scale = camera / (camera - z2);
        double sx = cx + x  * radius * scale;
        double sy = cy + y2 * radius * scale;

        double depth = (z2 + 1.0) / 2.0;
        int alpha = 60 + static_cast<int>(195 * depth);

        // Points blancs sur fond noir, points noirs sur fond blanc
        QColor color = m_inverted ? QColor(0, 0, 0, alpha)
                                  : QColor(255, 255, 255, alpha);

        painter.setPen(QPen(color, 1.5 * scale));
        painter.drawPoint(QPointF(sx, sy));
    }
}

void ZoomBackground::drawMap(QPainter &painter)
{
    const QPixmap &pix = m_inverted ? m_mapInverted : m_map;
    if (pix.isNull())
        return;

    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QSize scaled = pix.size().scaled(size(), Qt::KeepAspectRatioByExpanding);
    QRect target(QPoint(0, 0), scaled);
    target.moveCenter(rect().center());
    painter.drawPixmap(target, pix);
}

void ZoomBackground::drawWelcome(QPainter &painter)
{
    if (m_flyProgress >= 1.0)
        return;

    const double w = width();
    const double h = height();
    const double p = m_flyProgress;

    const QColor fg     = m_inverted ? QColor(17, 17, 17)    : QColor(255, 255, 255);
    const QColor shadow = m_inverted ? QColor(190, 190, 190) : QColor(0, 0, 0);
    const QColor subFg  = m_inverted ? QColor(60, 60, 60)    : QColor(210, 210, 210);

    // ----- Titre : police type Impact, très condensée -----
    QFont titleFont;
    titleFont.setFamilies({ "Impact", "Bahnschrift Condensed", "Arial Black" });
    titleFont.setPixelSize(static_cast<int>(h * 0.15));
    painter.setFont(titleFont);
    const QFontMetrics fm(titleFont);

    const QStringList lines = { "BIENVENUE À", "MAKA IT" };
    const double startX     = w * 0.08;
    const double startY     = h * 0.38;
    const double lineH      = fm.height();
    const double spacing    = h * 0.008;
    const int    depthPx    = std::max(3, static_cast<int>(h * 0.012));
    const int    totalChars = lines[0].size() + lines[1].size();

    painter.setOpacity(1.0);

    int idx = 0;
    for (int k = 0; k < lines.size(); ++k) {
        const QString &line = lines[k];
        const double baseline = startY + k * lineH + fm.ascent();

        for (int j = 0; j < line.size(); ++j, ++idx) {
            const QChar ch = line[j];
            if (ch == ' ')
                continue;

            // Chaque lettre disparaît d'un coup, dans l'ordre d'écriture
            const double goneAt = (static_cast<double>(idx + 1) / totalChars) * 0.9;
            if (p >= goneAt)
                continue;

            const double x = startX + fm.horizontalAdvance(line.left(j)) + j * spacing;
            const QPointF pos(x, baseline);

            // Relief noir, décalé en diagonale
            painter.setPen(shadow);
            for (int d = 1; d <= depthPx; ++d)
                painter.drawText(pos + QPointF(d, d), QString(ch));

            // Lettre principale
            painter.setPen(fg);
            painter.drawText(pos, QString(ch));
        }
    }

    // ----- Sous-titre : disparaît d'un coup, à la fin -----
    if (p < 0.9) {
        QFont subFont;
        subFont.setFamilies({ "Bahnschrift Light", "Segoe UI Light", "Arial" });
        subFont.setPixelSize(static_cast<int>(h * 0.03));
        subFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);
        painter.setFont(subFont);

        const double subY = startY + 2 * lineH + h * 0.05;
        painter.setPen(subFg);
        painter.drawText(QRectF(startX, subY, w * 0.7, h * 0.2), Qt::TextWordWrap,
                         "Une application de gestion des emprunts de matériels informatiques");
    }
}