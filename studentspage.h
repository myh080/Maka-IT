#pragma once

#include <QWidget>
#include <QList>
#include "datastore.h"

class QShowEvent;
class QLineEdit;
class QTableWidget;
class QLabel;
class QComboBox;
class QPushButton;

class StudentsPage : public QWidget
{
    Q_OBJECT

public:
    explicit StudentsPage(DataStore *store, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void reload();          // relit la base
    void applyPage();       // découpe en pages et remplit le tableau
    void onAdd();
    void onEdit(const Student &student);

    DataStore *m_store = nullptr;
    QList<Student> m_all;   // tous les étudiants correspondant à la recherche
    int m_page = 0;

    QLineEdit *m_search = nullptr;
    QLabel *m_count = nullptr;
    QTableWidget *m_table = nullptr;
    QComboBox *m_perPage = nullptr;
    QPushButton *m_prev = nullptr;
    QPushButton *m_next = nullptr;
    QLabel *m_pageLabel = nullptr;
};