#include "dashboardpage.h"
#include "datastore.h"
#include "theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QLayout>
#include <QLayoutItem>
#include <QStyle>
#include <QShowEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QColor>
#include <QFont>
#include <QPair>
#include <QStringList>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QAbstractAnimation>
#include <QTimer>
#include <algorithm>
#include <cmath>

namespace {

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

// Couleur du camembert, par rang
QColor pieColor(int i)
{
    static const QStringList hex = { "#38bdf8", "#818cf8", "#34d399", "#fbbf24", "#f87171" };
    return QColor(hex.at(i % hex.size()));
}

// Couleur verte (normal) ou rouge (alerte), gérée par le thème
void setTone(QLabel *l, bool alert)
{
    l->setObjectName(alert ? "down" : "up");
    l->style()->unpolish(l);
    l->style()->polish(l);
}

// Vide un layout, y compris les sous-layouts
void clearLayout(QLayout *layout)
{
    if (!layout)
        return;

    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QWidget *w = item->widget())
            delete w;
        else if (QLayout *child = item->layout())
            clearLayout(child);
        delete item;
    }
}

// Fait varier l'opacité d'un effet après un délai (en millisecondes)
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
        if (n < 2 || m_labels.size() < n)
            return;

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const QRectF area = QRectF(rect()).adjusted(34, 12, -14, -26);

        // Échelle verticale adaptée aux données
        double maxV = 0;
        for (double v : m_values)
            maxV = std::max(maxV, v);
        const double yMax = std::max(5.0, std::ceil(maxV / 5.0) * 5.0);

        QFont f = font();
        f.setPixelSize(11);
        p.setFont(f);

        // Grille et valeurs de l'axe Y
        for (int i = 0; i <= 4; ++i) {
            const double y = area.bottom() - area.height() * i / 4.0;
            p.setPen(QPen(Theme::chartGrid(), 1));
            p.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
            p.setPen(Theme::muted());
            p.drawText(QRectF(0, y - 8, area.left() - 8, 16),
                       Qt::AlignRight | Qt::AlignVCenter,
                       QString::number(qRound(yMax * i / 4.0)));
        }

        // Positions des points
        QList<QPointF> pts;
        for (int i = 0; i < n; ++i) {
            const double x = area.left() + area.width() * i / (n - 1);
            const double y = area.bottom() - area.height() * (m_values[i] / yMax);
            pts << QPointF(x, y);
        }

        // Libellés de l'axe X
        p.setPen(Theme::muted());
        for (int i = 0; i < n; ++i)
            p.drawText(QRectF(pts[i].x() - 25, area.bottom() + 6, 50, 16),
                       Qt::AlignCenter, m_labels[i]);

        // Courbe et points
        QPainterPath path;
        path.moveTo(pts[0]);
        for (int i = 1; i < n; ++i)
            path.lineTo(pts[i]);
        p.setPen(QPen(Theme::chartLine(), 2));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);

        p.setPen(QPen(Theme::background(), 2));
        p.setBrush(Theme::chartLine());
        for (const QPointF &pt : pts)
            p.drawEllipse(pt, 4, 4);

        // Info-bulle sur le dernier point, toujours dans le widget
        const QPointF last = pts.last();
        const double tipW = 110;
        const double tipH = 40;

        double tipY = last.y() - tipH - 12;     // au-dessus du point par défaut
        if (tipY < 2)
            tipY = last.y() + 14;               // sinon en dessous
        tipY = qBound(2.0, tipY, height() - tipH - 2.0);

        const double tipX = qBound(2.0, last.x() - tipW / 2.0, width() - tipW - 2.0);
        const QRectF tip(tipX, tipY, tipW, tipH);

        p.setPen(QPen(Theme::chartGrid(), 1));
        p.setBrush(Theme::tooltipBg());
        p.drawRoundedRect(tip, 8, 8);
        p.setPen(Theme::text());
        p.drawText(tip.adjusted(10, 4, -10, -4), Qt::AlignLeft | Qt::AlignTop,
                   m_labels[n - 1] + "\n" + QString::number(qRound(m_values[n - 1])) + " emprunts");
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

        if (total > 0) {
            double start = 90.0;
            for (int i = 0; i < m_values.size() && i < m_colors.size(); ++i) {
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
        }

        QFont big = font();
        big.setPixelSize(static_cast<int>(side * 0.22));
        big.setBold(true);
        p.setFont(big);
        p.setPen(Theme::text());

        const QRectF valueRect(inner.left(), inner.center().y() - inner.height() * 0.35,
                               inner.width(), inner.height() * 0.4);
        p.drawText(valueRect, Qt::AlignCenter, m_centerValue);

        QFont small = font();
        small.setPixelSize(static_cast<int>(side * 0.08));
        p.setFont(small);
        p.setPen(Theme::muted());

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

DashboardPage::DashboardPage(DataStore *store, QWidget *parent)
    : QWidget(parent)
    , m_store(store)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(22, 20, 22, 20);
    root->setSpacing(14);

    QLabel *h1 = new QLabel("Bon visionnage");
    h1->setObjectName("h1");
    QLabel *sub = new QLabel("Voici un aperçu de l'état actuel du système.");
    sub->setObjectName("sub");
    root->addWidget(h1);
    root->addWidget(sub);

    // ----- Ligne 1 : quatre cartes, chacune avec son effet de fondu -----
    auto *kpis = new QHBoxLayout;
    kpis->setSpacing(14);

    const QStringList titles = { "Matériels disponibles", "Étudiants inscrits",
                                "Emprunts en cours", "Retours aujourd'hui" };
    for (int i = 0; i < 4; ++i) {
        QFrame *card = makeCard();
        auto *lay = new QVBoxLayout(card);
        lay->setContentsMargins(18, 14, 18, 14);
        lay->setSpacing(2);

        m_kpiValue[i] = new QLabel("0");
        m_kpiValue[i]->setObjectName("kpiValue");

        QLabel *t = new QLabel(titles[i]);
        t->setObjectName("kpiTitle");

        m_kpiSub[i] = new QLabel;
        m_kpiSub[i]->setObjectName("up");

        lay->addWidget(m_kpiValue[i]);
        lay->addWidget(t);
        lay->addWidget(m_kpiSub[i]);

        m_kpiFx[i] = new QGraphicsOpacityEffect(card);
        m_kpiFx[i]->setOpacity(0.0);
        card->setGraphicsEffect(m_kpiFx[i]);

        kpis->addWidget(card);
    }
    root->addLayout(kpis);

    // ----- Ligne 2 : courbe et répartition -----
    auto *middle = new QHBoxLayout;
    middle->setSpacing(14);

    QFrame *chartCard = makeCard();
    auto *cl = new QVBoxLayout(chartCard);
    cl->setContentsMargins(18, 14, 18, 14);
    cl->addWidget(makeTitle("Emprunts par mois"));

    LineChart *line = new LineChart;
    m_line = line;
    cl->addWidget(line, 1);
    middle->addWidget(chartCard, 3);

    QFrame *donutCard = makeCard();
    auto *dl = new QVBoxLayout(donutCard);
    dl->setContentsMargins(18, 14, 18, 14);
    dl->addWidget(makeTitle("Répartition des matériels"));

    auto *dRow = new QHBoxLayout;
    Donut *donut = new Donut;
    m_donut = donut;
    dRow->addWidget(donut, 1);

    QWidget *legendBox = new QWidget;
    m_legend = new QVBoxLayout(legendBox);
    m_legend->setContentsMargins(0, 0, 0, 0);
    dRow->addWidget(legendBox, 1);

    dl->addLayout(dRow, 1);
    middle->addWidget(donutCard, 2);
    root->addLayout(middle, 3);

    // ----- Ligne 3 : derniers emprunts et actions rapides -----
    auto *bottom = new QHBoxLayout;
    bottom->setSpacing(14);

    QFrame *tableCard = makeCard();
    auto *tl = new QVBoxLayout(tableCard);
    tl->setContentsMargins(18, 14, 18, 14);
    tl->addWidget(makeTitle("Derniers emprunts"));

    m_table = new QTableWidget(0, 5);
    m_table->setHorizontalHeaderLabels({ "Date", "Étudiant", "Matériel", "Durée", "Statut" });
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->viewport()->setAutoFillBackground(false);
    tl->addWidget(m_table, 1);
    bottom->addWidget(tableCard, 3);

    QFrame *actionCard = makeCard();
    auto *al = new QVBoxLayout(actionCard);
    al->setContentsMargins(18, 14, 18, 14);
    al->setSpacing(10);
    al->addWidget(makeTitle("Actions rapides"));

    // Bouton → index de la page à ouvrir
    const QList<QPair<QString, int>> actions = {
        { "Ajouter un matériel", 1 },
        { "Ajouter un étudiant", 2 },
        { "Nouvel emprunt", 3 },
        { "Nouveau retour", 4 }
    };
    for (const auto &a : actions) {
        QPushButton *b = new QPushButton(a.first + "            ›");
        b->setObjectName("action");
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(40);

        const int page = a.second;
        connect(b, &QPushButton::clicked, this, [this, page] {
            emit pageRequested(page);
        });
        al->addWidget(b);
    }
    al->addStretch();
    bottom->addWidget(actionCard, 2);
    root->addLayout(bottom, 2);

    reload();
}

void DashboardPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    reload();
}

void DashboardPage::playEntrance()
{
    for (int i = 0; i < 4; ++i)
        fadeTo(m_kpiFx[i], 1.0, 400, 150 + i * 150);
}

void DashboardPage::reload()
{
    const DashboardStats s = m_store->dashboardStats();

    // ----- Cartes -----
    m_kpiValue[0]->setText(QString::number(s.materielsDispo));
    setTone(m_kpiSub[0], false);
    m_kpiSub[0]->setText(QString("sur %1 au total").arg(s.materielsTotal));

    m_kpiValue[1]->setText(QString::number(s.etudiants));
    setTone(m_kpiSub[1], false);
    m_kpiSub[1]->setText("inscrits dans la base");

    m_kpiValue[2]->setText(QString::number(s.empruntsEnCours));
    setTone(m_kpiSub[2], s.empruntsRetard > 0);
    m_kpiSub[2]->setText(s.empruntsRetard > 0
                             ? QString("dont %1 en retard").arg(s.empruntsRetard)
                             : QString("aucun retard"));

    m_kpiValue[3]->setText(QString::number(s.retoursAujourdhui));
    setTone(m_kpiSub[3], false);
    m_kpiSub[3]->setText(QString("%1 prévu(s) aujourd'hui").arg(s.retoursPrevusAujourdhui));

    // ----- Courbe -----
    QList<double> vals;
    for (int v : s.mensuel)
        vals << v;
    static_cast<LineChart *>(m_line)->setData(s.moisLabels, vals);

    // ----- Camembert et légende : 4 catégories + « Autres » -----
    int total = 0;
    for (const Category &c : s.categories)
        total += c.count;

    QList<double> values;
    QList<QColor> colors;
    QStringList names;
    int autres = 0;

    for (int i = 0; i < s.categories.size(); ++i) {
        const Category &c = s.categories[i];
        if (i < 4) {
            values << c.count;
            names << c.libelle;
            colors << pieColor(i);
        } else {
            autres += c.count;
        }
    }
    if (autres > 0) {
        values << autres;
        names << "Autres";
        colors << pieColor(4);
    }

    clearLayout(m_legend);
    for (int i = 0; i < names.size(); ++i) {
        const int count = static_cast<int>(values[i]);
        const int pct = total > 0 ? qRound(100.0 * count / total) : 0;

        QLabel *dot = new QLabel;
        dot->setFixedSize(10, 10);
        dot->setStyleSheet(QString("background-color: %1; border-radius: 5px;")
                               .arg(colors[i].name()));

        QLabel *name = new QLabel(names[i]);
        name->setObjectName("legend");

        QLabel *val = new QLabel(QString("%1 (%2%)").arg(count).arg(pct));
        val->setObjectName("legendVal");

        auto *row = new QHBoxLayout;
        row->addWidget(dot);
        row->addSpacing(8);
        row->addWidget(name, 1);
        row->addWidget(val);
        m_legend->addLayout(row);
    }
    m_legend->addStretch();

    static_cast<Donut *>(m_donut)->setData(values, colors, QString::number(total), "matériels");

    // ----- Derniers emprunts -----
    const QDate today = QDate::currentDate();
    m_table->setRowCount(0);

    for (const RecentLoan &l : s.recent) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);

        const bool rendu = l.dateRetourEffective.isValid();
        const QDate fin = rendu ? l.dateRetourEffective : today;
        const int jours = l.dateEmprunt.daysTo(fin);

        QString statut;
        QColor couleur;
        if (rendu) {
            statut = "Retourné";
            couleur = QColor("#94a3b8");
        } else if (l.dateRetourPrevue < today) {
            statut = "En retard";
            couleur = QColor("#f87171");
        } else {
            statut = "En cours";
            couleur = QColor("#93c5fd");
        }

        m_table->setItem(row, 0, new QTableWidgetItem(l.dateEmprunt.toString("dd/MM/yyyy")));
        m_table->setItem(row, 1, new QTableWidgetItem(l.nomComplet));
        m_table->setItem(row, 2, new QTableWidgetItem(l.designation));
        m_table->setItem(row, 3, new QTableWidgetItem(QString("%1 jours").arg(jours)));

        QTableWidgetItem *st = new QTableWidgetItem(statut);
        st->setForeground(couleur);
        m_table->setItem(row, 4, st);
    }
}