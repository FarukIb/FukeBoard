#include "mainwindow.h"

#include <QActionGroup>
#include <iostream>
#include <QLabel>
#include <QKeySequence>
#include <QBoxLayout>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFontComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFileInfo>
#include <QSpinBox>
#include <QPushButton>
#include <QColorDialog>
#include <QColor>

namespace {
constexpr int DefaultWindowHeight = 1000;
constexpr int DefaultWindowWidth = 700;

constexpr int MinPenWidth = 1;
constexpr int MaxPenWidth = 50;
constexpr int MinTextSize = 6;
constexpr int MaxTextSize = 96;
constexpr int DefaultTextSize = 14;

constexpr int MinMileGridSize = 1;
constexpr int MaxMileGridSize = 128;
constexpr int DefaultMileGridColumns = 16;
constexpr int DefaultMileGridRows = 8;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow{parent}
{
    m_canvas = new CanvasWidget(this);
    setCentralWidget(m_canvas);

    createFileMenu();
    createToolToolbar();
    createInsertToolbar();
    createHistoryToolbar();
    createTextToolbar();
    createOptionsToolbar();

    selectPenMode();

    resize(DefaultWindowHeight, DefaultWindowWidth);
    setWindowTitle("FukeBoard");
}

void MainWindow::createFileMenu()
{
    QMenu *fileMenu = menuBar()->addMenu("File");

    auto *openAction = new QAction("Open...", this);
    openAction->setShortcut(QKeySequence::Open);

    auto *saveAction = new QAction("Save...", this);
    saveAction->setShortcut(QKeySequence::Save);

    auto *saveAsAction = new QAction("Save As...", this);
    saveAsAction->setShortcut(QKeySequence::SaveAs);

    auto *savePdfAsAction = new QAction("Save as PDF...", this);

    fileMenu->addAction(openAction);
    fileMenu->addAction(saveAction);
    fileMenu->addAction(saveAsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(savePdfAsAction);

    connect(openAction, &QAction::triggered, this, &MainWindow::openFukeFile);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFukeFile);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveFukeFileAs);
    connect(savePdfAsAction, &QAction::triggered, this, &MainWindow::savePdfFileAs);
}

void MainWindow::openFukeFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open FukeBoard File",
        QString(),
        "FukeBoard Files (*.fuke);;All Files (*)"
        );

    if (filePath.isEmpty()) {
        return;
    }

    if (!m_canvas->loadFukeFile(filePath)) {
        QMessageBox::warning(this, "Open Failed", "Could not open the selected .fuke file.");
        return;
    }

    m_currentFilePath = filePath;
}

void MainWindow::saveFukeFile()
{
    QString filePath = m_currentFilePath;

    if (filePath.isEmpty()) {
        filePath = QFileDialog::getSaveFileName(
            this,
            "Save FukeBoard File",
            QString(),
            "FukeBoard Files (*.fuke);;All Files (*)"
            );
    }

    if (filePath.isEmpty()) {
        return;
    }

    if (!filePath.endsWith(".fuke", Qt::CaseInsensitive)) {
        filePath += ".fuke";
    }

    if (!m_canvas->saveFukeFile(filePath)) {
        QMessageBox::warning(this, "Save Failed", "Could not save the .fuke file.");
        return;
    }

    m_currentFilePath = filePath;
}

void MainWindow::saveFukeFileAs()
{
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Save FukeBoard File As",
        m_currentFilePath,
        "FukeBoard Files (*.fuke);;All Files (*)"
        );

    if (filePath.isEmpty()) {
        return;
    }

    if (!filePath.endsWith(".fuke", Qt::CaseInsensitive)) {
        filePath += ".fuke";
    }

    if (!m_canvas->saveFukeFile(filePath)) {
        QMessageBox::warning(this, "Save Failed", "Could not save the .fuke file.");
        return;
    }

    m_currentFilePath = filePath;
}

void MainWindow::savePdfFileAs()
{
    QString suggestedPath;
    if (!m_currentFilePath.isEmpty()) {
        QFileInfo fileInfo(m_currentFilePath);
        suggestedPath = fileInfo.path() + "/" + fileInfo.completeBaseName() + ".pdf";
    }

    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Save Canvas as PDF",
        suggestedPath,
        "PDF Files (*.pdf);;All Files (*)"
        );

    if (filePath.isEmpty()) {
        return;
    }

    if (!filePath.endsWith(".pdf", Qt::CaseInsensitive)) {
        filePath += ".pdf";
    }

    if (!m_canvas->exportPdf(filePath)) {
        QMessageBox::warning(this, "PDF Export Failed", "Could not save the canvas as a PDF.");
    }
}

void MainWindow::createToolToolbar() {
    m_toolToolbar = addToolBar("Tools");

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

    m_toolToolbar->addAction(selectPenAction);
    m_toolToolbar->addAction(selectEraserAction);
    m_toolToolbar->addAction(selectSelectAction);

    selectPenAction->setChecked(true);

    connect(selectPenAction, &QAction::triggered, this, &MainWindow::selectPenMode);
    connect(selectEraserAction, &QAction::triggered, this, &MainWindow::selectEraseMode);
    connect(selectSelectAction, &QAction::triggered, this, &MainWindow::selectSelectMode);
}

