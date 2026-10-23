#include "empruntspage.h"
#include "datastore.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDateEdit>
#include <QPushButton>
#include <QFrame>
#include <QMessageBox>

namespace {

const char *PAGE_QSS = R"CSS(
QLabel { background: transparent; color: #e5e7eb; }
QLabel#title { font-size: 24px; font-weight: bold; color: white; }
QLabel#subtitle { font-size: 12px; color: #94a3b8; }
QLabel#formTitle { font-size: 16px; font-weight: bold; color: white; }
QLabel#field { font-size: 12px; color: #cbd5e1; }
QLabel#done { font-size: 15px; font-weight: bold; color: #4ade80; }

QFrame#form, QFrame#summary {
    background-color: rgba(10, 10, 14, 190);
    border: 1px solid rgba(56, 189, 248, 80);
    border-radius: 14px;
}
QFrame#summary { border: 1px solid #4ade80; }

QLineEdit, QDateEdit {
    background-color: rgba(255, 255, 255, 15);
    border: 1px solid rgba(56, 189, 248, 90);
    border-radius: 8px;
    padding: 7px 10px;
    color: white;
}
QLineEdit:focus, QDateEdit:focus { border: 1px solid #38bdf8; }

QPushButton#primary {
    background-color: rgba(56, 189, 248, 70);
    color: white;
    border: 1px solid #38bdf8;
    border-radius: 8px;
    padding: 9px 14px;
    font-weight: bold;
}
QPushButton#primary:hover { background-color: rgba(56, 189, 248, 120); }
)CSS";

QLabel *fieldLabel(const QString &text)
{
    QLabel *l = new QLabel(text);
    l->setObjectName("field");
    return l;
}

} // namespace

EmpruntsPage::EmpruntsPage(DataStore *store, QWidget *parent)
    : QWidget(parent)
    , m_store(store)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);
    setStyleSheet(PAGE_QSS);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(16);

    QLabel *title = new QLabel("Emprunts");
    title->setObjectName("title");
    QLabel *sub = new QLabel("Enregistrez un nouvel emprunt de matériel.");
    sub->setObjectName("subtitle");
    root->addWidget(title);
    root->addWidget(sub);

    // ----- Formulaire -----
    QFrame *form = new QFrame;
    form->setObjectName("form");
    form->setMaximumWidth(520);
    auto *fl = new QVBoxLayout(form);
    fl->setContentsMargins(22, 20, 22, 20);
    fl->setSpacing(10);

    QLabel *ft = new QLabel("Nouvel emprunt");
    ft->setObjectName("formTitle");
    fl->addWidget(ft);

    fl->addWidget(fieldLabel("Nom ou numéro du matériel"));
    m_material = new QLineEdit;
    m_material->setPlaceholderText("Ex : Arduino Uno R3 ou A0001-00123");
    fl->addWidget(m_material);

    fl->addWidget(fieldLabel("Numéro matricule de l'emprunteur"));
    m_matricule = new QLineEdit;
    m_matricule->setPlaceholderText("Ex : ETU001");
    fl->addWidget(m_matricule);

    auto *dates = new QHBoxLayout;
    dates->setSpacing(12);

    auto *startCol = new QVBoxLayout;
    startCol->addWidget(fieldLabel("Date d'emprunt"));
    m_start = new QDateEdit(QDate::currentDate());
    m_start->setCalendarPopup(true);
    m_start->setDisplayFormat("dd/MM/yyyy");
    startCol->addWidget(m_start);
    dates->addLayout(startCol, 1);

    auto *endCol = new QVBoxLayout;
    endCol->addWidget(fieldLabel("Date de retour"));
    m_end = new QDateEdit(QDate::currentDate().addDays(7));
    m_end->setCalendarPopup(true);
    m_end->setDisplayFormat("dd/MM/yyyy");
    endCol->addWidget(m_end);
    dates->addLayout(endCol, 1);

    fl->addLayout(dates);

    QPushButton *ok = new QPushButton("Enregistrer l'emprunt");
    ok->setObjectName("primary");
    ok->setCursor(Qt::PointingHandCursor);
    fl->addWidget(ok);

    root->addWidget(form, 0, Qt::AlignTop);
    connect(ok, &QPushButton::clicked, this, &EmpruntsPage::onSubmit);

    // ----- Résumé, affiché après un emprunt réussi -----
    m_summary = new QFrame;
    m_summary->setObjectName("summary");
    m_summary->setMaximumWidth(520);
    m_summary->hide();

    auto *sl = new QVBoxLayout(m_summary);
    sl->setContentsMargins(22, 18, 22, 18);
    sl->setSpacing(10);

    QLabel *done = new QLabel("✔  Emprunt terminé");
    done->setObjectName("done");
    sl->addWidget(done);

    m_summaryText = new QLabel;
    m_summaryText->setTextFormat(Qt::RichText);
    m_summaryText->setWordWrap(true);
    sl->addWidget(m_summaryText);

    root->addWidget(m_summary, 0, Qt::AlignTop);
    root->addStretch();
}

