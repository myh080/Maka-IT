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
#include <QMessageBox>
#include <QStyle>
#include <utility>

namespace {

constexpr int COLUMNS = 3;

const QStringList CATEGORIES = {
    "Tous", "Ordinateurs", "Écrans", "Claviers", "Souris",
    "Projecteurs", "Imprimantes", "Arduino & µC", "Réseau", "Autres"
};

const char *PAGE_QSS = R"CSS(
QLabel { background: transparent; color: #e5e7eb; }
QLabel#title { font-size: 24px; font-weight: bold; color: white; }
QLabel#subtitle { font-size: 12px; color: #94a3b8; }
QLabel#panelTitle { font-size: 14px; font-weight: bold; color: white; }

QFrame#panel {
    background-color: rgba(10, 10, 14, 190);
    border: 1px solid rgba(56, 189, 248, 80);
    border-radius: 14px;
}
QFrame#catRow { background: transparent; border: 1px solid transparent; border-radius: 8px; }
QFrame#catRow[selected="true"] {
    background-color: rgba(56, 189, 248, 70);
    border: 1px solid #38bdf8;
}
QLabel#catCount { color: #94a3b8; font-size: 12px; }
QListWidget#cats { background: transparent; border: none; outline: none; }
QListWidget#cats::item { background: transparent; border: none; }
QListWidget#cats::item:selected { background: transparent; }
QListWidget#cats::item:hover { background: transparent; }

QFrame#matCard {
    background-color: rgba(10, 10, 14, 200);
    border: 1px solid rgba(56, 189, 248, 90);
    border-radius: 14px;
}
QFrame#matCard:hover { border: 1px solid #38bdf8; }
QFrame#imageBox { background-color: rgba(56, 189, 248, 18); border-radius: 10px; }
QLabel#matName { font-size: 14px; font-weight: bold; color: white; }
QLabel#matSerial { font-size: 11px; color: #94a3b8; }
QLabel#catBadge {
    background-color: rgba(56, 189, 248, 40);
    color: #7dd3fc;
    border-radius: 6px;
    padding: 2px 8px;
    font-size: 10px;
}
QLabel#warn { color: #f87171; font-size: 16px; font-weight: bold; }
QPushButton#more {
    background: transparent;
    color: #94a3b8;
    border: none;
    font-size: 14px;
    font-weight: bold;
}
QPushButton#more:hover { color: white; }

QLineEdit#search {
    background-color: rgba(10, 10, 14, 200);
    border: 1px solid rgba(56, 189, 248, 80);
    border-radius: 10px;
    padding: 8px 12px;
    color: white;
}
QLineEdit#search:focus { border: 1px solid #38bdf8; }

QPushButton#addBtn {
    background-color: rgba(56, 189, 248, 60);
    color: white;
    border: 1px solid #38bdf8;
    border-radius: 10px;
    padding: 9px 16px;
    font-weight: bold;
}
QPushButton#addBtn:hover { background-color: rgba(56, 189, 248, 110); }

QScrollArea { background: transparent; border: none; }
QWidget#gridHost { background: transparent; }
)CSS";

// Icône de recherche dessinée en code (pas de fichier nécessaire)
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

