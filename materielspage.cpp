#include "materielspage.h"
#include "addmaterieldialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QScrollArea>
#include <QFrame>
#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include <QPen>
#include <QPointF>
#include <QColor>
#include <QEvent>
#include <QShowEvent>
#include <QMessageBox>
#include <QStyle>
#include <utility>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>

namespace {

constexpr int COLUMNS = 3;

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

MaterielsPage::MaterielsPage(DataStore *store, QWidget *parent)
    : QWidget(parent)
    , m_store(store)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(16);

    // ----- En-tête -----
    auto *head = new QHBoxLayout;

    auto *titles = new QVBoxLayout;
    QLabel *title = new QLabel("Matériels");
    title->setObjectName("title");
    QLabel *sub = new QLabel("Consultez et gérez tous les matériels informatiques de l'établissement.");
    sub->setObjectName("subtitle");
    titles->addWidget(title);
    titles->addWidget(sub);
    head->addLayout(titles, 1);

    m_search = new QLineEdit;
    m_search->setObjectName("search");
    m_search->setPlaceholderText("Rechercher un matériel...");
    m_search->setFixedSize(280, 40);
    m_search->setStyleSheet("QLineEdit { padding-left: 40px; }");
    head->addWidget(m_search);

    QLabel *searchIco = new QLabel(m_search);
    searchIco->setPixmap(searchIcon(QColor("#94a3b8")).pixmap(QSize(18, 18)));
    searchIco->setGeometry(12, 11, 18, 18);
    searchIco->setStyleSheet("background: transparent;");
    searchIco->setAttribute(Qt::WA_TransparentForMouseEvents);

    QPushButton *addBtn = new QPushButton("+  Ajouter un matériel");
    addBtn->setObjectName("addBtn");
    addBtn->setCursor(Qt::PointingHandCursor);
    head->addWidget(addBtn);
    root->addLayout(head);

    // ----- Corps : catégories à gauche, grille à droite -----
    auto *body = new QHBoxLayout;
    body->setSpacing(16);

    QFrame *panel = new QFrame;
    panel->setObjectName("panel");
    panel->setFixedWidth(240);
    auto *pl = new QVBoxLayout(panel);
    pl->setContentsMargins(12, 14, 12, 14);

    QLabel *catTitle = new QLabel("Catégories");
    catTitle->setObjectName("panelTitle");
    pl->addWidget(catTitle);

    m_cats = new QListWidget;
    m_cats->setObjectName("cats");
    m_cats->setFrameShape(QFrame::NoFrame);
    pl->addWidget(m_cats, 1);
    body->addWidget(panel);

    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->viewport()->setAutoFillBackground(false);

    m_gridHost = new QWidget;
    m_gridHost->setObjectName("gridHost");
    m_grid = new QGridLayout(m_gridHost);
    m_grid->setContentsMargins(0, 0, 0, 0);
    m_grid->setSpacing(14);
    m_grid->setAlignment(Qt::AlignTop);
    scroll->setWidget(m_gridHost);
    body->addWidget(scroll, 1);

    root->addLayout(body, 1);

    // ----- Connexions -----
    connect(m_cats, &QListWidget::currentRowChanged, this, [this](int row) {
        selectCategory(row);
    });
    connect(m_search, &QLineEdit::textChanged, this, [this](const QString &) {
        loadMaterials();
    });
    connect(addBtn, &QPushButton::clicked, this, &MaterielsPage::onAddClicked);

    loadCategories();
}

void MaterielsPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // Recharge à chaque affichage : un emprunt fait ailleurs apparaît ici
    loadCategories();
}

