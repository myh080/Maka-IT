#pragma once

#include <QMainWindow>
#include <QList>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QStackedWidget;
class QPushButton;
class QResizeEvent;
class QEvent;
class DataStore;
class DashboardPage;
class EmpruntsPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *ev) override;

private slots:
    void setDashboardVisible(bool show);

private:
    void layoutInside();
    void showPage(int index);

    Ui::MainWindow *ui;

    DataStore *m_store = nullptr;
    DashboardPage *m_dashboard = nullptr;
    EmpruntsPage *m_emprunts = nullptr;

    QStackedWidget *m_pages = nullptr;
    QList<QPushButton *> m_navs;
};