#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "zoombackground.h"
#include "toggleswitch.h"
#include "datastore.h"
#include "dashboardpage.h"
#include "materielspage.h"
#include "studentspage.h"
#include "empruntspage.h"
#include "retourspage.h"
#include "theme.h"

#include <QStackedWidget>
#include <QPushButton>
#include <QResizeEvent>
#include <QEvent>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QColor>
#include <QDebug>

namespace {

constexpr int SIDEBAR_WIDTH = 220;
constexpr int TOP_BAR = 70;
const QString ICON_DIR = "H:/Qt project/Maka_IT/";

// Image du projet, colorée en gardant la transparence du PNG
QPixmap loadTinted(const QString &file, const QColor &color)
{
    QPixmap src(ICON_DIR + file);
    if (src.isNull()) {
        qDebug() << "Image introuvable :" << ICON_DIR + file;
        return src;
    }

    QPixmap out(src.size());
    out.fill(Qt::transparent);

    QPainter p(&out);
    p.drawPixmap(0, 0, src);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(out.rect(), color);
    p.end();

    return out;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    Theme::setDark(true);
    ui->setupUi(this);
    setWindowTitle("Gestion des emprunts de matériels informatiques");

    // Taille minimale de la fenêtre : plus petite que celle du .ui (1100 x 700)
    setMinimumSize(800, 560);

    // La base est ouverte avant les pages, qui en ont besoin
    m_store = new DataStore;

    const QColor accent(135, 206, 235);

    ui->logoLabel->setPixmap(loadTinted("departement-it.png", accent));
    ui->logoLabel->setScaledContents(true);
    ui->logoName->setStyleSheet(
        "font-family: 'Times New Roman', Georgia;"
        "font-size: 14pt;"
        "color: #e5e7eb;"
        "background: transparent;");

    auto setNavIcon = [&](QPushButton *b, const QString &file) {
        b->setIcon(QIcon(loadTinted(file, accent)));
        b->setIconSize(QSize(20, 20));
    };
    setNavIcon(ui->NavDashBoard, "lanalyse-des-donnees.png");
    setNavIcon(ui->NavMateriels, "ordinateur-personnel.png");
    setNavIcon(ui->NavEtudiant,  "etudiant.png");
    setNavIcon(ui->NavEmprunts,  "chaine-de-produits.png");
    setNavIcon(ui->NavRetours,   "boite-de-retour.png");

    ui->zoomBg->setMapImage(ICON_DIR + "typo.jpg");

    // ----- Pages : la position correspond au numéro de page -----
    m_pages = new QStackedWidget(ui->zoomBg);

    m_dashboard = new DashboardPage(m_store);
    m_pages->addWidget(m_dashboard);                    // 0 : tableau de bord

    MaterielsPage *materiels = new MaterielsPage(m_store);
    m_pages->addWidget(materiels);                      // 1 : matériels

    m_pages->addWidget(new StudentsPage(m_store));      // 2 : étudiants

    m_emprunts = new EmpruntsPage(m_store);
    m_pages->addWidget(m_emprunts);                     // 3 : emprunts

    m_pages->addWidget(new RetoursPage(m_store));       // 4 : retours

    // Au départ, le menu, les pages et l'interrupteur sont cachés
    ui->sidebar->hide();
    m_pages->hide();
    ui->toggleTheme->hide();

    // ----- Connexions -----
    connect(ui->zoomBg, &ZoomBackground::dashboardVisible,
            this, &MainWindow::setDashboardVisible);

    connect(ui->toggleTheme, &QAbstractButton::toggled, this, [this](bool on) {
        Theme::setDark(!on);
        ui->zoomBg->setInverted(on);
        for (QWidget *w : findChildren<QWidget *>())
            w->update();
    });

    connect(materiels, &MaterielsPage::emprunterRequested, this, [this](int materielId) {
        m_emprunts->prefillMaterial(materielId);
        showPage(3);
    });

    connect(m_emprunts, &EmpruntsPage::navigateToStudents, this, [this] {
        showPage(2);
    });

    connect(m_dashboard, &DashboardPage::pageRequested, this, [this](int page) {
        showPage(page);
    });

    // La molette ne zoome pas quand elle est sur le menu ou sur une page
    m_pages->installEventFilter(this);
    ui->sidebar->installEventFilter(this);

    // ----- Navigation -----
    m_navs = { ui->NavDashBoard, ui->NavMateriels, ui->NavEtudiant,
              ui->NavEmprunts,  ui->NavRetours };

    for (int i = 0; i < m_navs.size(); ++i) {
        m_navs[i]->setCheckable(true);
        connect(m_navs[i], &QPushButton::clicked, this, [this, i] { showPage(i); });
    }

    showPage(0);
    layoutInside();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_store;
}

void MainWindow::setDashboardVisible(bool show)
{
    if (show) {
        ui->sidebar->show();
        m_pages->show();
        ui->toggleTheme->show();
        ui->toggleTheme->raise();
        m_dashboard->playEntrance();
    } else {
        ui->sidebar->hide();
        m_pages->hide();
        ui->toggleTheme->hide();
    }
}

void MainWindow::showPage(int index)
{
    m_pages->setCurrentIndex(index);

    for (int i = 0; i < m_navs.size(); ++i)
        m_navs[i]->setChecked(i == index);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (!m_pages)
        return;

    ui->zoomBg->setGeometry(ui->centralwidget->rect());
    ui->zoomBg->lower();
    layoutInside();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *ev)
{
    if (ev->type() == QEvent::Wheel && (obj == m_pages || obj == ui->sidebar))
        return true;

    return QMainWindow::eventFilter(obj, ev);
}

void MainWindow::layoutInside()
{
    const int W = ui->zoomBg->width();
    const int H = ui->zoomBg->height();

    ui->sidebar->setGeometry(0, 0, SIDEBAR_WIDTH, H);
    m_pages->setGeometry(SIDEBAR_WIDTH, TOP_BAR, W - SIDEBAR_WIDTH, H - TOP_BAR);

    ui->toggleTheme->move(W - ui->toggleTheme->width() - 25, 25);
    ui->toggleTheme->raise();
}