void MainWindow::createInsertToolbar()
{
    m_insertToolbar = addToolBar("Insert");

    auto *insertImageAction = new QAction("Insert Image", this);
    auto *insertMileGridAction = new QAction("Mile Grid", this);
    auto *mileGridColorButton = new QPushButton("Cell Color", this);
    mileGridColorButton->setStyleSheet(
        QString("background-color: %1").arg(m_mileGridCellColor.name())
        );

    m_insertToolbar->addAction(insertImageAction);
    m_insertToolbar->addAction(insertMileGridAction);
    m_insertToolbar->addWidget(mileGridColorButton);

    connect(insertImageAction, &QAction::triggered,
            this, [this]() {
                const QString filePath = QFileDialog::getOpenFileName(
                    this,
                    "Insert Image",
                    QString(),
                    "Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp);;All Files (*)"
                    );

                if (filePath.isEmpty()) {
                    return;
                }

                m_canvas->insertImageFromFile(filePath);
            });
    connect(insertMileGridAction, &QAction::triggered,
            this, [this]() {
                QDialog dialog(this);
                dialog.setWindowTitle("Create Mile Grid");

                auto *layout = new QFormLayout(&dialog);
                auto *columnsSpinBox = new QSpinBox(&dialog);
                auto *rowsSpinBox = new QSpinBox(&dialog);
                auto *buttonBox = new QDialogButtonBox(
                    QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                    &dialog
                    );

                columnsSpinBox->setRange(MinMileGridSize, MaxMileGridSize);
                columnsSpinBox->setValue(DefaultMileGridColumns);
                rowsSpinBox->setRange(MinMileGridSize, MaxMileGridSize);
                rowsSpinBox->setValue(DefaultMileGridRows);

                layout->addRow("Columns:", columnsSpinBox);
                layout->addRow("Rows:", rowsSpinBox);
                layout->addRow(buttonBox);

                connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
                connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

                if (dialog.exec() != QDialog::Accepted) {
                    return;
                }

                m_canvas->insertMileGrid(columnsSpinBox->value(), rowsSpinBox->value());
            });
    connect(mileGridColorButton, &QPushButton::clicked,
            this, [this, mileGridColorButton]() {
                QColor selectedColor = QColorDialog::getColor(
                    m_mileGridCellColor,
                    this,
                    "Choose Mile Grid cell color"
                    );

                if (!selectedColor.isValid()) {
                    return;
                }

                m_mileGridCellColor = selectedColor;
                mileGridColorButton->setStyleSheet(
                    QString("background-color: %1").arg(m_mileGridCellColor.name())
                    );
                m_canvas->setMileGridCellColor(m_mileGridCellColor);
            });
}

void MainWindow::createHistoryToolbar()
{
    m_historyToolbar = new QToolBar("History", this);
    addToolBar(Qt::BottomToolBarArea, m_historyToolbar);

    auto *undoAction = new QAction("Undo", this);
    undoAction->setShortcut(QKeySequence::Undo);

    auto *redoAction = new QAction("Redo", this);
    redoAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Y));

    m_historyToolbar->addAction(undoAction);
    m_historyToolbar->addAction(redoAction);

    connect(undoAction, &QAction::triggered, m_canvas, &CanvasWidget::undo);
    connect(redoAction, &QAction::triggered, m_canvas, &CanvasWidget::redo);
}

void MainWindow::createTextToolbar()
{
    m_textToolbar = new QToolBar("Text", this);
    addToolBar(Qt::BottomToolBarArea, m_textToolbar);

    auto *fontComboBox = new QFontComboBox(this);
    auto *fontSizeSpinBox = new QSpinBox(this);
    fontSizeSpinBox->setRange(MinTextSize, MaxTextSize);
    fontSizeSpinBox->setValue(DefaultTextSize);

    auto *boldAction = new QAction("B", this);
    boldAction->setCheckable(true);
    boldAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));

    auto *italicAction = new QAction("I", this);
    italicAction->setCheckable(true);
    italicAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));

    auto *underlineAction = new QAction("U", this);
    underlineAction->setCheckable(true);
    underlineAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_U));

    auto *textColorButton = new QPushButton("Text Color", this);
    textColorButton->setStyleSheet(QString("background-color: %1").arg(m_textColor.name()));

    m_textToolbar->addWidget(fontComboBox);
    m_textToolbar->addWidget(fontSizeSpinBox);
    m_textToolbar->addAction(boldAction);
    m_textToolbar->addAction(italicAction);
    m_textToolbar->addAction(underlineAction);
    m_textToolbar->addWidget(textColorButton);

    connect(fontComboBox, &QFontComboBox::currentFontChanged,
            this, [this](const QFont &font) {
                m_canvas->setActiveTextFontFamily(font.family());
            });

    connect(fontSizeSpinBox, &QSpinBox::valueChanged,
            this, [this](int value) {
                m_canvas->setActiveTextFontPointSize(value);
            });

    connect(boldAction, &QAction::toggled,
            m_canvas, &CanvasWidget::setActiveTextBold);

    connect(italicAction, &QAction::toggled,
            m_canvas, &CanvasWidget::setActiveTextItalic);

    connect(underlineAction, &QAction::toggled,
            m_canvas, &CanvasWidget::setActiveTextUnderline);

    connect(textColorButton, &QPushButton::clicked,
            this, [this, textColorButton]() {
                QColor selectedColor = QColorDialog::getColor(
                    m_textColor,
                    this,
                    "Choose text color"
                    );

                if (!selectedColor.isValid()) {
                    return;
                }

                m_textColor = selectedColor;
                textColorButton->setStyleSheet(
                    QString("background-color: %1").arg(m_textColor.name())
                    );

                m_canvas->setActiveTextColor(m_textColor);
            });
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
