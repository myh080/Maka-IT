#include "datastore.h"

DataStore::DataStore()
{
    m_students = {
        { "ETU001", "Rakoto",        "Miora" },
        { "ETU002", "Rabe",          "Hery"  },
        { "ETU003", "Andriamifidy",  "Lova"  },
        { "ETU004", "Razafindrabe",  "Tiana" },
        { "ETU005", "Rakotomamonjy", "Sarah" }
    };

    m_materials = {
        { "Arduino Uno R3",           "A0001-00123" },
        { "Arduino Mega 2560",        "A0001-00456" },
        { "Raspberry Pi 4",           "RPI4-00876"  },
        { "ESP32 DevKit",             "ESP32-11223" },
        { "Arduino Nano",             "A0002-00987" },
        { "Breadboard",               "BB-00145"    },
        { "Capteur Ultrason HC-SR04", "HC-00321"    },
        { "Servo Moteur SG90",        "SG90-00765"  },
        { "Kit Arduino Complet",      "KIT-00112"   },
        { "Écran OLED 0.96\"",        "OLED-00567"  },
        { "Module Relais 5V",         "REL-00934"   },
        { "Caméra ESP32-CAM",         "CAM-00421"   }
    };

    // Exemples de prêts, dates calculées à partir d'aujourd'hui
    const QDate today = QDate::currentDate();
    m_loans = {
        { "Arduino Mega 2560", "ETU002", today.addDays(-3),  today.addDays(4),  false },  // en cours
        { "Breadboard",        "ETU004", today.addDays(-10), today.addDays(-2), false }   // en retard
    };
}

const Student *DataStore::findStudent(const QString &matricule) const
{
    for (const Student &s : m_students)
        if (s.matricule.compare(matricule, Qt::CaseInsensitive) == 0)
            return &s;
    return nullptr;
}

const Material *DataStore::findMaterial(const QString &key) const
{
    for (const Material &m : m_materials)
        if (m.name.compare(key, Qt::CaseInsensitive) == 0
            || m.serial.compare(key, Qt::CaseInsensitive) == 0)
            return &m;
    return nullptr;
}

bool DataStore::isBorrowed(const QString &materialName) const
{
    for (const Loan &l : m_loans)
        if (!l.returned && l.materialName == materialName)
            return true;
    return false;
}

bool DataStore::hasLateLoan(const QString &matricule) const
{
    const QDate today = QDate::currentDate();
    for (const Loan &l : m_loans)
        if (!l.returned && l.matricule == matricule && l.end < today)
            return true;
    return false;
}

void DataStore::addLoan(const Loan &loan)
{
    m_loans << loan;
}