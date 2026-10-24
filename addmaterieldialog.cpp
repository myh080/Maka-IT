#include "addmaterieldialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog>
#include <QPixmap>
#include <QMessageBox>

AddMaterielDialog::AddMaterielDialog(const QList<Category> &categories, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Nouveau matériel");
    setModal(true);
    setMinimumWidth(380);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(22, 20, 22, 20);
    root->setSpacing(12);

    QLabel *title = new QLabel("Ajouter un matériel");
    title->setObjectName("dlgTitle");
    root->addWidget(title);

    m_preview = new QLabel("Aucune photo");
    m_preview->setObjectName("preview");
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setFixedSize(320, 140);
    root->addWidget(m_preview, 0, Qt::AlignHCenter);

    QPushButton *pick = new QPushButton("Choisir une photo");
    pick->setObjectName("secondary");
    pick->setCursor(Qt::PointingHandCursor);
    connect(pick, &QPushButton::clicked, this, &AddMaterielDialog::choosePhoto);
    root->addWidget(pick);

    QLabel *lDes = new QLabel("Désignation");
    lDes->setObjectName("field");
    root->addWidget(lDes);
    m_designation = new QLineEdit;
    m_designation->setPlaceholderText("Ex : Arduino Uno R3");
    root->addWidget(m_designation);

    QLabel *lSer = new QLabel("Numéro de série");
    lSer->setObjectName("field");
    root->addWidget(lSer);
    m_serial = new QLineEdit;
    m_serial->setPlaceholderText("Ex : A0001-00999");
    root->addWidget(m_serial);

    QLabel *lCat = new QLabel("Catégorie");
    lCat->setObjectName("field");
    root->addWidget(lCat);
    m_cat = new QComboBox;
    for (const Category &c : categories)
        m_cat->addItem(c.libelle, c.id);
    root->addWidget(m_cat);

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
    root->addLayout(buttons);

    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(ok, &QPushButton::clicked, this, &AddMaterielDialog::validateAndAccept);
}

QString AddMaterielDialog::designation() const
{
    return m_designation->text().trimmed();
}

QString AddMaterielDialog::numeroSerie() const
{
    return m_serial->text().trimmed();
}

int AddMaterielDialog::categoryId() const
{
    return m_cat->currentData().toInt();
}

QString AddMaterielDialog::photoPath() const
{
    return m_photo;
}

void AddMaterielDialog::validateAndAccept()
{
    if (designation().isEmpty() || numeroSerie().isEmpty()) {
        QMessageBox::warning(this, "Champs requis",
                             "Veuillez saisir la désignation et le numéro de série.");
        return;
    }
    accept();
}

void AddMaterielDialog::choosePhoto()
{
    const QString file = QFileDialog::getOpenFileName(this, "Choisir une photo", QString(),
                                                      "Images (*.png *.jpg *.jpeg *.bmp)");
    if (file.isEmpty())
        return;

    QPixmap pm(file);
    if (pm.isNull()) {
        QMessageBox::warning(this, "Image invalide", "Impossible de charger cette image.");
        return;
    }

    m_photo = file;
    m_preview->setPixmap(pm.scaled(316, 136, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}