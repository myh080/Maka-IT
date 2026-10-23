#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "zoombackground.h"
#include "toggleswitch.h"
#include "materielspage.h"
#include "datastore.h"
#include "empruntspage.h"


#include <QStackedWidget>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QResizeEvent>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QColor>
#include <QPen>
#include <QIcon>
#include <QTimer>
#include <QDebug>

namespace {
constexpr int TOP_BAR = 70;   // hauteur réservée en haut pour la bascule Light/Dark
constexpr int SIDEBAR_WIDTH = 220;
const QString ICON_DIR = "H:/Qt project/Maka_IT/";

// Style du tableau de bord
const char *DASH_QSS = R"CSS(
QFrame#card {
    background-color: rgba(10, 10, 14, 170);
    border: 1px solid rgba(255, 255, 255, 45);
    border-radius: 14px;
}
QLabel { color: #e2e8f0; background: transparent; }
QLabel#h1 { font-size: 22px; font-weight: bold; color: white; }
QLabel#sub { font-size: 12px; color: #94a3b8; }
QLabel#cardTitle { font-size: 13px; font-weight: 600; color: white; }
QLabel#kpiValue { font-size: 26px; font-weight: bold; color: white; }
QLabel#kpiTitle { font-size: 12px; color: #e2e8f0; }
QLabel#up { font-size: 11px; color: #4ade80; }
QLabel#down { font-size: 11px; color: #f87171; }
QLabel#legend { font-size: 12px; color: #e2e8f0; }
QLabel#legendVal { font-size: 12px; color: #94a3b8; }
QPushButton#action {
    background-color: rgba(255, 255, 255, 12);
    color: white;
    border: 1px solid rgba(255, 255, 255, 45);
    border-radius: 10px;
    text-align: left;
    padding: 9px 14px;
    font-size: 12px;
}
QPushButton#action:hover { background-color: rgba(255, 255, 255, 30); }
)CSS";

// Image du projet, colorée en gardant la transparence du PNG
QPixmap loadTinted(const QString &file, const QColor &color)
{
    QPixmap src(ICON_DIR + file);
    if (src.isNull()) {
        qDebug() << "Image introuvable :" << ICON_DIR + file;
        return src;
    }

    QPixmap out(src.size());
    out.fill(Qt::transparent);

    QPainter p(&out);
    p.drawPixmap(0, 0, src);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(out.rect(), color);
    p.end();

    return out;
}

