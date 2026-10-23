#pragma once

#include <QWidget>
#include <QList>
#include <QStringList>

class QEvent;
class QFrame;
class QGridLayout;
class QLineEdit;
class QListWidget;

class MaterielsPage : public QWidget
{
    Q_OBJECT

public:
    explicit MaterielsPage(QWidget *parent = nullptr);

signals:
    // Émis quand l'utilisateur confirme vouloir emprunter un matériel disponible
    void emprunterRequested(const QString &name);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    struct Item {
        QString name;
        QString serial;
        QString category;
        int status;      // 0 = disponible, 1 = emprunté, 2 = endommagé
        QString photo;   // chemin de l'image, vide si aucune
    };

    void addItem(const Item &it);
    QFrame *makeCard(const Item &it, int index);
    void rebuildCategories();
    void updateCatSelection(int row);
    void refresh();
    void onAddClicked();
    void onCardDoubleClicked(int index);

    QList<Item> m_items;
    QList<QFrame *> m_cards;
    QList<QFrame *> m_catRows;
    QStringList m_catNames;
    QString m_currentCat = "Tous";

    QLineEdit *m_search = nullptr;
    QListWidget *m_cats = nullptr;
    QWidget *m_gridHost = nullptr;
    QGridLayout *m_grid = nullptr;
};