#ifndef DATASTORE_H
#define DATASTORE_H

#pragma once

#include <QString>
#include <QDate>
#include <QList>

struct Student {
    QString matricule;
    QString nom;
    QString prenom;
};

struct Material {
    QString name;
    QString serial;
};

struct Loan {
    QString materialName;
    QString matricule;
    QDate start;
    QDate end;
    bool returned = false;
};

// Données en mémoire, en attendant SQLite
class DataStore
{
public:
    DataStore();

    const Student *findStudent(const QString &matricule) const;
    const Material *findMaterial(const QString &key) const;   // nom ou numéro de série

    bool isBorrowed(const QString &materialName) const;       // un seul emprunt actif à la fois
    bool hasLateLoan(const QString &matricule) const;         // prêt en retard ?

    void addLoan(const Loan &loan);

private:
    QList<Student> m_students;
    QList<Material> m_materials;
    QList<Loan> m_loans;
};
#endif // DATASTORE_H
