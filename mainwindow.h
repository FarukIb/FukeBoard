#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include <QStackedWidget>
#include <QColor>
#include "appconstants.h"
#include "canvaswidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
private:
    Mode mode;
    QColor m_penColor = AppConstants::DefaultPenColor;

    CanvasWidget *m_canvas = nullptr;

    QToolBar* toolbar;

    QToolBar *m_toolToolbar = nullptr;
    QToolBar *m_optionsToolbar = nullptr;

    QStackedWidget *m_optionsStack = nullptr;

    QWidget *m_penOptions = nullptr;
    QWidget *m_eraserOptions = nullptr;
    QWidget *m_selectOptions = nullptr;

    void createToolToolbar();
    void createOptionsToolbar();

    void selectPenMode();
    void selectEraseMode();
    void selectSelectMode();

    QWidget *createPenOptions();
    QWidget *createEraserOptions();
    QWidget *createSelectOptions();
signals:
};

#endif // MAINWINDOW_H
