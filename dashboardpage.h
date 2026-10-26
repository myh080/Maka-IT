#ifndef DASHBOARDPAGE_H
#define DASHBOARDPAGE_H

#pragma once

#include <QWidget>

class QShowEvent;
class QGraphicsOpacityEffect;
class QLabel;
class QVBoxLayout;
class QTableWidget;
class DataStore;

class DashboardPage : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardPage(DataStore *store, QWidget *parent = nullptr);

    void playEntrance();   // fondu des quatre cartes, une par une

signals:
    void pageRequested(int index);   // boutons « Actions rapides »

protected:
    void showEvent(QShowEvent *event) override;

private:
    void reload();

    DataStore *m_store = nullptr;

    QLabel *m_kpiValue[4] = {};
    QLabel *m_kpiSub[4] = {};
    QGraphicsOpacityEffect *m_kpiFx[4] = {};

    QWidget *m_line = nullptr;       // LineChart (défini dans le .cpp)
    QWidget *m_donut = nullptr;      // Donut (défini dans le .cpp)
    QVBoxLayout *m_legend = nullptr;
    QTableWidget *m_table = nullptr;
};

#endif // DASHBOARDPAGE_H
