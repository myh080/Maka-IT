#pragma once

#include <QDialog>
#include <QList>
#include "datastore.h"

class QLabel;
class QLineEdit;
class QComboBox;

class AddMaterielDialog : public QDialog
{
    Q_OBJECT

public:
    // Si « edit » est fourni, le formulaire est pré-rempli pour une modification
    explicit AddMaterielDialog(const QList<Category> &categories, QWidget *parent = nullptr,
                               const MaterialRow *edit = nullptr);

    QString designation() const;
    int categoryId() const;
    QString photoPath() const;

private:
    void choosePhoto();
    void validateAndAccept();

    QLabel *m_preview = nullptr;
    QLineEdit *m_designation = nullptr;
    QComboBox *m_cat = nullptr;
    QString m_photo;
};