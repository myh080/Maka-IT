/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.11.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "toggleswitch.h"
#include "zoombackground.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    ZoomBackground *zoomBg;
    ToggleSwitch *toggleTheme;
    QWidget *sidebar;
    QLabel *logoLabel;
    QLabel *logoName;
    QWidget *verticalLayoutWidget;
    QVBoxLayout *verticalLayout;
    QPushButton *NavDashBoard;
    QPushButton *NavEtudiant;
    QPushButton *NavMateriels;
    QPushButton *NavEmprunts;
    QPushButton *NavRetours;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1350, 700);
        MainWindow->setMinimumSize(QSize(1100, 700));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        zoomBg = new ZoomBackground(centralwidget);
        zoomBg->setObjectName("zoomBg");
        zoomBg->setGeometry(QRect(-10, -30, 1311, 700));
        toggleTheme = new ToggleSwitch(zoomBg);
        toggleTheme->setObjectName("toggleTheme");
        toggleTheme->setGeometry(QRect(1175, 16, 131, 31));
        sidebar = new QWidget(zoomBg);
        sidebar->setObjectName("sidebar");
        sidebar->setGeometry(QRect(0, 0, 220, 700));
        logoLabel = new QLabel(sidebar);
        logoLabel->setObjectName("logoLabel");
        logoLabel->setGeometry(QRect(20, 20, 63, 61));
        logoLabel->setStyleSheet(QString::fromUtf8("backgroundColor : white"));
        logoLabel->setPixmap(QPixmap(QString::fromUtf8("departement-it.png")));
        logoLabel->setScaledContents(true);
        logoName = new QLabel(sidebar);
        logoName->setObjectName("logoName");
        logoName->setGeometry(QRect(100, 40, 101, 31));
        QFont font;
        font.setFamilies({QString::fromUtf8("Viner Hand ITC")});
        font.setPointSize(14);
        logoName->setFont(font);
        verticalLayoutWidget = new QWidget(sidebar);
        verticalLayoutWidget->setObjectName("verticalLayoutWidget");
        verticalLayoutWidget->setGeometry(QRect(40, 139, 160, 371));
        verticalLayout = new QVBoxLayout(verticalLayoutWidget);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        NavDashBoard = new QPushButton(verticalLayoutWidget);
        NavDashBoard->setObjectName("NavDashBoard");
        NavDashBoard->setCursor(QCursor(Qt::CursorShape::OpenHandCursor));
        QIcon icon;
        icon.addFile(QString::fromUtf8("lanalyse-des-donnees.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        NavDashBoard->setIcon(icon);

        verticalLayout->addWidget(NavDashBoard);

        NavEtudiant = new QPushButton(verticalLayoutWidget);
        NavEtudiant->setObjectName("NavEtudiant");
        NavEtudiant->setCursor(QCursor(Qt::CursorShape::OpenHandCursor));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8("etudiant.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        NavEtudiant->setIcon(icon1);

        verticalLayout->addWidget(NavEtudiant);

        NavMateriels = new QPushButton(verticalLayoutWidget);
        NavMateriels->setObjectName("NavMateriels");
        NavMateriels->setCursor(QCursor(Qt::CursorShape::OpenHandCursor));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8("ordinateur-personnel.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        NavMateriels->setIcon(icon2);

        verticalLayout->addWidget(NavMateriels);

        NavEmprunts = new QPushButton(verticalLayoutWidget);
        NavEmprunts->setObjectName("NavEmprunts");
        NavEmprunts->setCursor(QCursor(Qt::CursorShape::OpenHandCursor));
        QIcon icon3;
        icon3.addFile(QString::fromUtf8("chaine-de-produits.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        NavEmprunts->setIcon(icon3);

        verticalLayout->addWidget(NavEmprunts);

        NavRetours = new QPushButton(verticalLayoutWidget);
        NavRetours->setObjectName("NavRetours");
        NavRetours->setCursor(QCursor(Qt::CursorShape::OpenHandCursor));
        QIcon icon4;
        icon4.addFile(QString::fromUtf8("boite-de-retour.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        NavRetours->setIcon(icon4);

        verticalLayout->addWidget(NavRetours);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1350, 25));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "Gestion des emprunts de mat\303\251riels informatiques", nullptr));
        logoLabel->setText(QString());
        logoName->setText(QCoreApplication::translate("MainWindow", "Maka IT", nullptr));
        NavDashBoard->setText(QCoreApplication::translate("MainWindow", "Dashboard", nullptr));
        NavEtudiant->setText(QCoreApplication::translate("MainWindow", "Etudiant", nullptr));
        NavMateriels->setText(QCoreApplication::translate("MainWindow", "Mat\303\251riels", nullptr));
        NavEmprunts->setText(QCoreApplication::translate("MainWindow", "Emprunts", nullptr));
        NavRetours->setText(QCoreApplication::translate("MainWindow", "Retours", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
