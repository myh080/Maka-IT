#pragma once

#include <QWidget>
#include <QList>
#include "datastore.h"

class QShowEvent;
class QLineEdit;
class QComboBox;
class QTableWidget;
class QLabel;
class QPushButton;

class RetoursPage : public QWidget
{
    Q_OBJECT

public:
    explicit RetoursPage(DataStore *store, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void reload();                 // relit la base
    void applyFilters();           // recherche, statut, pagination, puis remplit le tableau
    void returnLoan(int idEmprunt, const QString &designation, const QString &who);

    DataStore *m_store = nullptr;
    QList<ActiveLoan> m_all;       // tous les emprunts non rendus
    QList<ActiveLoan> m_filtered;  // après recherche et filtre de statut
    int m_page = 0;                // page courante (0 = première)

    QLineEdit *m_search = nullptr;
    QComboBox *m_status = nullptr;
    QComboBox *m_perPage = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_pageLabel = nullptr;
    QPushButton *m_prev = nullptr;
    QPushButton *m_next = nullptr;
};