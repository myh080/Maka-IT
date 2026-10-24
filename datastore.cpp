#include "datastore.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

namespace {

const QString DB_PATH = "H:/Qt project/Maka_IT/maka_it.db";
const QString CONN    = "maka_it";
const QString ISO     = "yyyy-MM-dd";

bool run(QSqlQuery &q)
{
    if (!q.exec()) {
        qWarning() << "Erreur SQL :" << q.lastError().text() << "\n" << q.lastQuery();
        return false;
    }
    return true;
}

} // namespace

DataStore::DataStore()
{
    if (!open())
        return;

    if (!db().tables().contains("emprunteur")) {
        qWarning() << "Base vide : créez les tables dans DB Browser. Fichier :" << DB_PATH;
        return;
    }

    qDebug() << "Base ouverte :" << DB_PATH;
}

DataStore::~DataStore()
{
    {
        QSqlDatabase d = db();
        if (d.isOpen())
            d.close();
    }
    QSqlDatabase::removeDatabase(CONN);
}

QSqlDatabase DataStore::db() const
{
    return QSqlDatabase::database(CONN);
}

bool DataStore::open()
{
    QSqlDatabase d = QSqlDatabase::addDatabase("QSQLITE", CONN);
    d.setDatabaseName(DB_PATH);

    if (!d.open()) {
        qWarning() << "ECHEC ouverture :" << d.lastError().text() << "| chemin :" << DB_PATH;
        return false;
    }

    QSqlQuery pragma(d);
    pragma.exec("PRAGMA foreign_keys = ON");
    return true;
}

std::optional<Student> DataStore::findStudent(const QString &matricule) const
{
    QSqlQuery q(db());
    q.prepare("SELECT id_emprunteur, matricule, nom, prenom FROM emprunteur "
              "WHERE matricule = ? COLLATE NOCASE");
    q.addBindValue(matricule);

    if (!run(q) || !q.next())
        return std::nullopt;

    return Student{ q.value(0).toInt(), q.value(1).toString(),
                   q.value(2).toString(), q.value(3).toString() };
}

std::optional<Material> DataStore::findMaterial(const QString &key) const
{
    QSqlQuery q(db());
    q.prepare("SELECT id_materiel, designation, numero_serie, etat FROM materiel "
              "WHERE designation = ? COLLATE NOCASE OR numero_serie = ? COLLATE NOCASE LIMIT 1");
    q.addBindValue(key);
    q.addBindValue(key);

    if (!run(q) || !q.next())
        return std::nullopt;

    return Material{ q.value(0).toInt(), q.value(1).toString(),
                    q.value(2).toString(), q.value(3).toString() };
}

bool DataStore::isBorrowed(int materielId) const
{
    QSqlQuery q(db());
    q.prepare("SELECT COUNT(*) FROM emprunt "
              "WHERE id_materiel = ? AND date_retour_effective IS NULL");
    q.addBindValue(materielId);

    return run(q) && q.next() && q.value(0).toInt() > 0;
}

bool DataStore::hasLateLoan(int emprunteurId) const
{
    QSqlQuery q(db());
    q.prepare("SELECT COUNT(*) FROM emprunt "
              "WHERE id_emprunteur = ? AND date_retour_effective IS NULL "
              "AND date_retour_prevue < ?");
    q.addBindValue(emprunteurId);
    q.addBindValue(QDate::currentDate().toString(ISO));

    return run(q) && q.next() && q.value(0).toInt() > 0;
}

bool DataStore::addLoan(const Loan &loan)
{
    QSqlDatabase d = db();
    d.transaction();

    QSqlQuery ins(d);
    ins.prepare("INSERT INTO emprunt (id_emprunteur, id_materiel, date_emprunt, date_retour_prevue) "
                "VALUES (?, ?, ?, ?)");
    ins.addBindValue(loan.emprunteurId);
    ins.addBindValue(loan.materielId);
    ins.addBindValue(loan.dateEmprunt.toString(ISO));
    ins.addBindValue(loan.dateRetourPrevue.toString(ISO));

    QSqlQuery upd(d);
    upd.prepare("UPDATE materiel SET etat = 'emprunte' WHERE id_materiel = ?");
    upd.addBindValue(loan.materielId);

    if (!run(ins) || !run(upd)) {
        d.rollback();
        return false;
    }

    return d.commit();
}

