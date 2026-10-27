#include "studentspage.h"
#include "theme.h"

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
#include <QDialog>
#include <QMessageBox>
#include <QShowEvent>
#include <QTimer>
#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include <QPen>
#include <QPointF>
#include <QColor>
#include <algorithm>

namespace {

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

// Boîte de saisie d'un étudiant, pré-remplie avec « data ».
// Retourne true si l'utilisateur a validé, et met alors « data » à jour.
bool studentDialog(QWidget *parent, const QString &title, Student &data)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    dlg.setMinimumWidth(380);

    auto *lay = new QVBoxLayout(&dlg);
    lay->setContentsMargins(22, 20, 22, 20);
    lay->setSpacing(8);

    QLabel *head = new QLabel(title);
    head->setObjectName("dlgTitle");
    lay->addWidget(head);

    auto addField = [&lay](const QString &label, const QString &placeholder,
                           const QString &value) {
        QLabel *l = new QLabel(label);
        l->setObjectName("field");
        QLineEdit *e = new QLineEdit(value);
        e->setPlaceholderText(placeholder);
        lay->addWidget(l);
        lay->addWidget(e);
        return e;
    };

    QLineEdit *nom       = addField("Nom", "Ex : Rakoto", data.nom);
    QLineEdit *prenom    = addField("Prénom", "Ex : Miora", data.prenom);
    QLineEdit *matricule = addField("Matricule", "Ex : ETU006", data.matricule);
    QLineEdit *tel       = addField("Téléphone", "Ex : 034 00 000 00 (facultatif)", data.telephone);

    auto *buttons = new QHBoxLayout;
    buttons->addStretch();

    QPushButton *cancel = new QPushButton("Annuler");
    cancel->setObjectName("secondary");
    cancel->setCursor(Qt::PointingHandCursor);

    QPushButton *ok = new QPushButton("Enregistrer");
    ok->setObjectName("primary");
    ok->setCursor(Qt::PointingHandCursor);

    buttons->addWidget(cancel);
    buttons->addWidget(ok);
    lay->addLayout(buttons);

    QObject::connect(cancel, &QPushButton::clicked, &dlg, &QDialog::reject);
    QObject::connect(ok, &QPushButton::clicked, &dlg, [&dlg, nom, prenom, matricule] {
        if (nom->text().trimmed().isEmpty() || prenom->text().trimmed().isEmpty()
            || matricule->text().trimmed().isEmpty()) {
            QMessageBox::warning(&dlg, "Champs requis",
                                 "Le nom, le prénom et le matricule sont obligatoires.");
            return;
        }
        dlg.accept();
    });

    if (dlg.exec() != QDialog::Accepted)
        return false;

    data.nom       = nom->text().trimmed();
    data.prenom    = prenom->text().trimmed();
    data.matricule = matricule->text().trimmed().toUpper();
    data.telephone = tel->text().trimmed();
    return true;
}

} // namespace

