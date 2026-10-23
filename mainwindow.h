#pragma once

#include <QMainWindow>
#include <QStringList>
#include <QList>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QStackedWidget;
class QGraphicsOpacityEffect;
class QPushButton;
class QResizeEvent;
class DataStore;
class EmpruntsPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void setDashboardVisible(bool show);

private:
    void layoutInside();
    void showPage(int index);
    QWidget *makeDashboardPage();
    QWidget *makeTablePage(const QString &title,
                           const QStringList &headers,
                           const QList<QStringList> &rows);

    Ui::MainWindow *ui;

    DataStore *m_store = nullptr;
    EmpruntsPage *m_emprunts = nullptr;

    QStackedWidget *m_pages = nullptr;
    QList<QPushButton *> m_navs;   // position = numéro de page (nullptr si pas de bouton)

    QList<QGraphicsOpacityEffect *> m_kpiFx;
};