MaterielsPage::MaterielsPage(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);
    setStyleSheet(PAGE_QSS);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(16);

    // ----- En-tête : titre, recherche, bouton -----
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
    m_search->setStyleSheet("QLineEdit { padding-left: 40px; }");   // laisse la place à l'icône
    head->addWidget(m_search);

    // Icône de recherche, posée par-dessus le champ à gauche
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

    // ----- Données (à remplacer plus tard par SQLite) -----
    m_catNames = CATEGORIES;

    addItem({ "Arduino Uno R3",          "A0001-00123", "Arduino & µC", 0, "" });
    addItem({ "Arduino Mega 2560",       "A0001-00456", "Arduino & µC", 1, "" });
    addItem({ "Raspberry Pi 4",          "RPI4-00876",  "Réseau",       0, "" });
    addItem({ "ESP32 DevKit",            "ESP32-11223", "Arduino & µC", 0, "" });
    addItem({ "Arduino Nano",            "A0002-00987", "Arduino & µC", 0, "" });
    addItem({ "Breadboard",              "BB-00145",    "Autres",       0, "" });
    addItem({ "Capteur Ultrason HC-SR04","HC-00321",    "Arduino & µC", 0, "" });
    addItem({ "Servo Moteur SG90",       "SG90-00765",  "Arduino & µC", 2, "" });
    addItem({ "Kit Arduino Complet",     "KIT-00112",   "Arduino & µC", 0, "" });
    addItem({ "Écran OLED 0.96\"",       "OLED-00567",  "Écrans",       0, "" });
    addItem({ "Module Relais 5V",        "REL-00934",   "Autres",       0, "" });
    addItem({ "Caméra ESP32-CAM",        "CAM-00421",   "Autres",       2, "" });

    rebuildCategories();

    // ----- Connexions -----
    connect(m_cats, &QListWidget::currentRowChanged, this, [this](int row) {
        m_currentCat = (row >= 0 && row < m_catNames.size()) ? m_catNames[row] : "Tous";
        updateCatSelection(row);
        refresh();
    });
    connect(m_search, &QLineEdit::textChanged, this, [this](const QString &) {
        refresh();
    });
    connect(addBtn, &QPushButton::clicked, this, &MaterielsPage::onAddClicked);

    m_cats->setCurrentRow(0);
    refresh();
}

void MaterielsPage::addItem(const Item &it)
{
    m_items << it;
    m_cards << makeCard(it, m_items.size() - 1);
}

