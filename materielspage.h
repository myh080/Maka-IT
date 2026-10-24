#pragma once

#include <QWidget>
#include <QList>
#include "datastore.h"

class QEvent;
class QShowEvent;
class QFrame;
class QGridLayout;
class QLineEdit;
class QListWidget;

class MaterielsPage : public QWidget
{
    Q_OBJECT

public:
    explicit MaterielsPage(DataStore *store, QWidget *parent = nullptr);

signals:
    void emprunterRequested(const QString &designation);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;
    void showEvent(QShowEvent *event) override;

private:
    void loadCategories();
    void selectCategory(int row);
    void loadMaterials();
    QFrame *makeCard(const MaterialRow &row, int index);
    void onCardDoubleClicked(int index);
    void onAddClicked();

    DataStore *m_store = nullptr;
    QList<Category> m_categories;      // « Tous » en premier
    QList<QFrame *> m_catRows;
    QList<MaterialRow> m_rows;
    QList<QFrame *> m_cards;
    int m_currentCat = 0;

    QLineEdit *m_search = nullptr;
    QListWidget *m_cats = nullptr;
    QWidget *m_gridHost = nullptr;
    QGridLayout *m_grid = nullptr;
};