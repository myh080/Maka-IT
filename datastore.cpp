#include "datastore.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QMap>
#include <QDebug>
#include <algorithm>

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

QDate dateOf(const QVariant &v)
{
    return QDate::fromString(v.toString(), Qt::ISODate);
}

QString monthLabel(int month)
{
    static const QStringList names = { "Jan", "Fév", "Mars", "Avr", "Mai", "Juin",
                                      "Juil", "Août", "Sep", "Oct", "Nov", "Déc" };
    return names.at(month - 1);
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

// ----- Emprunteurs -----

std::optional<Student> DataStore::findStudent(const QString &matricule) const
{
    QSqlQuery q(db());
    q.prepare("SELECT id_emprunteur, matricule, nom, prenom, telephone FROM emprunteur "
              "WHERE matricule = ? COLLATE NOCASE");
    q.addBindValue(matricule);

    if (!run(q) || !q.next())
        return std::nullopt;

    return Student{ q.value(0).toInt(), q.value(1).toString(), q.value(2).toString(),
                   q.value(3).toString(), q.value(4).toString() };
}

QList<Student> DataStore::listStudents(const QString &search) const
{
    QList<Student> out;
    const QString like = "%" + search.trimmed() + "%";

    QSqlQuery q(db());
    q.prepare("SELECT id_emprunteur, matricule, nom, prenom, telephone FROM emprunteur "
              "WHERE nom LIKE ? OR prenom LIKE ? OR matricule LIKE ? OR telephone LIKE ? "
              "ORDER BY nom, prenom");
    for (int i = 0; i < 4; ++i)
        q.addBindValue(like);

    if (!run(q))
        return out;

    while (q.next())
        out << Student{ q.value(0).toInt(), q.value(1).toString(), q.value(2).toString(),
                       q.value(3).toString(), q.value(4).toString() };

    return out;
}

bool DataStore::addStudent(const QString &nom, const QString &prenom,
                           const QString &matricule, const QString &telephone)
{
    QSqlQuery q(db());
    q.prepare("INSERT INTO emprunteur (nom, prenom, matricule, telephone) VALUES (?, ?, ?, ?)");
    q.addBindValue(nom);
    q.addBindValue(prenom);
    q.addBindValue(matricule);
    q.addBindValue(telephone.isEmpty() ? QVariant() : QVariant(telephone));

    return run(q);
}

// ----- Matériels -----

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

// ----- Emprunts -----

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

bool DataStore::hasActiveLoan(int emprunteurId) const
{
    QSqlQuery q(db());
    q.prepare("SELECT COUNT(*) FROM emprunt "
              "WHERE id_emprunteur = ? AND date_retour_effective IS NULL");
    q.addBindValue(emprunteurId);

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

QList<ActiveLoan> DataStore::listActiveLoans() const
{
    QList<ActiveLoan> out;

    QSqlQuery q(db());
    q.prepare("SELECT e.id_emprunt, m.id_materiel, m.designation, m.numero_serie, "
              "       em.id_emprunteur, em.matricule, em.nom, em.prenom, "
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
        l.nom              = q.value(6).toString();
        l.prenom           = q.value(7).toString();
        l.dateEmprunt      = dateOf(q.value(8));
        l.dateRetourPrevue = dateOf(q.value(9));
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

// ----- Tableau de bord -----

DashboardStats DataStore::dashboardStats() const
{
    DashboardStats s;
    const QDate today = QDate::currentDate();
    const QString todayIso = today.toString(ISO);

    // 1. Compteurs
    QSqlQuery c(db());
    c.prepare("SELECT "
              " (SELECT COUNT(*) FROM materiel WHERE etat = 'disponible'), "
              " (SELECT COUNT(*) FROM materiel), "
              " (SELECT COUNT(*) FROM emprunteur), "
              " (SELECT COUNT(*) FROM emprunt WHERE date_retour_effective IS NULL), "
              " (SELECT COUNT(*) FROM emprunt WHERE date_retour_effective IS NULL "
              "    AND date_retour_prevue < ?), "
              " (SELECT COUNT(*) FROM emprunt WHERE date_retour_effective = ?), "
              " (SELECT COUNT(*) FROM emprunt WHERE date_retour_effective IS NULL "
              "    AND date_retour_prevue = ?)");
    c.addBindValue(todayIso);
    c.addBindValue(todayIso);
    c.addBindValue(todayIso);

    if (run(c) && c.next()) {
        s.materielsDispo          = c.value(0).toInt();
        s.materielsTotal          = c.value(1).toInt();
        s.etudiants               = c.value(2).toInt();
        s.empruntsEnCours         = c.value(3).toInt();
        s.empruntsRetard          = c.value(4).toInt();
        s.retoursAujourdhui       = c.value(5).toInt();
        s.retoursPrevusAujourdhui = c.value(6).toInt();
    }

    // 2. Emprunts par mois, sur les 10 derniers mois
    QMap<QString, int> parMois;
    QSqlQuery m(db());
    m.prepare("SELECT substr(date_emprunt, 1, 7), COUNT(*) FROM emprunt "
              "GROUP BY substr(date_emprunt, 1, 7)");
    if (run(m)) {
        while (m.next())
            parMois.insert(m.value(0).toString(), m.value(1).toInt());
    }

    for (int i = 9; i >= 0; --i) {
        const QDate d = today.addMonths(-i);
        s.mensuel << parMois.value(d.toString("yyyy-MM"), 0);
        s.moisLabels << monthLabel(d.month());
    }

    // 3. Répartition par catégorie (catégories non vides)
    for (const Category &cat : listCategories()) {
        if (cat.count > 0)
            s.categories << cat;
    }
    std::sort(s.categories.begin(), s.categories.end(),
              [](const Category &a, const Category &b) { return a.count > b.count; });

    // 4. Derniers emprunts
    QSqlQuery r(db());
    r.prepare("SELECT e.date_emprunt, e.date_retour_prevue, e.date_retour_effective, "
              "       m.designation, em.prenom, em.nom "
              "FROM emprunt e "
              "JOIN materiel m    ON m.id_materiel    = e.id_materiel "
              "JOIN emprunteur em ON em.id_emprunteur = e.id_emprunteur "
              "ORDER BY e.id_emprunt DESC LIMIT 5");
    if (run(r)) {
        while (r.next()) {
            RecentLoan l;
            l.dateEmprunt         = dateOf(r.value(0));
            l.dateRetourPrevue    = dateOf(r.value(1));
            l.dateRetourEffective = dateOf(r.value(2));
            l.designation         = r.value(3).toString();
            l.nomComplet          = r.value(4).toString() + " " + r.value(5).toString();
            s.recent << l;
        }
    }

    return s;
}

bool DataStore::updateStudent(int id, const QString &nom, const QString &prenom,
                              const QString &matricule, const QString &telephone)
{
    QSqlQuery q(db());
    q.prepare("UPDATE emprunteur SET nom = ?, prenom = ?, matricule = ?, telephone = ? "
              "WHERE id_emprunteur = ?");
    q.addBindValue(nom);
    q.addBindValue(prenom);
    q.addBindValue(matricule);
    q.addBindValue(telephone.isEmpty() ? QVariant() : QVariant(telephone));
    q.addBindValue(id);

    return run(q) && q.numRowsAffected() == 1;
}

bool DataStore::repairMaterial(int materielId)
{
    QSqlQuery q(db());
    q.prepare("UPDATE materiel SET etat = 'disponible' "
              "WHERE id_materiel = ? AND etat = 'endommage'");
    q.addBindValue(materielId);

    return run(q) && q.numRowsAffected() == 1;
}

bool DataStore::setMaterialState(int materielId, const QString &etat)
{
    // Un matériel emprunté ne change jamais de statut ici : il doit être rendu
    QSqlQuery q(db());
    q.prepare("UPDATE materiel SET etat = ? WHERE id_materiel = ? AND etat <> 'emprunte'");
    q.addBindValue(etat);
    q.addBindValue(materielId);

    return run(q) && q.numRowsAffected() == 1;
}

int DataStore::categoryIdOfMaterial(int materielId) const
{
    QSqlQuery q(db());
    q.prepare("SELECT id_categorie FROM materiel WHERE id_materiel = ?");
    q.addBindValue(materielId);

    return (run(q) && q.next()) ? q.value(0).toInt() : 0;
}

std::optional<Material> DataStore::findMaterialById(int materielId) const
{
    QSqlQuery q(db());
    q.prepare("SELECT id_materiel, designation, numero_serie, etat FROM materiel "
              "WHERE id_materiel = ?");
    q.addBindValue(materielId);

    if (!run(q) || !q.next())
        return std::nullopt;

    return Material{ q.value(0).toInt(), q.value(1).toString(),
                    q.value(2).toString(), q.value(3).toString() };
}

QList<Material> DataStore::listAvailableMaterials(int categoryId) const
{
    QList<Material> out;

    QSqlQuery q(db());
    q.prepare("SELECT id_materiel, designation, numero_serie, etat FROM materiel "
              "WHERE id_categorie = ? AND etat = 'disponible' "
              "AND id_materiel NOT IN (SELECT id_materiel FROM emprunt "
              "                        WHERE date_retour_effective IS NULL) "
              "ORDER BY designation");
    q.addBindValue(categoryId);

    if (!run(q))
        return out;

    while (q.next())
        out << Material{ q.value(0).toInt(), q.value(1).toString(),
                        q.value(2).toString(), q.value(3).toString() };

    return out;
}