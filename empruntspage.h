#pragma once

#include <QWidget>
#include <QDate>

class QShowEvent;
class QLineEdit;
class QDateEdit;
class QFrame;
class QLabel;
class QComboBox;
class DataStore;

class EmpruntsPage : public QWidget
{
    Q_OBJECT

public:
    explicit EmpruntsPage(DataStore *store, QWidget *parent = nullptr);

    // Appelé depuis la page Matériels : sélectionne la catégorie et le matériel
    void prefillMaterial(int materielId);

signals:
    void navigateToStudents();
    void loanCreated(const QString &designation);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void fillCategories(int selectId);
    void fillMaterials(int categoryId, int selectId);
    int currentCategoryId() const;
    int currentMaterialId() const;
    void updateBorrower();
    void resetForm();
    void onSubmit();
    void showSummary(const QString &material, const QString &who,
                     const QString &matricule, const QDate &start, const QDate &end);

    DataStore *m_store = nullptr;
    QComboBox *m_category = nullptr;
    QComboBox *m_material = nullptr;
    QLineEdit *m_matricule = nullptr;
    QLineEdit *m_nom = nullptr;
    QLineEdit *m_prenom = nullptr;
    QLabel *m_borrowerStatus = nullptr;
    QDateEdit *m_start = nullptr;
    QDateEdit *m_end = nullptr;
    QFrame *m_summary = nullptr;
    QLabel *m_summaryText = nullptr;
};