QList<Category> DataStore::listCategories() const
{
    QList<Category> out;

    QSqlQuery q(db());
    q.prepare("SELECT c.id_categorie, c.libelle, COUNT(m.id_materiel) "
              "FROM categorie c LEFT JOIN materiel m ON m.id_categorie = c.id_categorie "
              "GROUP BY c.id_categorie, c.libelle ORDER BY c.id_categorie");

    if (!run(q))
        return out;

    while (q.next())
        out << Category{ q.value(0).toInt(), q.value(1).toString(), q.value(2).toInt() };

    return out;
}

QList<MaterialRow> DataStore::listMaterials(int categoryId, const QString &search) const
{
    QList<MaterialRow> out;
    const QString like = "%" + search.trimmed() + "%";

    QSqlQuery q(db());
    q.prepare("SELECT m.id_materiel, m.designation, m.numero_serie, m.etat, c.libelle, m.photo "
              "FROM materiel m LEFT JOIN categorie c ON c.id_categorie = m.id_categorie "
              "WHERE (? = 0 OR m.id_categorie = ?) "
              "AND (m.designation LIKE ? OR m.numero_serie LIKE ?) "
              "ORDER BY m.designation");
    q.addBindValue(categoryId);
    q.addBindValue(categoryId);
    q.addBindValue(like);
    q.addBindValue(like);

    if (!run(q))
        return out;

    while (q.next())
        out << MaterialRow{ q.value(0).toInt(), q.value(1).toString(), q.value(2).toString(),
                           q.value(3).toString(), q.value(4).toString(), q.value(5).toString() };

    return out;
}

bool DataStore::addMaterial(const QString &designation, const QString &numeroSerie,
                            int categoryId, const QString &photo)
{
    QSqlQuery q(db());
    q.prepare("INSERT INTO materiel (id_categorie, designation, numero_serie, etat, photo) "
              "VALUES (?, ?, ?, 'disponible', ?)");
    q.addBindValue(categoryId);
    q.addBindValue(designation);
    q.addBindValue(numeroSerie);
    q.addBindValue(photo.isEmpty() ? QVariant() : QVariant(photo));

    return run(q);
}

bool DataStore::hasActiveLoan(int emprunteurId) const
{
    QSqlQuery q(db());
    q.prepare("SELECT COUNT(*) FROM emprunt "
              "WHERE id_emprunteur = ? AND date_retour_effective IS NULL");
    q.addBindValue(emprunteurId);

    return run(q) && q.next() && q.value(0).toInt() > 0;
}

QList<ActiveLoan> DataStore::listActiveLoans() const
{
    QList<ActiveLoan> out;

    QSqlQuery q(db());
    q.prepare("SELECT e.id_emprunt, m.id_materiel, m.designation, m.numero_serie, "
              "       em.id_emprunteur, em.matricule, em.prenom, em.nom, "
              "       e.date_emprunt, e.date_retour_prevue "
              "FROM emprunt e "
              "JOIN materiel m    ON m.id_materiel    = e.id_materiel "
              "JOIN emprunteur em ON em.id_emprunteur = e.id_emprunteur "
              "WHERE e.date_retour_effective IS NULL "
              "ORDER BY e.date_retour_prevue");

    if (!run(q))
        return out;

    while (q.next()) {
        ActiveLoan l;
        l.id               = q.value(0).toInt();
        l.materielId       = q.value(1).toInt();
        l.designation      = q.value(2).toString();
        l.numeroSerie      = q.value(3).toString();
        l.emprunteurId     = q.value(4).toInt();
        l.matricule        = q.value(5).toString();
        l.nomComplet       = q.value(6).toString() + " " + q.value(7).toString();
        l.dateEmprunt      = QDate::fromString(q.value(8).toString(), Qt::ISODate);
        l.dateRetourPrevue = QDate::fromString(q.value(9).toString(), Qt::ISODate);
        out << l;
    }

    return out;
}

bool DataStore::returnLoan(int idEmprunt, bool endommage)
{
    QSqlDatabase d = db();
    d.transaction();

    QSqlQuery upd(d);
    upd.prepare("UPDATE emprunt SET date_retour_effective = ? "
                "WHERE id_emprunt = ? AND date_retour_effective IS NULL");
    upd.addBindValue(QDate::currentDate().toString(ISO));
    upd.addBindValue(idEmprunt);

    QSqlQuery mat(d);
    mat.prepare("UPDATE materiel SET etat = ? "
                "WHERE etat = 'emprunte' "
                "AND id_materiel = (SELECT id_materiel FROM emprunt WHERE id_emprunt = ?)");
    mat.addBindValue(endommage ? "endommage" : "disponible");
    mat.addBindValue(idEmprunt);

    if (!run(upd) || !run(mat) || upd.numRowsAffected() != 1) {
        d.rollback();
        return false;
    }

    return d.commit();
}