QFrame *MaterielsPage::makeCard(const Item &it, int index)
{
    QFrame *card = new QFrame(m_gridHost);
    card->setObjectName("matCard");
    card->setMinimumWidth(200);
    card->setProperty("index", index);
    card->installEventFilter(this);     // double-clic géré dans eventFilter

    auto *lay = new QVBoxLayout(card);
    lay->setContentsMargins(14, 12, 14, 12);
    lay->setSpacing(8);

    // En-tête : catégorie, avertissement si endommagé
    auto *head = new QHBoxLayout;
    QLabel *badge = new QLabel(it.category);
    badge->setObjectName("catBadge");
    head->addWidget(badge);
    head->addStretch();
    if (it.status == 2) {
        QLabel *warn = new QLabel("⚠");
        warn->setObjectName("warn");
        head->addWidget(warn);
    }
    lay->addLayout(head);

    // Image : photo choisie, ou initiale en attendant
    QFrame *imageBox = new QFrame;
    imageBox->setObjectName("imageBox");
    imageBox->setFixedHeight(110);
    auto *il = new QVBoxLayout(imageBox);
    il->setContentsMargins(6, 6, 6, 6);

    QLabel *img = new QLabel;
    img->setAlignment(Qt::AlignCenter);
    const QPixmap pm(it.photo);
    if (!it.photo.isEmpty() && !pm.isNull()) {
        img->setPixmap(pm.scaled(180, 98, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        img->setText(it.name.left(1).toUpper());
        img->setStyleSheet("font-size: 40px; font-weight: bold;"
                           " color: rgba(125, 211, 252, 170); background: transparent;");
    }
    il->addWidget(img);
    lay->addWidget(imageBox);

    QLabel *name = new QLabel(it.name);
    name->setObjectName("matName");
    name->setWordWrap(true);
    QLabel *serial = new QLabel("N° série : " + it.serial);
    serial->setObjectName("matSerial");
    lay->addWidget(name);
    lay->addWidget(serial);

    // Ligne d'état : pastille, texte, bouton ...
    QString color;
    QString text;
    switch (it.status) {
    case 1:  color = "#fb923c"; text = "Emprunté";  break;
    case 2:  color = "#f87171"; text = "Endommagé"; break;
    default: color = "#4ade80"; text = "Disponible"; break;
    }

    auto *statusRow = new QHBoxLayout;
    QLabel *dot = new QLabel;
    dot->setFixedSize(9, 9);
    dot->setStyleSheet(QString("background-color: %1; border-radius: 4px;").arg(color));

    QLabel *st = new QLabel(text);
    st->setStyleSheet(QString("color: %1; font-size: 11px;").arg(color));

    QPushButton *more = new QPushButton("...");
    more->setObjectName("more");
    more->setCursor(Qt::PointingHandCursor);

    statusRow->addWidget(dot);
    statusRow->addSpacing(6);
    statusRow->addWidget(st);
    statusRow->addStretch();
    statusRow->addWidget(more);
    lay->addLayout(statusRow);

    return card;
}

void MaterielsPage::rebuildCategories()
{
    const int keep = m_cats->currentRow() < 0 ? 0 : m_cats->currentRow();

    m_cats->blockSignals(true);
    m_cats->clear();
    m_catRows.clear();

    for (const QString &name : std::as_const(m_catNames)) {
        int count = 0;
        for (const Item &it : std::as_const(m_items))
            if (name == "Tous" || it.category == name)
                ++count;

        QListWidgetItem *li = new QListWidgetItem(m_cats);
        li->setSizeHint(QSize(0, 42));

        QFrame *row = new QFrame;
        row->setObjectName("catRow");
        row->setAttribute(Qt::WA_TransparentForMouseEvents);   // le clic passe à la liste

        auto *rl = new QHBoxLayout(row);
        rl->setContentsMargins(12, 0, 12, 0);

        QLabel *n = new QLabel(name);
        QLabel *c = new QLabel(QString::number(count));
        c->setObjectName("catCount");
        rl->addWidget(n, 1);
        rl->addWidget(c);

        m_cats->setItemWidget(li, row);
        m_catRows << row;
    }

    m_cats->blockSignals(false);
    m_cats->setCurrentRow(keep);
    updateCatSelection(keep);
}

void MaterielsPage::updateCatSelection(int row)
{
    for (int i = 0; i < m_catRows.size(); ++i) {
        QFrame *r = m_catRows[i];
        r->setProperty("selected", i == row);
        r->style()->unpolish(r);
        r->style()->polish(r);
    }
}

void MaterielsPage::refresh()
{
    const QString needle = m_search->text().trimmed();

    // Retire toutes les cartes de la grille
    for (QFrame *card : std::as_const(m_cards)) {
        m_grid->removeWidget(card);
        card->hide();
    }

    // Replace celles qui correspondent à la catégorie et à la recherche
    int row = 0;
    int col = 0;
    for (int i = 0; i < m_items.size(); ++i) {
        const Item &it = m_items[i];

        const bool inCat  = (m_currentCat == "Tous") || (it.category == m_currentCat);
        const bool inText = needle.isEmpty()
                            || it.name.contains(needle, Qt::CaseInsensitive)
                            || it.serial.contains(needle, Qt::CaseInsensitive);

        if (!inCat || !inText)
            continue;

        m_grid->addWidget(m_cards[i], row, col);
        m_cards[i]->show();

        if (++col == COLUMNS) {
            col = 0;
            ++row;
        }
    }
}

bool MaterielsPage::eventFilter(QObject *obj, QEvent *ev)
{
    if (ev->type() == QEvent::MouseButtonDblClick && obj->property("index").isValid()) {
        onCardDoubleClicked(obj->property("index").toInt());
        return true;
    }
    return QWidget::eventFilter(obj, ev);
}

void MaterielsPage::onCardDoubleClicked(int index)
{
    if (index < 0 || index >= m_items.size())
        return;

    const Item &it = m_items[index];

    if (it.status == 0) {
        const auto answer = QMessageBox::question(
            this, "Emprunter",
            QString("Voulez-vous emprunter « %1 » ?").arg(it.name),
            QMessageBox::Yes | QMessageBox::No);

        if (answer == QMessageBox::Yes)
            emit emprunterRequested(it.name);
        return;
    }

    const QString reason = (it.status == 2) ? "est endommagé" : "est déjà emprunté";
    QMessageBox::information(
        this, "Matériel indisponible",
        QString("« %1 » %2 et ne peut pas être emprunté pour le moment.").arg(it.name, reason));
}

void MaterielsPage::onAddClicked()
{
    QStringList cats = m_catNames;
    cats.removeFirst();                  // on retire « Tous »

    AddMaterielDialog dlg(cats, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    Item it;
    it.name     = dlg.name();
    it.category = dlg.category();
    it.serial   = QString("MAT-%1").arg(m_items.size() + 1, 5, 10, QChar('0'));
    it.status   = 0;
    it.photo    = dlg.photoPath();

    addItem(it);
    rebuildCategories();
    refresh();
}