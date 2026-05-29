#pragma once

#include <QApplication>
#include <QIcon>
#include <QStyle>

namespace AppStyle {

enum class Theme { Light, Dark };

Theme savedTheme();
void saveTheme(Theme theme);
void applyTheme(QApplication &app, Theme theme);
Theme toggleTheme(QApplication &app);
QString themeLabel(Theme theme);
QIcon standardIcon(QStyle::StandardPixmap pixmap);

} // namespace AppStyle
