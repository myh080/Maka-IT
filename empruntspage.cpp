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

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(16);

    QLabel *title = new QLabel("Emprunts");
    title->setObjectName("title");
    QLabel *sub = new QLabel("Enregistrez un nouvel emprunt de matériel.");
    sub->setObjectName("subtitle");
    root->addWidget(title);
    root->addWidget(sub);

    // ----- Bloc centré : formulaire puis résumé -----
    auto *center = new QVBoxLayout;
    center->setSpacing(14);

    QFrame *form = new QFrame;
    form->setObjectName("form");
    form->setFixedWidth(420);
    auto *fl = new QVBoxLayout(form);
    fl->setContentsMargins(22, 18, 22, 18);
    fl->setSpacing(8);

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
    connect(ok, &QPushButton::clicked, this, &EmpruntsPage::onSubmit);

    center->addWidget(form, 0, Qt::AlignHCenter);

    // ----- Résumé, affiché après un emprunt réussi -----
    m_summary = new QFrame;
    m_summary->setObjectName("summary");
    m_summary->setFixedWidth(420);
    m_summary->hide();

    auto *sl = new QVBoxLayout(m_summary);
    sl->setContentsMargins(22, 16, 22, 16);
    sl->setSpacing(8);

    QLabel *done = new QLabel("✔  Emprunt terminé");
    done->setObjectName("done");
    sl->addWidget(done);

    m_summaryText = new QLabel;
    m_summaryText->setTextFormat(Qt::RichText);
    m_summaryText->setWordWrap(true);
    sl->addWidget(m_summaryText);

    center->addWidget(m_summary, 0, Qt::AlignHCenter);

    root->addStretch(1);
    root->addLayout(center);
    root->addStretch(1);
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
    const QString matricule = m_matricule->text().trimmed();
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
    const auto mat = m_store->findMaterial(key);
    if (!mat) {
        QMessageBox::warning(this, "Matériel introuvable",
                             QString("Aucun matériel ne correspond à « %1 ».").arg(key));
        return;
    }

    // 2. Le matériel est-il disponible ?
    if (mat->etat == "endommage") {
        QMessageBox::warning(this, "Matériel endommagé",
                             QString("« %1 » est endommagé et ne peut pas être emprunté.")
                                 .arg(mat->designation));
        return;
    }
    if (m_store->isBorrowed(mat->id)) {
        QMessageBox::warning(this, "Matériel déjà emprunté",
                             QString("« %1 » est déjà emprunté. Un matériel ne peut pas être "
                                     "emprunté par deux personnes en même temps.")
                                 .arg(mat->designation));
        return;
    }

    // 3. L'emprunteur est-il inscrit ?
    const auto st = m_store->findStudent(matricule);
    if (!st) {
        QMessageBox::information(this, "Emprunteur non inscrit",
                                 QString("Le matricule %1 n'est pas enregistré.\n\n"
                                         "Vous allez être redirigé vers la page Étudiants "
                                         "pour l'inscrire.").arg(matricule));
        emit navigateToStudents();
        return;
    }

    // 4. A-t-il un prêt en retard ?
    if (m_store->hasLateLoan(st->id)) {
        QMessageBox::warning(this, "Emprunt impossible",
                             QString("%1 %2 a un prêt en retard. Il doit rendre ce matériel "
                                     "avant de pouvoir emprunter à nouveau.")
                                 .arg(st->prenom, st->nom));
        return;
    }

    // 5. A-t-il un autre matériel non rendu ?
    if (m_store->hasActiveLoan(st->id)) {
        QMessageBox::warning(this, "Emprunt impossible",
                             QString("%1 %2 a déjà un matériel non rendu. Il doit le rendre "
                                     "avant de pouvoir emprunter autre chose.")
                                 .arg(st->prenom, st->nom));
        return;
    }

    // Tout est valide : enregistrement
    Loan loan;
    loan.materielId       = mat->id;
    loan.emprunteurId     = st->id;
    loan.dateEmprunt      = start;
    loan.dateRetourPrevue = end;

    if (!m_store->addLoan(loan)) {
        QMessageBox::critical(this, "Erreur", "L'emprunt n'a pas pu être enregistré.");
        return;
    }

    showSummary(mat->designation, st->prenom + " " + st->nom, st->matricule, start, end);

    m_material->clear();
    m_matricule->clear();

    emit loanCreated(mat->designation);
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