#include "zoombackground.h"

#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QImage>
#include <QRandomGenerator>
#include <QFont>
#include <QFontMetrics>
#include <QStringList>
#include <algorithm>
#include <cmath>

namespace {
constexpr double PI             = 3.14159265358979323846;
constexpr int    POINT_COUNT    = 5000;
constexpr double MIN_ZOOM       = 1.0;    // taille de départ de la sphère
constexpr double MAX_ZOOM       = 12.0;   // taille maximale
constexpr double ZOOM_PER_NOTCH = 1.1;    // chaque cran multiplie la taille par 1,1
constexpr int    MAX_NOTCHES    = 26;     // 1.1^26 ≈ 11.9, donc on reste sous MAX_ZOOM
constexpr int    DASHBOARD_AT   = 4;      // le tableau de bord apparaît au 4e cran
constexpr double FADE_SPEED     = 0.12;   // vitesse des fondus (0 à 1)
constexpr int    MAP_NOTCH    = 3;      // cran à partir duquel la carte est affichée
}

ZoomBackground::ZoomBackground(QWidget *parent)
    : QWidget(parent)
{
    m_points.reserve(POINT_COUNT);
    auto *rng = QRandomGenerator::global();
    for (int i = 0; i < POINT_COUNT; ++i) {
        double z = 2.0 * rng->generateDouble() - 1.0;
        double theta = 2.0 * PI * rng->generateDouble();
        double r = std::sqrt(1.0 - z * z);
        m_points.push_back({ r * std::cos(theta), r * std::sin(theta), z });
    }

    connect(&m_timer, &QTimer::timeout, this, [this] { step(); });
    m_timer.start(16);
}

void ZoomBackground::step()
{
    m_angle += m_speed;

    // Cibles selon le nombre de crans
    const double textTarget = std::clamp(1.0 - m_notches / 2.0, 0.0, 1.0); // texte disparu au 2e cran
    const double mapTarget  = std::clamp((m_notches - 1) / 2.0, 0.0, 1.0); // carte complète au 3e cran

    // Fondus progressifs vers les cibles
    m_textAlpha += (textTarget - m_textAlpha) * FADE_SPEED;
    m_mapAlpha  += (mapTarget  - m_mapAlpha)  * FADE_SPEED;

    // Arrondi pour éviter les restes de 0.0001 invisibles mais coûteux
    if (std::abs(textTarget - m_textAlpha) < 0.002) m_textAlpha = textTarget;
    if (std::abs(mapTarget  - m_mapAlpha)  < 0.002) m_mapAlpha  = mapTarget;

    update();
}

void ZoomBackground::setNotches(int notches)
{
    notches = std::clamp(notches, 0, MAX_NOTCHES);

    // Une fois la carte affichée, on ne peut plus revenir à la sphère
    if (notches >= MAP_NOTCH)
        m_mapReached = true;
    if (m_mapReached && notches < MAP_NOTCH)
        notches = MAP_NOTCH;

    m_notches = notches;
    m_zoom = std::min(MAX_ZOOM, std::pow(ZOOM_PER_NOTCH, m_notches));

    // Une fois le tableau de bord affiché, il ne disparaît plus
    if (!m_dashShown && m_notches >= DASHBOARD_AT) {
        m_dashShown = true;
        emit dashboardVisible(true);
    }
}

void ZoomBackground::setMapImage(const QString &path)
{
    QImage img(path);
    if (img.isNull())
        return;

    img = img.convertToFormat(QImage::Format_RGB32);

    // Mode sombre : océans assombris, continents lumineux
    for (int y = 0; y < img.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            const QRgb c = line[x];
            const double g = qGray(c) / 255.0;
            const double f = std::pow(g, 1.8);
            line[x] = qRgb(qRound(qRed(c) * f),
                           qRound(qGreen(c) * f),
                           qRound(qBlue(c) * f));
        }
    }

    m_map = QPixmap::fromImage(img);

    // Mode clair : version inversée, puis les blancs sont ramenés vers un gris moyen
    QImage light = img;
    light.invertPixels(QImage::InvertRgb);

    // Les valeurs claires deviennent grises (plafond à 150), les sombres restent sombres
    const int GREY_MAX = 150;
    auto toGrey = [GREY_MAX](int v) {
        return qRound(GREY_MAX * std::pow(v / 255.0, 1.6));
    };

    for (int y = 0; y < light.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(light.scanLine(y));
        for (int x = 0; x < light.width(); ++x) {
            const QRgb c = line[x];
            line[x] = qRgb(toGrey(qRed(c)), toGrey(qGreen(c)), toGrey(qBlue(c)));
        }
    }

    m_mapInverted = QPixmap::fromImage(light);
    update();
}