void MaterielsPage::loadCategories()
{
    const int keepId = m_currentCat;

    m_categories = m_store->listCategories();

    Category all;
    all.id = 0;
    all.libelle = "Tous";
    for (const Category &c : std::as_const(m_categories))
        all.count += c.count;
    m_categories.prepend(all);

    m_cats->blockSignals(true);
    m_cats->clear();
    m_catRows.clear();

    for (const Category &c : std::as_const(m_categories)) {
        QListWidgetItem *li = new QListWidgetItem(m_cats);
        li->setSizeHint(QSize(0, 42));

        QFrame *row = new QFrame;
        row->setObjectName("catRow");
        row->setAttribute(Qt::WA_TransparentForMouseEvents);

        auto *rl = new QHBoxLayout(row);
        rl->setContentsMargins(12, 0, 12, 0);

        QLabel *name = new QLabel(c.libelle);
        QLabel *count = new QLabel(QString::number(c.count));
        count->setObjectName("catCount");
        rl->addWidget(name, 1);
        rl->addWidget(count);

        m_cats->setItemWidget(li, row);
        m_catRows << row;
    }

    int row = 0;
    for (int i = 0; i < m_categories.size(); ++i) {
        if (m_categories[i].id == keepId) {
            row = i;
            break;
        }
    }
    m_cats->setCurrentRow(row);
    m_cats->blockSignals(false);

    selectCategory(row);
}

void MaterielsPage::selectCategory(int row)
{
    if (row < 0 || row >= m_categories.size())
        return;

    m_currentCat = m_categories[row].id;

    for (int i = 0; i < m_catRows.size(); ++i) {
        QFrame *r = m_catRows[i];
        r->setProperty("selected", i == row);
        r->style()->unpolish(r);
        r->style()->polish(r);
    }

    loadMaterials();
}

void MaterielsPage::loadMaterials()
{
    for (QFrame *card : std::as_const(m_cards)) {
        m_grid->removeWidget(card);
        card->deleteLater();
    }
    m_cards.clear();

    m_rows = m_store->listMaterials(m_currentCat, m_search->text());

    int row = 0;
    int col = 0;
    for (int i = 0; i < m_rows.size(); ++i) {
        QFrame *card = makeCard(m_rows[i], i);
        m_grid->addWidget(card, row, col);
        m_cards << card;

        if (++col == COLUMNS) {
            col = 0;
            ++row;
        }
    }
}

