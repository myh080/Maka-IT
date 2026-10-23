#ifndef ADDMATERIELDIALOG_H
#define ADDMATERIELDIALOG_H

#pragma once

#include <QDialog>
#include <QStringList>

class QLabel;
class QLineEdit;
class QComboBox;

class AddMaterielDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddMaterielDialog(const QStringList &categories, QWidget *parent = nullptr);

    QString name() const;
    QString category() const;
    QString photoPath() const;

private:
    void choosePhoto();

    QLabel *m_preview = nullptr;
    QLineEdit *m_name = nullptr;
    QComboBox *m_cat = nullptr;
    QString m_photo;
};

#endif // ADDMATERIELDIALOG_H
