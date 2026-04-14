#include "densityviewer.h"
#include "densityviewerwindow.h"
#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Fusion"));

    // Dark theme palette
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(45, 45, 48));
    darkPalette.setColor(QPalette::WindowText, QColor(212, 212, 212));
    darkPalette.setColor(QPalette::Base, QColor(30, 30, 30));
    darkPalette.setColor(QPalette::AlternateBase, QColor(45, 45, 48));
    darkPalette.setColor(QPalette::Text, QColor(212, 212, 212));
    darkPalette.setColor(QPalette::Button, QColor(60, 60, 64));
    darkPalette.setColor(QPalette::ButtonText, QColor(212, 212, 212));
    darkPalette.setColor(QPalette::Highlight, QColor(70, 130, 200));
    darkPalette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(100, 100, 100));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(100, 100, 100));
    a.setPalette(darkPalette);

    a.setStyleSheet(
        "QMainWindow { background: #2d2d30; }"
        "QPushButton {"
        "  background: #3c3c40; border: 1px solid #555; border-radius: 4px;"
        "  padding: 5px 12px; color: #d4d4d4; min-height: 20px;"
        "}"
        "QPushButton:hover { background: #505054; border-color: #4682c8; }"
        "QPushButton:pressed { background: #4682c8; border-color: #4682c8; color: #fff; }"
        "QPushButton:checked { background: #4682c8; border-color: #3a6ea5; color: #fff; }"
        "QPushButton:checked:hover { background: #5a9ae0; }"
        "QComboBox {"
        "  background: #3c3c40; border: 1px solid #555; border-radius: 4px;"
        "  padding: 4px 8px; color: #d4d4d4; min-height: 20px;"
        "}"
        "QComboBox:hover { border-color: #4682c8; }"
        "QComboBox QAbstractItemView { background: #2d2d30; color: #d4d4d4; selection-background-color: #4682c8; }"
        "QDoubleSpinBox {"
        "  background: #1e1e1e; border: 1px solid #555; border-radius: 4px;"
        "  padding: 4px 8px; color: #d4d4d4; min-height: 20px;"
        "  selection-background-color: #4682c8; selection-color: #fff;"
        "}"
        "QDoubleSpinBox:hover { border-color: #4682c8; }"
        "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { width: 0; border: none; }"
        "QCheckBox { color: #d4d4d4; spacing: 6px; }"
        "QCheckBox::indicator { width: 16px; height: 16px; }"
        "QLabel { color: #a0a0a0; }"
        "QMenuBar { background: #2d2d30; color: #d4d4d4; border-bottom: 1px solid #3c3c40; }"
        "QMenuBar::item:selected { background: #4682c8; }"
        "QMenu { background: #2d2d30; color: #d4d4d4; border: 1px solid #555; }"
        "QMenu::item:selected { background: #4682c8; }"
    );

    DensityViewerWindow w;
    w.show();

    return a.exec();
}

/*TODO wishlist:
 * - In reconstructions when going "home" it would be nice to zoom to the actual reconstructed plane
 * without showing tons of zeros (from the 3d plane shapes)
 *
 * - In Yell format change step_size to step_sizes
*/
