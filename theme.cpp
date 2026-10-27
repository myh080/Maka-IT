#include "theme.h"

#include <QApplication>
#include <QString>
#include <QList>
#include <QPair>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QTableWidget>
#include <QHeaderView>

namespace {

bool g_dark = true;

QString buildQss(bool d)
{
    QString s = QString::fromUtf8(R"CSS(
QWidget { color: @TEXT@; font-family: "Segoe UI"; font-size: 13px; }
QDialog { background-color: @DIALOG@; }
QLabel { background: transparent; }

QLabel#title      { font-size: 24px; font-weight: bold; }
QLabel#pageTitle  { font-size: 22px; font-weight: bold; }
QLabel#h1         { font-size: 22px; font-weight: bold; }
QLabel#formTitle  { font-size: 16px; font-weight: bold; }
QLabel#dlgTitle   { font-size: 18px; font-weight: bold; }
QLabel#panelTitle { font-size: 14px; font-weight: bold; }
QLabel#cardTitle  { font-size: 13px; font-weight: 600; }
QLabel#sub, QLabel#subtitle, QLabel#field, QLabel#matSerial,
QLabel#catCount, QLabel#legendVal { font-size: 12px; color: @MUTED@; }
QLabel#kpiValue   { font-size: 26px; font-weight: bold; }
QLabel#kpiTitle   { font-size: 12px; }
QLabel#up         { font-size: 11px; color: @UP@; }
QLabel#down       { font-size: 11px; color: @DOWN@; }
QLabel#legend     { font-size: 12px; }
QLabel#matName    { font-size: 14px; font-weight: bold; }
QLabel#catBadge   { background-color: @ACCENT_BG@; color: @ACCENT@; border-radius: 6px;
                    padding: 2px 8px; font-size: 10px; }
QLabel#warn       { color: @DOWN@; font-size: 16px; font-weight: bold; }
QLabel#done       { color: @UP@; font-size: 15px; font-weight: bold; }
QLabel#preview    { background-color: @ROW@; border: 1px dashed @ACCENT@; border-radius: 10px;
                    color: @MUTED@; }

QFrame#card, QFrame#panel, QFrame#form {
    background-color: @SURFACE@; border: 1px solid @BORDER@; border-radius: 14px;
}
QFrame#card { background-color: @CARD@; border: 1px solid @CARDBORDER@; }
QFrame#summary { background-color: @SURFACE@; border: 1px solid @UP@; border-radius: 14px; }
QFrame#matCard { background-color: @CARD@; border: 1px solid @BORDER@; border-radius: 14px; }
QFrame#matCard:hover { border: 1px solid @ACCENT@; }
QFrame#imageBox { background-color: @ROW@; border-radius: 10px; }
QFrame#catRow { background: transparent; border: 1px solid transparent; border-radius: 8px; }
QFrame#catRow[selected="true"] { background-color: @ACCENT_BG@; border: 1px solid @ACCENT@; }

QLineEdit, QDateEdit, QComboBox {
    background-color: @INPUT@; border: 1px solid @BORDER@; border-radius: 8px;
    padding: 7px 10px; color: @TEXT@;
}
QLineEdit:focus, QDateEdit:focus, QComboBox:focus { border: 1px solid @ACCENT@; }
QLineEdit#search { border-radius: 10px; padding: 8px 12px; }
QComboBox QAbstractItemView {
    background-color: @DIALOG@; color: @TEXT@; selection-background-color: @ACCENT_BG@;
}

QPushButton#addBtn, QPushButton#primary {
    background-color: @ACCENT_BG@; color: @TEXT@; border: 1px solid @ACCENT@;
    border-radius: 8px; padding: 9px 14px; font-weight: bold;
}
QPushButton#addBtn:hover, QPushButton#primary:hover { background-color: @ACCENT_HOVER@; }
QPushButton#secondary {
    background: transparent; color: @MUTED@; border: 1px solid @CARDBORDER@;
    border-radius: 8px; padding: 8px 14px;
}
QPushButton#more { background: transparent; color: @MUTED@; border: none; font-size: 14px; font-weight: bold; }
QPushButton#more:hover { color: @TEXT@; }
QPushButton#action {
    background-color: @CARD@; color: @TEXT@; border: 1px solid @CARDBORDER@;
    border-radius: 10px; text-align: left; padding: 9px 14px; font-size: 12px;
}
QPushButton#action:hover { background-color: @HOVER@; }

QPushButton#NavDashBoard, QPushButton#NavMateriels, QPushButton#NavEtudiant,
QPushButton#NavEmprunts, QPushButton#NavRetours {
    background-color: @CARD@; color: @TEXT@; border: 1px solid @NAVBORDER@;
    border-radius: 8px; padding: 10px 14px; text-align: left;
}
QPushButton#NavDashBoard:hover, QPushButton#NavMateriels:hover, QPushButton#NavEtudiant:hover,
QPushButton#NavEmprunts:hover, QPushButton#NavRetours:hover { background-color: @HOVER@; }
QPushButton#NavDashBoard:checked, QPushButton#NavMateriels:checked, QPushButton#NavEtudiant:checked,
QPushButton#NavEmprunts:checked, QPushButton#NavRetours:checked {
    background-color: @ACCENT_BG@; border: 1px solid @ACCENT@; font-weight: bold;
}

