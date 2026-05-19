#include "mainwindow.h"

#include <QActionGroup>
#include <iostream>
#include <QLabel>
#include <QBoxLayout>
#include <QSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QColorDialog>
#include <QColor>
#include <QPushButton>

namespace {
constexpr int DefaultWindowHeight = 1000;
constexpr int DefaultWindowWidth = 700;

constexpr int MinPenWidth = 1;
constexpr int MaxPenWidth = 50;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow{parent}
{
    m_canvas = new CanvasWidget(this);
    setCentralWidget(m_canvas);

    createToolToolbar();
    createOptionsToolbar();

    selectPenMode();

    resize(DefaultWindowHeight, DefaultWindowWidth);
    setWindowTitle("FukeBoard");
}

void MainWindow::createToolToolbar() {
    toolbar = addToolBar("Toolbar");

    auto *toolGroup = new QActionGroup(this);
    toolGroup->setExclusive(true);

    auto *selectPenAction = new QAction("Pen mode", this);
    selectPenAction->setCheckable(true);

    auto *selectEraserAction = new QAction("Eraser mode", this);
    selectEraserAction->setCheckable(true);

    auto *selectSelectAction = new QAction("Select mode", this);
    selectSelectAction->setCheckable(true);

    toolGroup->addAction(selectPenAction);
    toolGroup->addAction(selectEraserAction);
    toolGroup->addAction(selectSelectAction);

    toolbar->addAction(selectPenAction);
    toolbar->addAction(selectEraserAction);
    toolbar->addAction(selectSelectAction);

    selectPenAction->setChecked(true);

    connect(selectPenAction, &QAction::triggered, this, &MainWindow::selectPenMode);
    connect(selectEraserAction, &QAction::triggered, this, &MainWindow::selectEraseMode);
    connect(selectSelectAction, &QAction::triggered, this, &MainWindow::selectSelectMode);
}

void MainWindow::createOptionsToolbar()
{
    m_optionsToolbar = addToolBar("Tool Options");

    m_optionsStack = new QStackedWidget(this);

    m_penOptions = createPenOptions();
    m_eraserOptions = createEraserOptions();
    m_selectOptions = createSelectOptions();

    m_optionsStack->addWidget(m_penOptions);
    m_optionsStack->addWidget(m_eraserOptions);
    m_optionsStack->addWidget(m_selectOptions);

    m_optionsToolbar->addWidget(m_optionsStack);
}

QWidget* MainWindow::createPenOptions()
{
    auto *widget = new QWidget(this);
    auto *layout = new QHBoxLayout(widget);

    auto *widthLabel = new QLabel("Pen width:", widget);
    auto *widthSpinBox = new QSpinBox(widget);

    widthSpinBox->setRange(MinPenWidth, MaxPenWidth);
    widthSpinBox->setValue(AppConstants::DefaultPenWidth);

    auto *colorButton = new QPushButton("Color", widget);
    colorButton->setStyleSheet(
        QString("background-color: %1").arg(m_penColor.name())
        );

    layout->addWidget(widthLabel);
    layout->addWidget(widthSpinBox);
    layout->addWidget(colorButton);
    layout->addStretch();

    connect(widthSpinBox, &QSpinBox::valueChanged,
            this, [this](int value) {
                std::cout << "Pen width: " << value << "\n";

                m_canvas->setPenWidth(value);
            });

    connect(colorButton, &QPushButton::clicked,
            this, [this, colorButton]() {
                QColor selectedColor = QColorDialog::getColor(
                    m_penColor,
                    this,
                    "Choose pen color"
                    );

                if (!selectedColor.isValid()) {
                    return; // user cancelled
                }

                m_penColor = selectedColor;

                colorButton->setStyleSheet(
                    QString("background-color: %1").arg(m_penColor.name())
                    );

                std::cout << "Selected color: "
                          << m_penColor.name().toStdString()
                          << std::endl;

                m_canvas->setPenColor(m_penColor);
            });

    return widget;
}

QWidget *MainWindow::createEraserOptions()
{
    auto *widget = new QWidget(this);
    return widget;
}

QWidget *MainWindow::createSelectOptions() {
    auto *widget = new QWidget(this);
    auto *layout = new QHBoxLayout(widget);

    auto *deleteButton = new QPushButton("Delete", widget);
    auto *duplicateButton = new QPushButton("Duplicate", widget);

    layout->addWidget(deleteButton);
    layout->addWidget(duplicateButton);
    layout->addStretch();

    connect(deleteButton, &QPushButton::clicked,
            this, [this]() {
                std::cout << "Delete selected item" << std::endl;

                m_canvas->deleteSelection();
            });

    connect(duplicateButton, &QPushButton::clicked,
            this, [this]() {
                std::cout << "Duplicate selected items" << std::endl;

                m_canvas->duplicateSelection();
            });

    return widget;
}

void MainWindow::selectPenMode() {
    mode = Mode::Pen;
    m_optionsStack->setCurrentWidget(m_penOptions);
    m_canvas->setMode(mode);
    std::cout << "PENNEER" << std::endl;
}

void MainWindow::selectEraseMode() {
    mode = Mode::Erase;
    m_optionsStack->setCurrentWidget(m_eraserOptions);
    m_canvas->setMode(mode);
    std::cout << "ERASE" << std::endl;
}

void MainWindow::selectSelectMode() {
    mode = Mode::Select;
    m_optionsStack->setCurrentWidget(m_selectOptions);
    m_canvas->setMode(mode);
    std::cout << "SELECT" << std::endl;
}
