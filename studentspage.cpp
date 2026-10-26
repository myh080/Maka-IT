#include "studentspage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
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

    // ----- Carte : recherche et bouton d'ajout sur la même ligne -----
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

    // ----- Carte : liste -----
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
    root->addWidget(listCard, 1);

    // ----- Connexions -----
    connect(m_search, &QLineEdit::textChanged, this, [this](const QString &) {
        reload();
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
    const QList<Student> list = m_store->listStudents(m_search->text());

    m_table->setRowCount(0);
    for (int i = 0; i < list.size(); ++i) {
        const Student s = list[i];
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
        connect(btn, &QPushButton::clicked, this, [this, s] { onEdit(s); });
        m_table->setCellWidget(row, 5, btn);
    }

    m_count->setText(QString("%1 étudiant(s)").arg(list.size()));
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

    // Reporté : on ne supprime pas le bouton qui vient d'émettre le signal
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