#include "retourspage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QShowEvent>
#include <QMessageBox>
#include <QInputDialog>
#include <QStringList>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QColor>
#include <algorithm>
#include <utility>

namespace {

// Nombre de jours de retard (0 si le retour n'est pas dépassé)
int lateDaysOf(const ActiveLoan &l, const QDate &today)
{
    return l.dateRetourPrevue < today ? l.dateRetourPrevue.daysTo(today) : 0;
}

// Icône de recherche dessinée en code
QIcon searchIcon(const QColor &color)
{
    QPixmap pm(32, 32);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, 2.4, Qt::SolidLine, Qt::RoundCap));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPointF(13, 13), 8.5, 8.5);
    p.drawLine(QPointF(19.5, 19.5), QPointF(27, 27));
    p.end();

    return QIcon(pm);
}

} // namespace

RetoursPage::RetoursPage(DataStore *store, QWidget *parent)
    : QWidget(parent)
    , m_store(store)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(16);

    // ----- Carte 1 : recherche et filtre -----
    QFrame *searchCard = new QFrame;
    searchCard->setObjectName("card");
    auto *sl = new QVBoxLayout(searchCard);
    sl->setContentsMargins(18, 14, 18, 14);
    sl->setSpacing(12);

    QLabel *searchTitle = new QLabel("Rechercher un emprunt");
    searchTitle->setObjectName("cardTitle");
    sl->addWidget(searchTitle);

    auto *row = new QHBoxLayout;
    row->setSpacing(12);

    m_search = new QLineEdit;
    m_search->setPlaceholderText("Rechercher par nom, matricule, matériel...");
    m_search->setFixedHeight(40);
    m_search->setStyleSheet("QLineEdit { padding-left: 40px; }");
    row->addWidget(m_search, 1);

    QLabel *searchIco = new QLabel(m_search);
    searchIco->setPixmap(searchIcon(QColor("#94a3b8")).pixmap(QSize(18, 18)));
    searchIco->setGeometry(12, 11, 18, 18);
    searchIco->setStyleSheet("background: transparent;");
    searchIco->setAttribute(Qt::WA_TransparentForMouseEvents);

    m_status = new QComboBox;
    m_status->addItems({ "Tous les statuts", "En cours", "En retard" });
    m_status->setFixedSize(200, 40);
    row->addWidget(m_status);

    sl->addLayout(row);
    root->addWidget(searchCard);

    // ----- Carte 2 : liste des emprunts -----
    QFrame *listCard = new QFrame;
    listCard->setObjectName("card");
    auto *ll = new QVBoxLayout(listCard);
    ll->setContentsMargins(18, 14, 18, 14);
    ll->setSpacing(12);

    QLabel *listTitle = new QLabel("Liste des emprunts en cours");
    listTitle->setObjectName("cardTitle");
    ll->addWidget(listTitle);

    m_table = new QTableWidget(0, 7);
    m_table->setHorizontalHeaderLabels({ "#", "Étudiant", "Matériel", "Date d'emprunt",
                                        "Retour prévu", "Statut", "Action" });
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(52);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setColumnWidth(0, 50);
    m_table->viewport()->setAutoFillBackground(false);
    ll->addWidget(m_table, 1);

    // Pied : nombre d'éléments par page et pagination
    auto *foot = new QHBoxLayout;
    foot->setSpacing(8);

    foot->addWidget(new QLabel("Afficher"));
    m_perPage = new QComboBox;
    m_perPage->addItem("5", 5);
    m_perPage->addItem("10", 10);
    m_perPage->addItem("20", 20);
    m_perPage->setFixedWidth(80);
    foot->addWidget(m_perPage);
    foot->addWidget(new QLabel("éléments par page"));
    foot->addStretch();

    m_prev = new QPushButton("‹");
    m_prev->setFixedSize(34, 34);
    m_prev->setCursor(Qt::PointingHandCursor);

    m_pageLabel = new QLabel;
    m_pageLabel->setAlignment(Qt::AlignCenter);

    m_next = new QPushButton("›");
    m_next->setFixedSize(34, 34);
    m_next->setCursor(Qt::PointingHandCursor);

    foot->addWidget(m_prev);
    foot->addWidget(m_pageLabel);
    foot->addWidget(m_next);
    ll->addLayout(foot);

    root->addWidget(listCard, 1);

    // ----- Connexions : chaque changement revient à la page 1 -----
    connect(m_search, &QLineEdit::textChanged, this, [this](const QString &) {
        m_page = 0;
        applyFilters();
    });
    connect(m_status, &QComboBox::currentIndexChanged, this, [this](int) {
        m_page = 0;
        applyFilters();
    });
    connect(m_perPage, &QComboBox::currentIndexChanged, this, [this](int) {
        m_page = 0;
        applyFilters();
    });
    connect(m_prev, &QPushButton::clicked, this, [this] {
        --m_page;
        applyFilters();
    });
    connect(m_next, &QPushButton::clicked, this, [this] {
        ++m_page;
        applyFilters();
    });

    reload();
}

void RetoursPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    reload();
}

void RetoursPage::reload()
{
    m_all = m_store->listActiveLoans();
    applyFilters();
}

void RetoursPage::applyFilters()
{
    const QString needle = m_search->text().trimmed();
    const int statusMode = m_status->currentIndex();   // 0 = tous, 1 = en cours, 2 = en retard
    const QDate today = QDate::currentDate();

    // 1. Filtre : recherche et statut
    m_filtered.clear();
    for (const ActiveLoan &l : std::as_const(m_all)) {
        const bool late = lateDaysOf(l, today) > 0;

        if (statusMode == 1 && late)
            continue;
        if (statusMode == 2 && !late)
            continue;

        const bool match = needle.isEmpty()
                           || l.nomComplet.contains(needle, Qt::CaseInsensitive)
                           || l.matricule.contains(needle, Qt::CaseInsensitive)
                           || l.designation.contains(needle, Qt::CaseInsensitive)
                           || l.numeroSerie.contains(needle, Qt::CaseInsensitive);
        if (match)
            m_filtered << l;
    }

    // 2. Pagination
    const int perPage = m_perPage->currentData().toInt();
    const int total = static_cast<int>(m_filtered.size());
    const int pages = std::max(1, (total + perPage - 1) / perPage);
    m_page = std::clamp(m_page, 0, pages - 1);

    // 3. Remplissage du tableau pour la page courante
    m_table->setRowCount(0);

    const int start = m_page * perPage;
        const int end = std::min(total, start + perPage);
    for (int i = start; i < end; ++i) {
        const ActiveLoan &l = m_filtered[i];
        const int lateDays = lateDaysOf(l, today);
        const bool late = lateDays > 0;

        const int r = m_table->rowCount();
        m_table->insertRow(r);

        m_table->setItem(r, 0, new QTableWidgetItem(QString::number(i + 1)));
        m_table->setItem(r, 1, new QTableWidgetItem(l.nomComplet + "\n" + l.matricule));
        m_table->setItem(r, 2, new QTableWidgetItem(l.designation + "\n" + l.numeroSerie));
        m_table->setItem(r, 3, new QTableWidgetItem(l.dateEmprunt.toString("dd/MM/yyyy")));
        m_table->setItem(r, 4, new QTableWidgetItem(l.dateRetourPrevue.toString("dd/MM/yyyy")));

        QTableWidgetItem *statut = new QTableWidgetItem(
            late ? QString("En retard de %1 j").arg(lateDays) : QString("En cours"));
        statut->setForeground(late ? QColor("#f87171") : QColor("#93c5fd"));
        m_table->setItem(r, 5, statut);

        QPushButton *btn = new QPushButton("Retourner");
        btn->setObjectName("primary");
        btn->setCursor(Qt::PointingHandCursor);

        const int id = l.id;
        const QString designation = l.designation;
        const QString who = l.nomComplet;
        connect(btn, &QPushButton::clicked, this, [this, id, designation, who] {
            returnLoan(id, designation, who);
        });
        m_table->setCellWidget(r, 6, btn);
    }

    m_pageLabel->setText(QString("Page %1 / %2").arg(m_page + 1).arg(pages));
    m_prev->setEnabled(m_page > 0);
    m_next->setEnabled(m_page < pages - 1);
}

void RetoursPage::returnLoan(int idEmprunt, const QString &designation, const QString &who)
{
    // Demande l'état du matériel au moment du retour
    bool ok = false;
    const QStringList states = { "Bon état", "Endommagé" };
    const QString state = QInputDialog::getItem(
        this, "Retour du matériel",
        QString("État de « %1 » rendu par %2 :").arg(designation, who),
        states, 0, false, &ok);

    if (!ok)
        return;

    const bool damaged = (state == "Endommagé");

    if (!m_store->returnLoan(idEmprunt, damaged)) {
        QMessageBox::critical(this, "Erreur", "Le retour n'a pas pu être enregistré.");
        return;
    }

    QMessageBox::information(
        this, "Retour enregistré",
        damaged ? QString("« %1 » est enregistré comme endommagé. Il ne pourra pas être emprunté "
                          "avant réparation.").arg(designation)
                : QString("« %1 » est de nouveau disponible.").arg(designation));

    reload();
}