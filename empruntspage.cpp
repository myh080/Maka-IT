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
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QPen>
#include <QColor>
#include <QRectF>
#include <QPointF>

namespace {

// Les deux cartes ont exactement la même taille
constexpr int CARD_W = 400;
constexpr int CARD_H = 540;

// Icône de libellé dessinée en code, dans une couleur violette
QIcon fieldIcon(const QString &name)
{
    QPixmap pm(32, 32);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor("#a78bfa"), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    if (name == "box") {
        p.drawRoundedRect(QRectF(6, 8, 20, 18), 3, 3);
        p.drawLine(QPointF(6, 14), QPointF(26, 14));
    } else if (name == "chip") {
        p.drawRoundedRect(QRectF(9, 9, 14, 14), 2, 2);
        p.drawLine(QPointF(13, 4), QPointF(13, 9));
        p.drawLine(QPointF(19, 4), QPointF(19, 9));
        p.drawLine(QPointF(13, 23), QPointF(13, 28));
        p.drawLine(QPointF(19, 23), QPointF(19, 28));
    } else if (name == "user") {
        p.drawEllipse(QPointF(16, 11), 5, 5);
        p.drawArc(QRectF(7, 18, 18, 12), 0, 180 * 16);
    } else if (name == "users") {
        p.drawEllipse(QPointF(13, 11), 4, 4);
        p.drawArc(QRectF(5, 18, 16, 10), 0, 180 * 16);
        p.drawArc(QRectF(18, 17, 10, 10), 0, 180 * 16);
    } else if (name == "calendar") {
        p.drawRoundedRect(QRectF(6, 8, 20, 18), 3, 3);
        p.drawLine(QPointF(6, 14), QPointF(26, 14));
        p.drawLine(QPointF(11, 5), QPointF(11, 10));
        p.drawLine(QPointF(21, 5), QPointF(21, 10));
    }

    p.end();
    return QIcon(pm);
}

// Libellé avec son icône à gauche
QWidget *iconLabel(const QString &icon, const QString &text)
{
    QWidget *w = new QWidget;
    w->setStyleSheet("background: transparent;");

    auto *h = new QHBoxLayout(w);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(8);

    QLabel *ic = new QLabel;
    ic->setPixmap(fieldIcon(icon).pixmap(QSize(20, 20)));
    ic->setFixedSize(22, 22);

    QLabel *t = new QLabel(text);
    t->setObjectName("field");

    h->addWidget(ic);
    h->addWidget(t);
    h->addStretch();
    return w;
}

// Bloc du récapitulatif : un titre petit, puis une valeur en dessous
QFrame *infoBlock(const QString &title, QLabel *&valueOut)
{
    QFrame *block = new QFrame;
    block->setObjectName("card");

    auto *lay = new QVBoxLayout(block);
    lay->setContentsMargins(16, 12, 16, 12);
    lay->setSpacing(6);

    QLabel *t = new QLabel(title);
    t->setObjectName("field");
    lay->addWidget(t);

    valueOut = new QLabel;
    valueOut->setTextFormat(Qt::RichText);
    valueOut->setWordWrap(true);
    lay->addWidget(valueOut);

    return block;
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

    // ----- Carte gauche : formulaire -----
    QFrame *form = new QFrame;
    form->setObjectName("form");
    form->setFixedSize(CARD_W, CARD_H);

    auto *fl = new QVBoxLayout(form);
    fl->setContentsMargins(24, 20, 24, 20);
    fl->setSpacing(8);

    QLabel *ft = new QLabel("Nouvel emprunt");
    ft->setObjectName("formTitle");
    fl->addWidget(ft);
    fl->addSpacing(4);

    // Nom et Prénoms : en haut du formulaire
    fl->addWidget(iconLabel("user", "Nom :"));
    m_nom = new QLineEdit;
    m_nom->setPlaceholderText("Ex : Rakotomamonjy");
    m_nom->setFixedHeight(40);
    fl->addWidget(m_nom);

    fl->addWidget(iconLabel("users", "Prénoms :"));
    m_prenom = new QLineEdit;
    m_prenom->setPlaceholderText("Ex : Sarah");
    m_prenom->setFixedHeight(40);
    fl->addWidget(m_prenom);

    m_borrowerStatus = new QLabel;
    m_borrowerStatus->setObjectName("field");
    m_borrowerStatus->setMinimumHeight(16);
    fl->addWidget(m_borrowerStatus);

    // Catégorie de matériel
    fl->addWidget(iconLabel("box", "Catégorie de matériel"));
    m_category = new QComboBox;
    m_category->setFixedHeight(40);
    fl->addWidget(m_category);

    // Matériel disponible
    fl->addWidget(iconLabel("chip", "Matériel disponible"));
    m_material = new QComboBox;
    m_material->setFixedHeight(40);
    fl->addWidget(m_material);

    // Dates côte à côte, chacune avec son icône
    auto *dates = new QHBoxLayout;
    dates->setSpacing(12);

    auto *startCol = new QVBoxLayout;
    startCol->setSpacing(8);
    startCol->addWidget(iconLabel("calendar", "Date d'emprunt"));
    m_start = new QDateEdit(QDate::currentDate());
    m_start->setCalendarPopup(true);
    m_start->setDisplayFormat("dd/MM/yyyy");
    m_start->setFixedHeight(40);
    startCol->addWidget(m_start);
    dates->addLayout(startCol, 1);

    auto *endCol = new QVBoxLayout;
    endCol->setSpacing(8);
    endCol->addWidget(iconLabel("calendar", "Date de retour prévue"));
    m_end = new QDateEdit(QDate::currentDate().addDays(7));
    m_end->setCalendarPopup(true);
    m_end->setDisplayFormat("dd/MM/yyyy");
    m_end->setFixedHeight(40);
    endCol->addWidget(m_end);
    dates->addLayout(endCol, 1);

    fl->addLayout(dates);
    fl->addStretch(1);

    // Bouton pleine largeur, en bas
    QPushButton *ok = new QPushButton("Enregistrer l'emprunt");
    ok->setObjectName("primary");
    ok->setCursor(Qt::PointingHandCursor);
    ok->setFixedHeight(44);
    fl->addWidget(ok);

    // ----- Carte droite : récapitulatif (caché jusqu'à un emprunt réussi) -----
    m_summary = new QFrame;
    m_summary->setObjectName("summary");
    m_summary->setFixedSize(CARD_W, CARD_H);

    auto *sl = new QVBoxLayout(m_summary);
    sl->setContentsMargins(24, 20, 24, 20);
    sl->setSpacing(12);

    QLabel *sumTitle = new QLabel("Récapitulatif");
    sumTitle->setAlignment(Qt::AlignCenter);
    sumTitle->setStyleSheet("font-size: 22px; font-weight: bold;");
    sl->addWidget(sumTitle);
    sl->addSpacing(6);

    sl->addWidget(infoBlock("Matériel", m_sumMaterial));
    sl->addWidget(infoBlock("Emprunteur", m_sumBorrower));
    sl->addWidget(infoBlock("Dates", m_sumDates));

    sl->addStretch(1);
    QPushButton *close = new QPushButton("Fermer");
    close->setObjectName("secondary");
    close->setCursor(Qt::PointingHandCursor);
    close->setFixedHeight(40);
    sl->addWidget(close);
    connect(close, &QPushButton::clicked, this, [this] { m_summary->hide(); });
    m_summary->hide();

    // ----- Disposition : formulaire au centre, récapitulatif à droite après l'emprunt -----
    auto *cards = new QHBoxLayout;
    cards->setSpacing(20);
    cards->addStretch(1);
    cards->addWidget(form);
    cards->addWidget(m_summary);
    cards->addStretch(1);

    root->addStretch(1);
    root->addLayout(cards);
    root->addStretch(1);

    // ----- Connexions -----
    connect(m_category, &QComboBox::currentIndexChanged, this, [this](int) {
        fillMaterials(currentCategoryId(), 0);
    });
    connect(m_nom, &QLineEdit::textChanged, this, [this](const QString &) {
        updateBorrower();
    });
    connect(m_prenom, &QLineEdit::textChanged, this, [this](const QString &) {
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
    m_nom->setFocus();
}

void EmpruntsPage::updateBorrower()
{
    const QString nom    = m_nom->text().trimmed();
    const QString prenom = m_prenom->text().trimmed();

    // Le message n'apparaît qu'une fois les deux champs remplis
    if (nom.isEmpty() || prenom.isEmpty()) {
        m_borrowerStatus->clear();
        return;
    }

    const auto st = m_store->findStudentByName(nom, prenom);

    if (st) {
        m_borrowerStatus->setText("✔  Étudiant inscrit (matricule " + st->matricule + ")");
        m_borrowerStatus->setStyleSheet("color: #4ade80; font-size: 11px;");
    } else {
        m_borrowerStatus->setText("Cet étudiant n'est pas encore inscrit");
        m_borrowerStatus->setStyleSheet("color: #fb923c; font-size: 11px;");
    }
}

void EmpruntsPage::showSummary(const QString &material, const QString &numeroSerie,
                               const QString &nomPrenom, const QString &matricule,
                               const QDate &start, const QDate &end)
{
    m_sumMaterial->setText(QString("<b>%1</b><br>N° série : %2").arg(material, numeroSerie));
    m_sumBorrower->setText(QString("<b>Nom :</b> %1<br><b>Prénoms :</b> %2<br>"
                                   "<b>Matricule :</b> %3")
                               .arg(nomPrenom.section(' ', 0, 0),
                                    nomPrenom.section(' ', 1),
                                    matricule));
    m_sumDates->setText(QString("<b>Emprunt :</b> %1<br><b>Retour prévu :</b> %2")
                            .arg(start.toString("dd/MM/yyyy"), end.toString("dd/MM/yyyy")));

    m_summary->show();
}

void EmpruntsPage::resetForm()
{
    m_nom->clear();          // efface aussi le message de statut
    m_prenom->clear();
    fillCategories(0);
    fillMaterials(0, 0);
}

void EmpruntsPage::onSubmit()
{
    // Le récapitulatif disparaît à chaque nouvelle tentative
    m_summary->hide();

    const int catId      = currentCategoryId();
    const int matId      = currentMaterialId();
    const QString nom    = m_nom->text().trimmed();
    const QString prenom = m_prenom->text().trimmed();
    const QDate start    = m_start->date();
    const QDate end      = m_end->date();

    if (nom.isEmpty() || prenom.isEmpty()) {
        QMessageBox::warning(this, "Champs manquants",
                             "Saisissez le nom et les prénoms de l'emprunteur.");
        return;
    }

    // 1. L'étudiant est-il inscrit ? Sinon : message, puis page Étudiants
    const auto st = m_store->findStudentByName(nom, prenom);
    if (!st) {
        QMessageBox::information(this, "Étudiant non inscrit",
                                 QString("%1 %2 n'est pas encore inscrit.\n\n"
                                         "Vous allez être redirigé vers la page Étudiants "
                                         "pour l'inscrire.").arg(prenom, nom));
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

    // 5. Enregistrement, puis récapitulatif à droite
    Loan loan;
    loan.materielId       = mat->id;
    loan.emprunteurId     = st->id;
    loan.dateEmprunt      = start;
    loan.dateRetourPrevue = end;

    if (!m_store->addLoan(loan)) {
        QMessageBox::critical(this, "Erreur", "L'emprunt n'a pas pu être enregistré.");
        return;
    }

    showSummary(mat->designation, mat->numeroSerie,
                st->nom + " " + st->prenom, st->matricule, start, end);
    resetForm();
    emit loanCreated(mat->designation);
}