void EmpruntsPage::prefillMaterial(const QString &name)
{
    m_material->setText(name);
    m_matricule->setFocus();
}

void EmpruntsPage::onSubmit()
{
    m_summary->hide();

    const QString key       = m_material->text().trimmed();
    const QString matricule = m_matricule->text().trimmed().toUpper();
    const QDate start       = m_start->date();
    const QDate end         = m_end->date();

    if (key.isEmpty() || matricule.isEmpty()) {
        QMessageBox::warning(this, "Champs manquants",
                             "Veuillez renseigner le matériel et le matricule de l'emprunteur.");
        return;
    }

    if (end < start) {
        QMessageBox::warning(this, "Dates incorrectes",
                             "La date de retour doit être postérieure à la date d'emprunt.");
        return;
    }

    // 1. Le matériel existe-t-il ?
    const Material *mat = m_store->findMaterial(key);
    if (!mat) {
        QMessageBox::warning(this, "Matériel introuvable",
                             QString("Aucun matériel ne correspond à « %1 ».").arg(key));
        return;
    }
    const QString materialName = mat->name;

    // 2. Le matériel est-il déjà emprunté ?
    if (m_store->isBorrowed(materialName)) {
        QMessageBox::warning(this, "Matériel déjà emprunté",
                             QString("« %1 » est déjà emprunté. Un matériel ne peut pas être "
                                     "emprunté par deux personnes en même temps.")
                                 .arg(materialName));
        return;
    }

    // 3. L'emprunteur est-il inscrit ?
    const Student *st = m_store->findStudent(matricule);
    if (!st) {
        QMessageBox::information(this, "Emprunteur non inscrit",
                                 QString("Le matricule %1 n'est pas enregistré.\n\n"
                                         "Vous allez être redirigé vers la page Étudiants "
                                         "pour l'inscrire.").arg(matricule));
        emit navigateToStudents();
        return;
    }

    // 4. A-t-il un prêt en retard ?
    if (m_store->hasLateLoan(st->matricule)) {
        QMessageBox::warning(this, "Emprunt impossible",
                             QString("%1 %2 a un prêt en retard. Il ne peut pas effectuer "
                                     "de nouvel emprunt tant que le matériel n'est pas rendu.")
                                 .arg(st->prenom, st->nom));
        return;
    }

    // Tout est valide : on enregistre l'emprunt
    const QString who = st->prenom + " " + st->nom;
    const QString mat2 = st->matricule;

    m_store->addLoan(Loan{ materialName, mat2, start, end, false });
    showSummary(materialName, who, mat2, start, end);

    m_material->clear();
    m_matricule->clear();

    emit loanCreated(materialName);
}

void EmpruntsPage::showSummary(const QString &material, const QString &who,
                               const QString &matricule, const QDate &start, const QDate &end)
{
    m_summaryText->setText(QString(
                               "<b>Matériel :</b> %1<br>"
                               "<b>Emprunteur :</b> %2 (%3)<br>"
                               "<b>Date d'emprunt :</b> %4<br>"
                               "<b>Date de retour :</b> %5")
                               .arg(material, who, matricule,
                                    start.toString("dd/MM/yyyy"), end.toString("dd/MM/yyyy")));

    m_summary->show();
}