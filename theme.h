#ifndef THEME_H
#define THEME_H

#pragma once

#include <QColor>

namespace Theme {

void setDark(bool dark);     // applique le thème à toute l'application
bool isDark();

QColor text();               // texte principal
QColor muted();              // texte secondaire
QColor accent();             // couleur d'accent
QColor background();         // fond uni (contour des points du graphique)
QColor chartLine();          // courbe et points
QColor chartGrid();          // grille et bordure des graphiques
QColor tooltipBg();          // bulle d'information du graphique

}

#endif // THEME_H