void ZoomBackground::setInverted(bool on)
{
    m_inverted = on;
    update();
}

void ZoomBackground::wheelEvent(QWheelEvent *event)
{
    setNotches(m_notches + (event->angleDelta().y() > 0 ? 1 : -1));
    event->accept();
}

void ZoomBackground::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.fillRect(rect(), m_inverted ? QColor("#e5e7eb") : Qt::black);
    if (m_mapAlpha < 1.0) {
        painter.setOpacity(1.0 - m_mapAlpha);
        drawSphere(painter);
    }
    if (m_mapAlpha > 0.0) {
        painter.setOpacity(m_mapAlpha);
        drawMap(painter);
    }

    if (m_textAlpha > 0.0)
        drawWelcome(painter);
}

void ZoomBackground::drawSphere(QPainter &painter)
{
    const double cx = width() / 2.0;
    const double cy = height() / 2.0;
    const double radius = std::min(width(), height()) * 0.5 * m_zoom;

    const double cosA = std::cos(m_angle), sinA = std::sin(m_angle);
    const double tilt = 0.3;
    const double cosT = std::cos(tilt), sinT = std::sin(tilt);
    const double camera = 3.0;

    for (const auto &p : m_points) {
        double x  =  p.x * cosA + p.z * sinA;
        double y  =  p.y;
        double z  = -p.x * sinA + p.z * cosA;

        double y2 =  y * cosT - z * sinT;
        double z2 =  y * sinT + z * cosT;

        double scale = camera / (camera - z2);
        double sx = cx + x  * radius * scale;
        double sy = cy + y2 * radius * scale;

        double depth = (z2 + 1.0) / 2.0;
        int alpha = 60 + static_cast<int>(195 * depth);

        QColor color = m_inverted ? QColor(0, 0, 0, alpha)
                                  : QColor(255, 255, 255, alpha);

        painter.setPen(QPen(color, 1.5 * scale));
        painter.drawPoint(QPointF(sx, sy));
    }
}

void ZoomBackground::drawMap(QPainter &painter)
{
    // En mode clair, on n'affiche pas la carte : le fond gris suffit
    if (m_inverted)
        return;

    const QPixmap &pix = m_map;
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
    const double w = width();
    const double h = height();

    const QColor fg     = m_inverted ? QColor(17, 17, 17)    : QColor(255, 255, 255);
    const QColor shadow = m_inverted ? QColor(190, 190, 190) : QColor(0, 0, 0);
    const QColor subFg  = m_inverted ? QColor(60, 60, 60)    : QColor(210, 210, 210);

    painter.save();
    painter.setOpacity(m_textAlpha);

    QFont titleFont;
    titleFont.setFamilies({ "Impact", "Bahnschrift Condensed", "Arial Black" });
    titleFont.setPixelSize(static_cast<int>(h * 0.15));
    painter.setFont(titleFont);
    const QFontMetrics fm(titleFont);

    const QStringList lines = { "BIENVENUE À", "MAKA IT" };
    const double startX  = w * 0.08;
    const double startY  = h * 0.38;
    const double lineH   = fm.height();
    const int    depthPx = std::max(3, static_cast<int>(h * 0.012));

    for (int k = 0; k < lines.size(); ++k) {
        const double baseline = startY + k * lineH + fm.ascent();
        const QPointF pos(startX, baseline);

        // Relief noir décalé
        painter.setPen(shadow);
        for (int d = 1; d <= depthPx; ++d)
            painter.drawText(pos + QPointF(d, d), lines[k]);

        // Texte principal
        painter.setPen(fg);
        painter.drawText(pos, lines[k]);
    }

    QFont subFont;
    subFont.setFamilies({ "Bahnschrift Light", "Segoe UI Light", "Arial" });
    subFont.setPixelSize(static_cast<int>(h * 0.03));
    subFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);
    painter.setFont(subFont);

    const double subY = startY + 2 * lineH + h * 0.05;
    painter.setPen(subFg);
    painter.drawText(QRectF(startX, subY, w * 0.7, h * 0.2), Qt::TextWordWrap,
                     "Une application de gestion des emprunts de matériels informatiques");

    painter.restore();
}