#ifndef EMPRUNTSPAGE_H
#define EMPRUNTSPAGE_H

#pragma once

#include <QWidget>
#include <QDate>

class QLineEdit;
class QDateEdit;
class QFrame;
class QLabel;
class DataStore;

class EmpruntsPage : public QWidget
{
    Q_OBJECT

public:
    explicit EmpruntsPage(DataStore *store, QWidget *parent = nullptr);

    // Remplit le champ matériel (appelé depuis la page Matériels)
    void prefillMaterial(const QString &name);

signals:
    void navigateToStudents();            // emprunteur non inscrit
    void loanCreated(const QString &material);

private:
    void onSubmit();
    void showSummary(const QString &material, const QString &who,
                     const QString &matricule, const QDate &start, const QDate &end);

    DataStore *m_store = nullptr;
    QLineEdit *m_material = nullptr;
    QLineEdit *m_matricule = nullptr;
    QDateEdit *m_start = nullptr;
    QDateEdit *m_end = nullptr;
    QFrame *m_summary = nullptr;
    QLabel *m_summaryText = nullptr;
};

#endif // EMPRUNTSPAGE_H
