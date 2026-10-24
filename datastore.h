#pragma once

#include <QString>
#include <QDate>
#include <QList>
#include <optional>

class QSqlDatabase;

struct Student {
    int id = 0;               // id_emprunteur
    QString matricule;
    QString nom;
    QString prenom;
};

struct Material {
    int id = 0;               // id_materiel
    QString designation;
    QString numeroSerie;
    QString etat;             // disponible, emprunte, endommage
};

struct Category {
    int id = 0;               // id_categorie (0 = « Tous »)
    QString libelle;
    int count = 0;
};

struct MaterialRow {
    int id = 0;
    QString designation;
    QString numeroSerie;
    QString etat;
    QString libelle;
    QString photo;
};

struct Loan {
    int materielId = 0;
    int emprunteurId = 0;
    QDate dateEmprunt;
    QDate dateRetourPrevue;
};

struct ActiveLoan {
    int id = 0;                // id_emprunt
    int materielId = 0;
    QString designation;
    QString numeroSerie;
    int emprunteurId = 0;
    QString matricule;
    QString nomComplet;        // « Prénom Nom »
    QDate dateEmprunt;
    QDate dateRetourPrevue;
};

class DataStore
{
public:
    DataStore();
    ~DataStore();

    std::optional<Student>  findStudent(const QString &matricule) const;
    std::optional<Material> findMaterial(const QString &key) const;

    bool isBorrowed(int materielId) const;        // ce matériel est-il non rendu ?
    bool hasLateLoan(int emprunteurId) const;     // a-t-il un retour dépassé ?
    bool hasActiveLoan(int emprunteurId) const;   // a-t-il un matériel non rendu ?
    bool addLoan(const Loan &loan);

    QList<ActiveLoan> listActiveLoans() const;    // emprunts non rendus
    bool returnLoan(int idEmprunt, bool endommage);   // true = matériel endommagé

    QList<Category>    listCategories() const;
    QList<MaterialRow> listMaterials(int categoryId, const QString &search) const;
    bool addMaterial(const QString &designation, const QString &numeroSerie,
                     int categoryId, const QString &photo);

private:
    QSqlDatabase db() const;
    bool open();
};