// Fait varier l'opacité d'un effet, après un délai (en millisecondes)
void fadeTo(QGraphicsOpacityEffect *fx, double to, int ms, int delay)
{
    auto *anim = new QPropertyAnimation(fx, "opacity", fx);
    anim->setDuration(ms);
    anim->setEndValue(to);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    QTimer::singleShot(delay, anim, [anim] {
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

QFrame *makeCard()
{
    QFrame *f = new QFrame;
    f->setObjectName("card");
    return f;
}

QLabel *makeTitle(const QString &text)
{
    QLabel *l = new QLabel(text);
    l->setObjectName("cardTitle");
    return l;
}

// Carte de chiffre clé : valeur, titre, tendance
QWidget *makeKpi(const QString &value, const QString &title,
                 const QString &trend, bool up)
{
    QFrame *card = makeCard();
    auto *lay = new QVBoxLayout(card);
    lay->setContentsMargins(18, 14, 18, 14);
    lay->setSpacing(2);

    QLabel *v = new QLabel(value);
    v->setObjectName("kpiValue");

    QLabel *t = new QLabel(title);
    t->setObjectName("kpiTitle");

    QLabel *tr = new QLabel((up ? "▲ " : "▼ ") + trend);
    tr->setObjectName(up ? "up" : "down");

    lay->addWidget(v);
    lay->addWidget(t);
    lay->addWidget(tr);
    return card;
}

// Tableau translucide, utilisé par toutes les pages
QTableWidget *makeTable(const QStringList &headers, const QList<QStringList> &rows)
{
    QTableWidget *table = new QTableWidget(rows.size(), headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    table->viewport()->setAutoFillBackground(false);
    table->setStyleSheet(
        "QTableWidget { background-color: rgba(255, 255, 255, 20);"
        " border: none; gridline-color: rgba(255, 255, 255, 25); color: white;"
        " alternate-background-color: rgba(255, 255, 255, 12); }"
        "QHeaderView::section { background-color: transparent; color: #94a3b8;"
        " border: none; border-bottom: 1px solid rgba(255, 255, 255, 40);"
        " padding: 6px; font-weight: bold; }");

    for (int r = 0; r < rows.size(); ++r)
        for (int c = 0; c < rows[r].size() && c < headers.size(); ++c)
            table->setItem(r, c, new QTableWidgetItem(rows[r][c]));

    return table;
}

// Courbe « Emprunts par mois »
class LineChart : public QWidget
{
public:
    explicit LineChart(QWidget *parent = nullptr) : QWidget(parent)
    {
        setMinimumHeight(160);
    }

    void setData(const QStringList &labels, const QList<double> &values)
    {
        m_labels = labels;
        m_values = values;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        const int n = m_values.size();
        if (n < 2)
            return;

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const QRectF area = QRectF(rect()).adjusted(34, 12, -14, -26);

        QFont f = font();
        f.setPixelSize(11);
        p.setFont(f);

        for (int i = 0; i <= 4; ++i) {
            const double y = area.bottom() - area.height() * i / 4.0;
            p.setPen(QPen(QColor(255, 255, 255, 35), 1));
            p.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
            p.setPen(QColor(148, 163, 184));
            p.drawText(QRectF(0, y - 8, area.left() - 8, 16),
                       Qt::AlignRight | Qt::AlignVCenter, QString::number(i * 5));
        }

        QList<QPointF> pts;
        for (int i = 0; i < n; ++i) {
            const double x = area.left() + area.width() * i / (n - 1);
            const double y = area.bottom() - area.height() * (m_values[i] / 20.0);
            pts << QPointF(x, y);
        }

        p.setPen(QColor(148, 163, 184));
        for (int i = 0; i < n && i < m_labels.size(); ++i)
            p.drawText(QRectF(pts[i].x() - 25, area.bottom() + 6, 50, 16),
                       Qt::AlignCenter, m_labels[i]);

        QPainterPath path;
        path.moveTo(pts[0]);
        for (int i = 1; i < n; ++i)
            path.lineTo(pts[i]);
        p.setPen(QPen(Qt::white, 2));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);

        p.setPen(QPen(QColor(15, 15, 15), 2));
        p.setBrush(Qt::white);
        for (const QPointF &pt : pts)
            p.drawEllipse(pt, 4, 4);

        const QPointF last = pts.last();
        const QRectF tip(qMin(last.x() - 50, width() - 120.0), last.y() - 52, 110, 40);
        p.setPen(QPen(QColor(255, 255, 255, 70), 1));
        p.setBrush(QColor(15, 15, 15, 230));
        p.drawRoundedRect(tip, 8, 8);
        p.setPen(Qt::white);
        p.drawText(tip.adjusted(10, 4, -10, -4), Qt::AlignLeft | Qt::AlignTop,
                   m_labels.last() + "\n" + QString::number(m_values.last()) + " emprunts");
    }

private:
    QStringList m_labels;
    QList<double> m_values;
};

// Anneau « Répartition des matériels »
class Donut : public QWidget
{
public:
    explicit Donut(QWidget *parent = nullptr) : QWidget(parent)
    {
        setMinimumSize(150, 150);
    }

    void setData(const QList<double> &values, const QList<QColor> &colors,
                 const QString &centerValue, const QString &centerLabel)
    {
        m_values = values;
        m_colors = colors;
        m_centerValue = centerValue;
        m_centerLabel = centerLabel;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const double side = qMin(width(), height()) - 12.0;
        const QRectF outer((width() - side) / 2.0, (height() - side) / 2.0, side, side);
        const double thick = side * 0.20;
        const QRectF inner = outer.adjusted(thick, thick, -thick, -thick);

        double total = 0;
        for (double v : m_values)
            total += v;

        double start = 90.0;
        for (int i = 0; i < m_values.size() && i < m_colors.size() && total > 0; ++i) {
            const double span = -360.0 * m_values[i] / total;

            QPainterPath path;
            path.arcMoveTo(outer, start);
            path.arcTo(outer, start, span);
            path.arcTo(inner, start + span, -span);
            path.closeSubpath();

            p.setPen(Qt::NoPen);
            p.setBrush(m_colors[i]);
            p.drawPath(path);
            start += span;
        }

        QFont big = font();
        big.setPixelSize(static_cast<int>(side * 0.22));
        big.setBold(true);
        p.setFont(big);
        p.setPen(Qt::white);

        const QRectF valueRect(inner.left(), inner.center().y() - inner.height() * 0.35,
                               inner.width(), inner.height() * 0.4);
        p.drawText(valueRect, Qt::AlignCenter, m_centerValue);

        QFont small = font();
        small.setPixelSize(static_cast<int>(side * 0.08));
        p.setFont(small);
        p.setPen(QColor(148, 163, 184));

        const QRectF labelRect(inner.left(), inner.center().y() + inner.height() * 0.05,
                               inner.width(), inner.height() * 0.3);
        p.drawText(labelRect, Qt::AlignCenter, m_centerLabel);
    }

private:
    QList<double> m_values;
    QList<QColor> m_colors;
    QString m_centerValue;
    QString m_centerLabel;
};

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Gestion des emprunts de matériels informatiques");

    const QColor accent(255, 255, 255);   // blanc

    // Logo et icône du Tableau de bord
    ui->logoLabel->setPixmap(loadTinted("departement-it.png", accent));
    ui->logoLabel->setScaledContents(true);

    ui->NavDashBoard->setIcon(QIcon(loadTinted("lanalyse-des-donnees.png", accent)));
    ui->NavDashBoard->setIconSize(QSize(20, 20));

    ui->NavEtudiant->setIcon(QIcon(loadTinted("etudiant.png", accent)));
    ui->NavEtudiant->setIconSize(QSize(20, 20));

    ui->NavMateriels->setIcon(QIcon(loadTinted("ordinateur-personnel.png", accent)));
    ui->NavMateriels->setIconSize(QSize(20, 20));

    ui->NavEmprunts->setIcon(QIcon(loadTinted("chaine-de-produits.png", accent)));
    ui->NavEmprunts->setIconSize(QSize(20, 20));

    ui->NavRetours->setIcon(QIcon(loadTinted("boite-de-retour.png", accent)));
    ui->NavRetours->setIconSize(QSize(20, 20));


    // Image de fond
    ui->zoomBg->setMapImage(ICON_DIR + "typo.jpg");

    // Pages à droite de la barre latérale
    m_pages = new QStackedWidget(ui->zoomBg);

    m_pages->addWidget(makeDashboardPage());          // 0 : tableau de bord

    MaterielsPage *materiels = new MaterielsPage;
    m_pages->addWidget(materiels);                      // 1 : matériels

    // Double-clic sur un matériel disponible et « Oui » : ouvre la page Emprunts
    connect(materiels, &MaterielsPage::emprunterRequested, this, [this](const QString &name) {
        m_emprunts->prefillMaterial(name);
        showPage(3);
    });

    m_pages->addWidget(makeTablePage("Étudiants",     // 2
                                     { "ID", "Nom", "Prénom", "N° étudiant" },
                                     {
                                         { "1", "Rakoto",        "Miora", "ETU001" },
                                         { "2", "Rabe",          "Hery",  "ETU002" },
                                         { "3", "Andriamifidy",  "Lova",  "ETU003" },
                                         { "4", "Razafindrabe",  "Tiana", "ETU004" },
                                         { "5", "Rakotomamonjy", "Sarah", "ETU005" }
                                     }));
    // 3 : Emprunts (données partagées et formulaire)
    m_store = new DataStore;
    m_emprunts = new EmpruntsPage(m_store);
    m_pages->addWidget(m_emprunts);
    connect(m_emprunts, &EmpruntsPage::navigateToStudents, this, [this] { showPage(2); });

    QLabel *retours = new QLabel("Retours : page à venir");
    retours->setAlignment(Qt::AlignCenter);
    m_pages->addWidget(retours);                      // 4 : retours

    // Au départ, le menu, les pages et l'interrupteur sont cachés
    ui->sidebar->hide();
    m_pages->hide();
    ui->toggleTheme->hide();

    connect(ui->zoomBg, &ZoomBackground::dashboardVisible,
            this, &MainWindow::setDashboardVisible);

    // Interrupteur : inverse les couleurs du fond
    connect(ui->toggleTheme, &QAbstractButton::toggled,
            ui->zoomBg, &ZoomBackground::setInverted);

    // ----- Navigation : la position dans m_navs correspond au numéro de page -----
    // 0 = Tableau de bord, 1 = Matériels, 2 = Étudiants (pas de bouton), 3 = Emprunts, 4 = Retours
    m_navs = { ui->NavDashBoard, ui->NavMateriels, nullptr, ui->NavEmprunts, nullptr };

    for (int i = 0; i < m_navs.size(); ++i) {
        if (!m_navs[i])
            continue;
        m_navs[i]->setCheckable(true);
        connect(m_navs[i], &QPushButton::clicked, this, [this, i] { showPage(i); });
    }

    showPage(0);
    connect(ui->NavMateriels, &QPushButton::clicked, this, [this] { showPage(1); });

    showPage(0);

    layoutInside();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setDashboardVisible(bool show)
{
    if (show) {
        ui->sidebar->show();
        m_pages->show();
        ui->toggleTheme->show();
        ui->toggleTheme->raise();

        // Les cartes apparaissent une par une, en fondu
        for (int i = 0; i < m_kpiFx.size(); ++i)
            fadeTo(m_kpiFx[i], 1.0, 400, 150 + i * 150);
    } else {
        ui->sidebar->hide();
        m_pages->hide();
        ui->toggleTheme->hide();

        for (QGraphicsOpacityEffect *fx : m_kpiFx)
            fx->setOpacity(0.0);
    }
}

void MainWindow::showPage(int index)
{
    m_pages->setCurrentIndex(index);

    for (int i = 0; i < m_navs.size(); ++i)
        if (m_navs[i])
            m_navs[i]->setChecked(i == index);
}

QWidget *MainWindow::makeDashboardPage()
{
    QWidget *page = new QWidget;
    page->setAttribute(Qt::WA_TranslucentBackground);
    page->setAutoFillBackground(false);
    page->setStyleSheet(DASH_QSS);

    auto *root = new QVBoxLayout(page);
    root->setContentsMargins(22, 20, 22, 20);
    root->setSpacing(14);

    QLabel *h1 = new QLabel("Bon Visionnage!");
    h1->setObjectName("h1");
    QLabel *sub = new QLabel("Voici un aperçu de l'état actuel du système.");
    sub->setObjectName("sub");
    root->addWidget(h1);
    root->addWidget(sub);

    // Ligne 1 : quatre cartes de statistiques, en fondu une par une
    auto *kpis = new QHBoxLayout;
    kpis->setSpacing(14);

    m_kpiFx.clear();
    const QList<QWidget *> cards = {
        makeKpi("32", "Matériels disponibles", "+2 cette semaine", true),
        makeKpi("18", "Étudiants inscrits", "+1 cette semaine", true),
        makeKpi("7", "Emprunts en cours", "-3 cette semaine", false),
        makeKpi("5", "Retours aujourd'hui", "+2 cette semaine", true)
    };
    for (QWidget *card : cards) {
        auto *fx = new QGraphicsOpacityEffect(card);
        fx->setOpacity(0.0);
        card->setGraphicsEffect(fx);
        m_kpiFx << fx;
        kpis->addWidget(card);
    }
    root->addLayout(kpis);

    // Ligne 2 : courbe + répartition
    auto *middle = new QHBoxLayout;
    middle->setSpacing(14);

    QFrame *chartCard = makeCard();
    auto *cl = new QVBoxLayout(chartCard);
    cl->setContentsMargins(18, 14, 18, 14);
    cl->addWidget(makeTitle("Emprunts par mois"));

    LineChart *line = new LineChart;
    line->setData({ "Sep", "Oct", "Nov", "Déc", "Jan", "Fév", "Mars", "Avr", "Mai", "Juin" },
                  { 4, 7, 5, 8, 10, 9, 12, 10, 12, 14 });
    cl->addWidget(line, 1);
    middle->addWidget(chartCard, 3);

    QFrame *donutCard = makeCard();
    auto *dl = new QVBoxLayout(donutCard);
    dl->setContentsMargins(18, 14, 18, 14);
    dl->addWidget(makeTitle("Répartition des matériels"));

    const QStringList names  = { "PC Portable", "PC Fixe", "Écran", "Clavier", "Souris" };
    const QStringList counts = { "12 (37%)", "8 (25%)", "6 (19%)", "4 (13%)", "2 (6%)" };
    const QStringList hexes  = { "#f1f5f9", "#cbd5e1", "#94a3b8", "#64748b", "#334155" };

    QList<QColor> colors;
    for (const QString &h : hexes)
        colors << QColor(h);

    Donut *donut = new Donut;
    donut->setData({ 12, 8, 6, 4, 2 }, colors, "32", "matériels");

    auto *dRow = new QHBoxLayout;
    dRow->addWidget(donut, 1);

    auto *legend = new QVBoxLayout;
    for (int i = 0; i < names.size(); ++i) {
        QLabel *dot = new QLabel;
        dot->setFixedSize(10, 10);
        dot->setStyleSheet(QString("background-color: %1; border-radius: 5px;").arg(hexes[i]));

        QLabel *name = new QLabel(names[i]);
        name->setObjectName("legend");
        QLabel *val = new QLabel(counts[i]);
        val->setObjectName("legendVal");

        auto *row = new QHBoxLayout;
        row->addWidget(dot);
        row->addSpacing(8);
        row->addWidget(name, 1);
        row->addWidget(val);
        legend->addLayout(row);
    }
    dRow->addLayout(legend, 1);
    dl->addLayout(dRow, 1);
    middle->addWidget(donutCard, 2);

    root->addLayout(middle, 3);

    // Ligne 3 : derniers emprunts + actions rapides
    auto *bottom = new QHBoxLayout;
    bottom->setSpacing(14);

    QFrame *tableCard = makeCard();
    auto *tl = new QVBoxLayout(tableCard);
    tl->setContentsMargins(18, 14, 18, 14);
    tl->addWidget(makeTitle("Derniers emprunts"));

    QTableWidget *table = makeTable(
        { "Date", "Étudiant", "Matériel", "Durée", "Statut" },
        {
            { "24/10/2026 08:14", "Rajaonary L.",        "PC Portable", "3 jours", "En cours" },
            { "23/10/2026 15:32", "Andrianantenaina T.", "Souris",      "7 jours", "En cours" },
            { "22/10/2026 10:20", "Razafindrakoto M.",   "Écran",       "5 jours", "En cours" },
            { "20/10/2026 16:45", "Rasoa F.",            "Clavier",     "3 jours", "Retourné" },
            { "18/10/2026 09:12", "Rakotomalala H.",     "PC Fixe",     "7 jours", "Retourné" }
        });

    for (int r = 0; r < table->rowCount(); ++r) {
        QTableWidgetItem *it = table->item(r, 4);
        it->setForeground(it->text().startsWith("En") ? QColor("#93c5fd") : QColor("#94a3b8"));
    }

    tl->addWidget(table, 1);
    bottom->addWidget(tableCard, 3);

    QFrame *actionCard = makeCard();
    auto *al = new QVBoxLayout(actionCard);
    al->setContentsMargins(18, 14, 18, 14);
    al->setSpacing(10);
    al->addWidget(makeTitle("Actions rapides"));

    const QStringList actions = { "Ajouter un matériel", "Ajouter un étudiant",
                                 "Nouvel emprunt", "Nouveau retour" };
    for (const QString &a : actions) {
        QPushButton *b = new QPushButton(a + "            ›");
        b->setObjectName("action");
        b->setCursor(Qt::PointingHandCursor);
        al->addWidget(b);
    }
    al->addStretch();
    bottom->addWidget(actionCard, 2);

    root->addLayout(bottom, 2);

    return page;
}

QWidget *MainWindow::makeTablePage(const QString &title,
                                   const QStringList &headers,
                                   const QList<QStringList> &rows)
{
    QWidget *page = new QWidget;
    page->setAttribute(Qt::WA_TranslucentBackground);
    page->setAutoFillBackground(false);

    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    QLabel *titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: white;"
                              " background: transparent;");
    layout->addWidget(titleLabel);
    layout->addWidget(makeTable(headers, rows), 1);

    return page;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (!m_pages)
        return;

    ui->zoomBg->setGeometry(ui->centralwidget->rect());
    ui->zoomBg->lower();
    layoutInside();
}

void MainWindow::layoutInside()
{
    const int W = ui->zoomBg->width();
    const int H = ui->zoomBg->height();

    ui->sidebar->setGeometry(0, 0, SIDEBAR_WIDTH, H);

    // Les pages commencent sous la zone de la bascule
    m_pages->setGeometry(SIDEBAR_WIDTH, TOP_BAR, W - SIDEBAR_WIDTH, H - TOP_BAR);

    ui->toggleTheme->move(W - ui->toggleTheme->width() - 25, 25);
    ui->toggleTheme->raise();
}