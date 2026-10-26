#pragma once

#include <QWidget>
#include "datastore.h"

class QShowEvent;
class QLineEdit;
class QTableWidget;
class QLabel;

class StudentsPage : public QWidget
{
    Q_OBJECT

public:
    explicit StudentsPage(DataStore *store, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void reload();
    void onAdd();
    void onEdit(const Student &student);

    DataStore *m_store = nullptr;
    QLineEdit *m_search = nullptr;
    QLabel *m_count = nullptr;
    QTableWidget *m_table = nullptr;
};