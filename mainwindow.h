#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include <QStackedWidget>
#include <QColor>
#include <QString>
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
    QColor m_textColor = Qt::black;
    QColor m_mileGridCellColor = QColor(20, 20, 20);
    QString m_currentFilePath;

    CanvasWidget *m_canvas = nullptr;

    QToolBar *m_toolToolbar = nullptr;
    QToolBar *m_insertToolbar = nullptr;
    QToolBar *m_optionsToolbar = nullptr;
    QToolBar *m_historyToolbar = nullptr;
    QToolBar *m_textToolbar = nullptr;

    QStackedWidget *m_optionsStack = nullptr;

    QWidget *m_penOptions = nullptr;
    QWidget *m_eraserOptions = nullptr;
    QWidget *m_selectOptions = nullptr;

    void createFileMenu();
    void createToolToolbar();
    void createInsertToolbar();
    void createHistoryToolbar();
    void createTextToolbar();
    void createOptionsToolbar();

    void selectPenMode();
    void selectEraseMode();
    void selectSelectMode();

    QWidget *createPenOptions();
    QWidget *createEraserOptions();
    QWidget *createSelectOptions();
    void openFukeFile();
    void saveFukeFile();
    void saveFukeFileAs();
    void savePdfFileAs();
signals:
};

#endif // MAINWINDOW_H
