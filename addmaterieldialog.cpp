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

namespace {
const char *DIALOG_QSS = R"CSS(
QDialog { background-color: #0b0f17; border: 1px solid #38bdf8; }
QLabel { color: #e5e7eb; background: transparent; }
QLabel#dlgTitle { font-size: 18px; font-weight: bold; color: white; }
QLabel#preview {
    background-color: rgba(56, 189, 248, 18);
    border: 1px dashed #38bdf8;
    border-radius: 10px;
    color: #94a3b8;
}
QLineEdit, QComboBox {
    background-color: rgba(255, 255, 255, 15);
    border: 1px solid rgba(56, 189, 248, 90);
    border-radius: 8px;
    padding: 7px 10px;
    color: white;
}
QComboBox QAbstractItemView {
    background-color: #0b0f17;
    color: white;
    selection-background-color: rgba(56, 189, 248, 80);
}
QPushButton#secondary {
    background: transparent;
    color: #cbd5e1;
    border: 1px solid rgba(255, 255, 255, 50);
    border-radius: 8px;
    padding: 8px 14px;
}
QPushButton#primary {
    background-color: rgba(56, 189, 248, 70);
    color: white;
    border: 1px solid #38bdf8;
    border-radius: 8px;
    padding: 8px 14px;
    font-weight: bold;
}
)CSS";
}

AddMaterielDialog::AddMaterielDialog(const QStringList &categories, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Nouveau matériel");
    setModal(true);
    setMinimumWidth(380);
    setStyleSheet(DIALOG_QSS);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(22, 20, 22, 20);
    root->setSpacing(12);

    QLabel *title = new QLabel("Ajouter un matériel");
    title->setObjectName("dlgTitle");
    root->addWidget(title);

    // Zone photo
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

    // Nom
    root->addWidget(new QLabel("Nom"));
    m_name = new QLineEdit;
    m_name->setPlaceholderText("Ex : Arduino Uno R3");
    root->addWidget(m_name);

    // Catégorie
    root->addWidget(new QLabel("Catégorie"));
    m_cat = new QComboBox;
    m_cat->addItems(categories);
    root->addWidget(m_cat);

    // Boutons
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
    connect(ok, &QPushButton::clicked, this, [this] {
        if (m_name->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Champ requis", "Veuillez saisir le nom du matériel.");
            return;
        }
        accept();
    });
}

QString AddMaterielDialog::name() const
{
    return m_name->text().trimmed();
}

QString AddMaterielDialog::category() const
{
    return m_cat->currentText();
}

QString AddMaterielDialog::photoPath() const
{
    return m_photo;
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