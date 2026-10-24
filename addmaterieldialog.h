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
    explicit AddMaterielDialog(const QList<Category> &categories, QWidget *parent = nullptr);

    QString designation() const;
    QString numeroSerie() const;
    int categoryId() const;
    QString photoPath() const;

private:
    void choosePhoto();
    void validateAndAccept();

    QLabel *m_preview = nullptr;
    QLineEdit *m_designation = nullptr;
    QLineEdit *m_serial = nullptr;
    QComboBox *m_cat = nullptr;
    QString m_photo;
};