QTableWidget {
    background-color: @SECTION@; border: 1px solid @CARDBORDER@; border-radius: 10px;
    gridline-color: @GRID@; color: @TEXT@; alternate-background-color: @ROW@;
    selection-background-color: @ACCENT_BG@; selection-color: @TEXT@;
}
QHeaderView::section {
    background-color: @HEADER@; color: @MUTED@; border: none; padding: 6px; font-weight: bold;
}

QListWidget#cats { background: transparent; border: none; outline: none; }
QListWidget#cats::item { background: transparent; border: none; }
QListWidget#cats::item:selected, QListWidget#cats::item:hover { background: transparent; }

QScrollArea { background: transparent; border: none; }
QWidget#gridHost { background: transparent; }
)CSS");

    const QList<QPair<QString, QString>> tokens = {
        { "@TEXT@",         d ? "#e5e7eb"                  : "#0f172a" },
        { "@MUTED@",        d ? "#94a3b8"                  : "#334155" },
        { "@ACCENT@",       d ? "#38bdf8"                  : "#0369a1" },
        { "@ACCENT_BG@",    d ? "rgba(56, 189, 248, 70)"   : "rgba(3, 105, 161, 40)" },
        { "@ACCENT_HOVER@", d ? "rgba(56, 189, 248, 120)"  : "rgba(3, 105, 161, 70)" },
        { "@SURFACE@",      d ? "rgba(10, 10, 14, 190)"    : "rgba(255, 255, 255, 60)" },
        { "@CARD@",         d ? "rgba(10, 10, 14, 200)"    : "rgba(255, 255, 255, 70)" },
        { "@CARDBORDER@",   d ? "rgba(255, 255, 255, 45)"  : "rgba(15, 23, 42, 90)" },
        { "@BORDER@",       d ? "rgba(56, 189, 248, 80)"   : "rgba(3, 105, 161, 130)" },
        { "@INPUT@",        d ? "rgba(255, 255, 255, 15)"  : "rgba(255, 255, 255, 90)" },
        { "@ROW@",          d ? "rgba(255, 255, 255, 12)"  : "rgba(15, 23, 42, 18)" },
        { "@HOVER@",        d ? "rgba(255, 255, 255, 30)"  : "rgba(15, 23, 42, 30)" },
        { "@GRID@",         d ? "rgba(255, 255, 255, 25)"  : "rgba(15, 23, 42, 45)" },
        { "@DIALOG@",       d ? "#0b0f17"                  : "rgba(255, 255, 255, 235)" },
        { "@NAVBORDER@",    d ? "#ffffff"                  : "#0f172a" },
        { "@SECTION@",      d ? "rgba(255, 255, 255, 20)"  : "rgba(255, 255, 255, 60)" },
        { "@HEADER@",       d ? "rgba(255, 255, 255, 30)"  : "rgba(15, 23, 42, 30)" },
        { "@UP@",           d ? "#4ade80"                  : "#15803d" },
        { "@DOWN@",         d ? "#f87171"                  : "#b91c1c" }
    };

    for (const auto &t : tokens)
        s.replace(t.first, t.second);

    return s;
}

// Délégué qui force l'alignement centré de chaque cellule
class CenterDelegate : public QStyledItemDelegate
{
public:
    explicit CenterDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);
        option->displayAlignment = Qt::AlignCenter;
    }
};

} // namespace

namespace Theme {

void setDark(bool dark)
{
    g_dark = dark;
    qApp->setStyleSheet(buildQss(dark));
}

bool isDark() { return g_dark; }

QColor text()       { return g_dark ? QColor("#e5e7eb") : QColor("#0f172a"); }
QColor muted()      { return g_dark ? QColor("#94a3b8") : QColor("#334155"); }
QColor accent()     { return g_dark ? QColor("#38bdf8") : QColor("#0369a1"); }
QColor background() { return g_dark ? QColor(0, 0, 0)   : QColor(255, 255, 255); }
QColor chartLine()  { return g_dark ? QColor(255, 255, 255) : QColor("#0f172a"); }
QColor chartGrid()  { return g_dark ? QColor(255, 255, 255, 35) : QColor(15, 23, 42, 45); }
QColor tooltipBg()  { return g_dark ? QColor(15, 15, 15, 230) : QColor(255, 255, 255, 245); }

void centerTable(QTableWidget *table)
{
    if (!table)
        return;

    table->setItemDelegate(new CenterDelegate(table));
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
}

} // namespace Theme