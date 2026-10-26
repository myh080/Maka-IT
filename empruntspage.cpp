#include "empruntspage.h"
#include "datastore.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QPushButton>
#include <QFrame>
#include <QMessageBox>
#include <QShowEvent>

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

    // ----- Formulaire -----
    auto *center = new QVBoxLayout;
    center->setSpacing(14);

    QFrame *form = new QFrame;
    form->setObjectName("form");
    form->setFixedWidth(460);

    auto *fl = new QVBoxLayout(form);
    fl->setContentsMargins(24, 20, 24, 20);
    fl->setSpacing(6);

    QLabel *ft = new QLabel("Nouvel emprunt");
    ft->setObjectName("formTitle");
    fl->addWidget(ft);
    fl->addSpacing(6);

    // Chaque champ a son libellé au-dessus, et une hauteur fixe
    auto field = [&fl](const QString &label, QWidget *input) {
        QLabel *l = new QLabel(label);
        l->setObjectName("field");
        fl->addWidget(l);
        input->setFixedHeight(36);
        fl->addWidget(input);
    };

    m_category = new QComboBox;
    field("Catégorie de matériel", m_category);

    m_material = new QComboBox;
    field("Matériel disponible", m_material);

    m_matricule = new QLineEdit;
    m_matricule->setPlaceholderText("Ex : ETU001");
    field("Matricule de l'emprunteur", m_matricule);

    m_borrowerStatus = new QLabel;
    m_borrowerStatus->setObjectName("field");
    m_borrowerStatus->setMinimumHeight(18);
    fl->addWidget(m_borrowerStatus);

    // Nom et prénom côte à côte
    auto *names = new QHBoxLayout;
    names->setSpacing(12);

    auto *nomCol = new QVBoxLayout;
    nomCol->setSpacing(6);
    QLabel *lNom = new QLabel("Nom");
    lNom->setObjectName("field");
    m_nom = new QLineEdit;
    m_nom->setReadOnly(true);
    m_nom->setFixedHeight(36);
    nomCol->addWidget(lNom);
    nomCol->addWidget(m_nom);
    names->addLayout(nomCol, 1);

    auto *prenomCol = new QVBoxLayout;
    prenomCol->setSpacing(6);
    QLabel *lPrenom = new QLabel("Prénom");
    lPrenom->setObjectName("field");
    m_prenom = new QLineEdit;
    m_prenom->setReadOnly(true);
    m_prenom->setFixedHeight(36);
    prenomCol->addWidget(lPrenom);
    prenomCol->addWidget(m_prenom);
    names->addLayout(prenomCol, 1);

    fl->addLayout(names);
    fl->addSpacing(6);

    // Dates côte à côte
    auto *dates = new QHBoxLayout;
    dates->setSpacing(12);

    auto *startCol = new QVBoxLayout;
    startCol->setSpacing(6);
    QLabel *lStart = new QLabel("Date d'emprunt");
    lStart->setObjectName("field");
    m_start = new QDateEdit(QDate::currentDate());
    m_start->setCalendarPopup(true);
    m_start->setDisplayFormat("dd/MM/yyyy");
    m_start->setFixedHeight(36);
    startCol->addWidget(lStart);
    startCol->addWidget(m_start);
    dates->addLayout(startCol, 1);

    auto *endCol = new QVBoxLayout;
    endCol->setSpacing(6);
    QLabel *lEnd = new QLabel("Date de retour prévue");
    lEnd->setObjectName("field");
    m_end = new QDateEdit(QDate::currentDate().addDays(7));
    m_end->setCalendarPopup(true);
    m_end->setDisplayFormat("dd/MM/yyyy");
    m_end->setFixedHeight(36);
    endCol->addWidget(lEnd);
    endCol->addWidget(m_end);
    dates->addLayout(endCol, 1);

    fl->addLayout(dates);
    fl->addSpacing(10);

    QPushButton *ok = new QPushButton("Enregistrer l'emprunt");
    ok->setObjectName("primary");
    ok->setCursor(Qt::PointingHandCursor);
    ok->setFixedHeight(42);
    fl->addWidget(ok);

    // ----- Résumé, affiché après un emprunt réussi -----
    m_summary = new QFrame;
    m_summary->setObjectName("summary");
    m_summary->setFixedWidth(460);
    m_summary->hide();

    auto *sl = new QVBoxLayout(m_summary);
    sl->setContentsMargins(24, 18, 24, 18);
    sl->setSpacing(8);

    QLabel *done = new QLabel("✔  Emprunt terminé");
    done->setObjectName("done");
    sl->addWidget(done);

    m_summaryText = new QLabel;
    m_summaryText->setTextFormat(Qt::RichText);
    m_summaryText->setWordWrap(true);
    sl->addWidget(m_summaryText);

    center->addWidget(form, 0, Qt::AlignHCenter);
    center->addWidget(m_summary, 0, Qt::AlignHCenter);

    root->addStretch(1);
    root->addLayout(center);
    root->addStretch(1);

    // ----- Connexions -----
    connect(m_category, &QComboBox::currentIndexChanged, this, [this](int) {
        fillMaterials(currentCategoryId(), 0);
    });
    connect(m_matricule, &QLineEdit::textChanged, this, [this](const QString &) {
        updateBorrower();
    });
    connect(ok, &QPushButton::clicked, this, &EmpruntsPage::onSubmit);

    fillCategories(0);
    fillMaterials(0, 0);
}

void EmpruntsPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    // Recharge les listes en gardant la sélection en cours
    const int cat = currentCategoryId();
    const int mat = currentMaterialId();
    fillCategories(cat);
    fillMaterials(cat, mat);
}

void EmpruntsPage::fillCategories(int selectId)
{
    m_category->blockSignals(true);
    m_category->clear();
    m_category->addItem("Choisir une catégorie", 0);

    for (const Category &c : m_store->listCategories())
        m_category->addItem(c.libelle, c.id);

    const int idx = m_category->findData(selectId);
    m_category->setCurrentIndex(idx < 0 ? 0 : idx);
    m_category->blockSignals(false);
}

void EmpruntsPage::fillMaterials(int categoryId, int selectId)
{
    m_material->blockSignals(true);
    m_material->clear();
    m_material->addItem("Choisir un matériel", 0);

    if (categoryId > 0) {
        for (const Material &m : m_store->listAvailableMaterials(categoryId))
            m_material->addItem(m.designation + "  —  " + m.numeroSerie, m.id);
    }

    const int idx = m_material->findData(selectId);
    m_material->setCurrentIndex(idx < 0 ? 0 : idx);
    m_material->blockSignals(false);
}

int EmpruntsPage::currentCategoryId() const
{
    return m_category->currentData().toInt();
}

int EmpruntsPage::currentMaterialId() const
{
    return m_material->currentData().toInt();
}

void EmpruntsPage::prefillMaterial(int materielId)
{
    const int catId = m_store->categoryIdOfMaterial(materielId);

    fillCategories(catId);
    fillMaterials(catId, materielId);
    m_matricule->setFocus();
}

void EmpruntsPage::updateBorrower()
{
    const QString matricule = m_matricule->text().trimmed();

    if (matricule.isEmpty()) {
        m_nom->clear();
        m_prenom->clear();
        m_borrowerStatus->clear();
        return;
    }

    const auto st = m_store->findStudent(matricule);

    if (st) {
        m_nom->setText(st->nom);
        m_prenom->setText(st->prenom);
        m_borrowerStatus->setText("✔  Emprunteur inscrit");
        m_borrowerStatus->setStyleSheet("color: #4ade80; font-size: 11px;");
    } else {
        m_nom->clear();
        m_prenom->clear();
        m_borrowerStatus->setText("L'étudiant n'est pas encore inscrit");
        m_borrowerStatus->setStyleSheet("color: #fb923c; font-size: 11px;");
    }
}

void EmpruntsPage::resetForm()
{
    m_matricule->clear();     // efface aussi nom et prénom
    fillCategories(0);
    fillMaterials(0, 0);
}

void EmpruntsPage::onSubmit()
{
    m_summary->hide();

    const int catId         = currentCategoryId();
    const int matId         = currentMaterialId();
    const QString matricule = m_matricule->text().trimmed();
    const QDate start       = m_start->date();
    const QDate end         = m_end->date();

    if (matricule.isEmpty()) {
        QMessageBox::warning(this, "Champs manquants",
                             "Saisissez le matricule de l'emprunteur.");
        return;
    }

    // 1. L'étudiant est-il inscrit ? (contrôle en premier)
    const auto st = m_store->findStudent(matricule);
    if (!st) {
        QMessageBox::information(this, "Étudiant non inscrit",
                                 QString("L'étudiant avec le matricule %1 n'est pas encore "
                                         "inscrit.\n\nVous allez être redirigé vers la page "
                                         "Étudiants pour l'inscrire.").arg(matricule));
        emit navigateToStudents();
        return;
    }

    if (catId == 0 || matId == 0) {
        QMessageBox::warning(this, "Champs manquants",
                             "Choisissez une catégorie, puis un matériel.");
        return;
    }

    if (end < start) {
        QMessageBox::warning(this, "Dates incorrectes",
                             "La date de retour doit être postérieure à la date d'emprunt.");
        return;
    }

    // 2. Le matériel existe-t-il et est-il disponible ?
    const auto mat = m_store->findMaterialById(matId);
    if (!mat) {
        QMessageBox::warning(this, "Matériel introuvable",
                             "Ce matériel n'existe plus dans la base.");
        return;
    }

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

    // 3. A-t-il un prêt en retard ?
    if (m_store->hasLateLoan(st->id)) {
        QMessageBox::warning(this, "Emprunt impossible",
                             QString("%1 %2 a un prêt en retard. Il doit rendre ce matériel "
                                     "avant de pouvoir emprunter à nouveau.")
                                 .arg(st->prenom, st->nom));
        return;
    }

    // 4. A-t-il déjà un matériel non rendu ?
    if (m_store->hasActiveLoan(st->id)) {
        QMessageBox::warning(this, "Emprunt impossible",
                             QString("%1 %2 a déjà un matériel non rendu. Il doit le rendre "
                                     "avant de pouvoir emprunter autre chose.")
                                 .arg(st->prenom, st->nom));
        return;
    }

    // 5. Enregistrement
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
    resetForm();
    emit loanCreated(mat->designation);
}

void EmpruntsPage::showSummary(const QString &material, const QString &who,
                               const QString &matricule, const QDate &start, const QDate &end)
{
    m_summaryText->setText(QString(
                               "<b>Matériel :</b> %1<br>"
                               "<b>Emprunteur :</b> %2 (%3)<br>"
                               "<b>Date d'emprunt :</b> %4<br>"
                               "<b>Date de retour prévue :</b> %5")
                               .arg(material, who, matricule,
                                    start.toString("dd/MM/yyyy"), end.toString("dd/MM/yyyy")));

    m_summary->show();
}