#pragma once

#include <QColor>

class QTableWidget;

namespace Theme {

void setDark(bool dark);
bool isDark();

QColor text();
QColor muted();
QColor accent();
QColor background();
QColor chartLine();
QColor chartGrid();
QColor tooltipBg();

// Centre le contenu de toutes les cellules et des en-têtes d'un tableau
void centerTable(QTableWidget *table);

} // namespace Theme