QFrame *MaterielsPage::makeCard(const MaterialRow &row, int index)
{
    QFrame *card = new QFrame(m_gridHost);
    card->setObjectName("matCard");
    card->setMinimumWidth(200);
    card->setProperty("index", index);
    card->installEventFilter(this);

    auto *lay = new QVBoxLayout(card);
    lay->setContentsMargins(14, 12, 14, 12);
    lay->setSpacing(8);

    // En-tête : catégorie, avertissement si endommagé
    auto *head = new QHBoxLayout;
    QLabel *badge = new QLabel(row.libelle);
    badge->setObjectName("catBadge");
    head->addWidget(badge);
    head->addStretch();
    if (row.etat == "endommage") {
        QLabel *warn = new QLabel("⚠");
        warn->setObjectName("warn");
        head->addWidget(warn);
    }
    lay->addLayout(head);

    // Image : photo, ou initiale en attendant
    QFrame *imageBox = new QFrame;
    imageBox->setObjectName("imageBox");
    imageBox->setFixedHeight(110);
    auto *il = new QVBoxLayout(imageBox);
    il->setContentsMargins(6, 6, 6, 6);

    QLabel *img = new QLabel;
    img->setAlignment(Qt::AlignCenter);
    const QPixmap pm(row.photo);
    if (!row.photo.isEmpty() && !pm.isNull()) {
        img->setPixmap(pm.scaled(180, 98, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        img->setText(row.designation.left(1).toUpper());
        img->setStyleSheet("font-size: 40px; font-weight: bold;"
                           " color: rgba(125, 211, 252, 170); background: transparent;");
    }
    il->addWidget(img);
    lay->addWidget(imageBox);

    QLabel *name = new QLabel(row.designation);
    name->setObjectName("matName");
    name->setWordWrap(true);
    QLabel *serial = new QLabel("N° série : " + row.numeroSerie);
    serial->setObjectName("matSerial");
    lay->addWidget(name);
    lay->addWidget(serial);

    // Etat réel, lu dans la base
    QString color;
    QString text;
    if (row.etat == "emprunte") {
        color = "#fb923c";
        text = "Emprunté";
    } else if (row.etat == "endommage") {
        color = "#f87171";
        text = "Endommagé";
    } else {
        color = "#4ade80";
        text = "Disponible";
    }

    auto *statusRow = new QHBoxLayout;
    QLabel *dot = new QLabel;
    dot->setFixedSize(9, 9);
    dot->setStyleSheet(QString("background-color: %1; border-radius: 4px;").arg(color));

    QLabel *st = new QLabel(text);
    st->setStyleSheet(QString("color: %1; font-size: 11px;").arg(color));

    statusRow->addWidget(dot);
    statusRow->addSpacing(6);
    statusRow->addWidget(st);
    statusRow->addStretch();
    lay->addLayout(statusRow);

    return card;
}

bool MaterielsPage::eventFilter(QObject *obj, QEvent *ev)
{
    if (!obj->property("index").isValid())
        return QWidget::eventFilter(obj, ev);

    const int index = obj->property("index").toInt();

    if (ev->type() == QEvent::MouseButtonDblClick) {
        onCardDoubleClicked(index);
        return true;
    }

    if (ev->type() == QEvent::ContextMenu) {
        auto *cm = static_cast<QContextMenuEvent *>(ev);
        onCardContextMenu(index, cm->globalPos());
        return true;
    }

    return QWidget::eventFilter(obj, ev);
}

void MaterielsPage::onCardContextMenu(int index, const QPoint &globalPos)
{
    if (index < 0 || index >= m_rows.size())
        return;

    const MaterialRow row = m_rows[index];
    const bool disponible = (row.etat == "disponible");
    const bool endommage  = (row.etat == "endommage");
    const bool emprunte   = (row.etat == "emprunte");

    QMenu menu(this);

    QAction *damage = menu.addAction("Marquer endommagé");
    damage->setEnabled(disponible);

    QAction *repair = menu.addAction("Marquer réparé (redevient disponible)");
    repair->setEnabled(endommage);

    if (emprunte) {
        QAction *info = menu.addAction("Matériel emprunté : retour depuis la page Retours");
        info->setEnabled(false);
    }

    QAction *chosen = menu.exec(globalPos);

    QString newState;
    if (chosen == damage)
        newState = "endommage";
    else if (chosen == repair)
        newState = "disponible";
    else
        return;

    if (!m_store->setMaterialState(row.id, newState)) {
        QMessageBox::critical(this, "Erreur", "Le statut n'a pas pu être modifié.");
        return;
    }

    loadMaterials();
}

void MaterielsPage::onCardDoubleClicked(int index)
{
    if (index < 0 || index >= m_rows.size())
        return;

    const MaterialRow row = m_rows[index];

    if (row.etat == "emprunte") {
        QMessageBox::information(this, "Matériel indisponible",
                                 QString("« %1 » est déjà emprunté.").arg(row.designation));
        return;
    }

    if (row.etat == "endommage") {
        QMessageBox::information(this, "Matériel indisponible",
                                 QString("« %1 » est endommagé et ne peut pas être emprunté.")
                                     .arg(row.designation));
        return;
    }

    const auto answer = QMessageBox::question(
        this, "Emprunter",
        QString("Voulez-vous emprunter « %1 » ?").arg(row.designation),
        QMessageBox::Yes | QMessageBox::No);

    if (answer == QMessageBox::Yes)
        emit emprunterRequested(row.id);
}

void MaterielsPage::onAddClicked()
{
    // « Tous » n'est pas une vraie catégorie : on le retire de la liste proposée
    QList<Category> cats = m_categories;
    cats.removeFirst();

    AddMaterielDialog dlg(cats, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    if (!m_store->addMaterial(dlg.designation(), dlg.numeroSerie(),
                              dlg.categoryId(), dlg.photoPath())) {
        QMessageBox::warning(this, "Ajout impossible",
                             "Ce numéro de série existe déjà ou l'enregistrement a échoué.");
        return;
    }

    loadCategories();
}