StudentsPage::StudentsPage(DataStore *store, QWidget *parent)
    : QWidget(parent)
    , m_store(store)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(16);

    QLabel *title = new QLabel("Étudiants");
    title->setObjectName("pageTitle");
    root->addWidget(title);

    // ----- Carte : recherche et bouton d'ajout -----
    QFrame *searchCard = new QFrame;
    searchCard->setObjectName("card");
    auto *sl = new QHBoxLayout(searchCard);
    sl->setContentsMargins(18, 14, 18, 14);
    sl->setSpacing(12);

    m_search = new QLineEdit;
    m_search->setPlaceholderText("Rechercher par nom, prénom, matricule ou téléphone...");
    m_search->setFixedHeight(40);
    m_search->setStyleSheet("QLineEdit { padding-left: 40px; }");
    sl->addWidget(m_search, 1);

    QLabel *searchIco = new QLabel(m_search);
    searchIco->setPixmap(searchIcon(QColor("#94a3b8")).pixmap(QSize(18, 18)));
    searchIco->setGeometry(12, 11, 18, 18);
    searchIco->setStyleSheet("background: transparent;");
    searchIco->setAttribute(Qt::WA_TransparentForMouseEvents);

    QPushButton *addBtn = new QPushButton("+  Ajouter un étudiant");
    addBtn->setObjectName("addBtn");
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setFixedHeight(40);
    sl->addWidget(addBtn);

    root->addWidget(searchCard);

    // ----- Carte : liste et pagination -----
    QFrame *listCard = new QFrame;
    listCard->setObjectName("card");
    auto *ll = new QVBoxLayout(listCard);
    ll->setContentsMargins(18, 14, 18, 14);
    ll->setSpacing(10);

    QLabel *listTitle = new QLabel("Liste des étudiants");
    listTitle->setObjectName("cardTitle");
    ll->addWidget(listTitle);

    m_count = new QLabel;
    m_count->setObjectName("subtitle");
    ll->addWidget(m_count);

    m_table = new QTableWidget(0, 6);
    m_table->setHorizontalHeaderLabels({ "#", "Nom", "Prénom", "Matricule", "Téléphone", "Action" });
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(44);
    m_table->viewport()->setAutoFillBackground(false);
    Theme::centerTable(m_table);


    QHeaderView *hh = m_table->horizontalHeader();
    hh->setStretchLastSection(false);
    hh->setMinimumSectionSize(50);
    hh->setSectionResizeMode(0, QHeaderView::Fixed);
    hh->setSectionResizeMode(1, QHeaderView::Stretch);
    hh->setSectionResizeMode(2, QHeaderView::Stretch);
    hh->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    hh->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    hh->setSectionResizeMode(5, QHeaderView::Fixed);
    m_table->setColumnWidth(0, 50);
    m_table->setColumnWidth(5, 120);

    ll->addWidget(m_table, 1);

    // Pied : éléments par page et pagination
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

    // ----- Connexions -----
    connect(m_search, &QLineEdit::textChanged, this, [this](const QString &) {
        m_page = 0;
        reload();
    });
    connect(m_perPage, &QComboBox::currentIndexChanged, this, [this](int) {
        m_page = 0;
        applyPage();
    });
    connect(m_prev, &QPushButton::clicked, this, [this] {
        --m_page;
        applyPage();
    });
    connect(m_next, &QPushButton::clicked, this, [this] {
        ++m_page;
        applyPage();
    });
    connect(addBtn, &QPushButton::clicked, this, &StudentsPage::onAdd);

    reload();
}

void StudentsPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    reload();
}

void StudentsPage::reload()
{
    m_all = m_store->listStudents(m_search->text());
    applyPage();
}

void StudentsPage::applyPage()
{
    const int perPage = m_perPage->currentData().toInt();
    const int total = static_cast<int>(m_all.size());
    const int pages = std::max(1, (total + perPage - 1) / perPage);
    m_page = std::clamp(m_page, 0, pages - 1);

    m_table->setRowCount(0);

    const int start = m_page * perPage;
    const int end = std::min(total, start + perPage);

    for (int i = start; i < end; ++i) {
        const Student s = m_all[i];
        const int row = m_table->rowCount();
        m_table->insertRow(row);

        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(i + 1)));
        m_table->setItem(row, 1, new QTableWidgetItem(s.nom));
        m_table->setItem(row, 2, new QTableWidgetItem(s.prenom));
        m_table->setItem(row, 3, new QTableWidgetItem(s.matricule));
        m_table->setItem(row, 4, new QTableWidgetItem(s.telephone.isEmpty() ? QString("—")
                                                                            : s.telephone));

        QPushButton *btn = new QPushButton("Modifier");
        btn->setObjectName("secondary");
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedSize(100, 32);
        connect(btn, &QPushButton::clicked, this, [this, s] { onEdit(s); });

        QWidget *cell = new QWidget;
        cell->setStyleSheet("background: transparent;");
        auto *cl = new QHBoxLayout(cell);
        cl->setContentsMargins(4, 0, 4, 0);
        cl->addWidget(btn, 0, Qt::AlignCenter);
        m_table->setCellWidget(row, 5, cell);
    }

    m_count->setText(QString("%1 étudiant(s)").arg(total));
    m_pageLabel->setText(QString("Page %1 / %2").arg(m_page + 1).arg(pages));
    m_prev->setEnabled(m_page > 0);
    m_next->setEnabled(m_page < pages - 1);
}

void StudentsPage::onAdd()
{
    Student data;
    if (!studentDialog(this, "Nouvel étudiant", data))
        return;

    if (!m_store->addStudent(data.nom, data.prenom, data.matricule, data.telephone)) {
        QMessageBox::warning(this, "Ajout impossible",
                             "Ce matricule existe déjà, ou l'enregistrement a échoué.");
        return;
    }

    QTimer::singleShot(0, this, &StudentsPage::reload);
}

void StudentsPage::onEdit(const Student &student)
{
    Student data = student;
    if (!studentDialog(this, "Modifier l'étudiant", data))
        return;

    if (!m_store->updateStudent(data.id, data.nom, data.prenom, data.matricule, data.telephone)) {
        QMessageBox::warning(this, "Modification impossible",
                             "Ce matricule est déjà utilisé par un autre étudiant, "
                             "ou la modification a échoué.");
        return;
    }

    QTimer::singleShot(0, this, &StudentsPage::reload);
}