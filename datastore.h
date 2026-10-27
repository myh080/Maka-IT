#pragma once

#include <QString>
#include <QStringList>
#include <QDate>
#include <QList>
#include <optional>

class QSqlDatabase;

struct Student {
    int id = 0;               // id_emprunteur
    QString matricule;
    QString nom;
    QString prenom;
    QString telephone;
};

struct Material {
    int id = 0;               // id_materiel
    QString designation;
    QString numeroSerie;
    QString etat;             // disponible, emprunte, endommage
};

struct Category {
    int id = 0;               // id_categorie
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
    int id = 0;               // id_emprunt
    int materielId = 0;
    QString designation;
    QString numeroSerie;
    int emprunteurId = 0;
    QString matricule;
    QString nom;
    QString prenom;
    QDate dateEmprunt;
    QDate dateRetourPrevue;
};

struct RecentLoan {
    QDate dateEmprunt;
    QDate dateRetourPrevue;
    QDate dateRetourEffective;
    QString designation;
    QString nomComplet;
};

struct DashboardStats {
    int materielsDispo = 0;
    int materielsTotal = 0;
    int etudiants = 0;
    int empruntsEnCours = 0;
    int empruntsRetard = 0;
    int retoursAujourdhui = 0;
    int retoursPrevusAujourdhui = 0;
    QList<int> mensuel;
    QStringList moisLabels;
    QList<Category> categories;
    QList<RecentLoan> recent;
};

class DataStore
{
public:
    DataStore();
    ~DataStore();

    std::optional<Student>  findStudent(const QString &matricule) const;
    std::optional<Student> findStudentByName(const QString &nom, const QString &prenom) const;
    std::optional<Material> findMaterial(const QString &key) const;
    std::optional<Material> findMaterialById(int materielId) const;
    int categoryIdOfMaterial(int materielId) const;
    QList<Material> listAvailableMaterials(int categoryId) const;

    bool isBorrowed(int materielId) const;
    bool hasLateLoan(int emprunteurId) const;
    bool hasActiveLoan(int emprunteurId) const;
    bool addLoan(const Loan &loan);

    QList<ActiveLoan> listActiveLoans() const;
    bool returnLoan(int idEmprunt, bool endommage);

    QList<Category>    listCategories() const;
    QList<MaterialRow> listMaterials(int categoryId, const QString &search) const;
    bool addMaterial(const QString &designation, const QString &numeroSerie,
    int categoryId, const QString &photo);

    QString nextSerialFor(int categoryId) const;
    bool updateMaterial(int materielId, const QString &designation, int categoryId);

    bool repairMaterial(int materielId);
    bool setMaterialState(int materielId, const QString &etat);

    QList<Student> listStudents(const QString &search) const;
    bool addStudent(const QString &nom, const QString &prenom,
                    const QString &matricule, const QString &telephone);
    bool updateStudent(int id, const QString &nom, const QString &prenom,
                       const QString &matricule, const QString &telephone);

    DashboardStats dashboardStats() const;

private:
    QSqlDatabase db() const;
    bool open();
};