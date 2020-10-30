/****************************************************************************
**
** Copyright (C) 2004-2006 Trolltech ASA. All rights reserved.
**
** This file is part of the example classes of the Qt Toolkit.
**
** Licensees holding a valid Qt License Agreement may use this file in
** accordance with the rights, responsibilities and obligations
** contained therein.  Please consult your licensing agreement or
** contact sales@trolltech.com if any conditions of this licensing
** agreement are not clear to you.
**
** Further information about Qt licensing is available at:
** http://www.trolltech.com/products/qt/licensing.html or by
** contacting info@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPoint>
#include <QSettings>
#include <QSize>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextStream>
#include <QToolBar>

#include "mainwindow.h"
#include "preferences.h"
#include "tab.h"

static QString filename(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

static const char *untitled = "untitled.escad";

MainWindow::MainWindow()
    : main(new QTabWidget(this)), preferences(new Preferences(this))
{
    setCentralWidget(main);
    readSettings();
    //main->setDocumentMode(true);
    main->setTabsClosable(true);
    main->setMovable(true);
    newFile();

    createActions();
    createMenus();
    createToolBars();

    createStatusBar();

    connect(main, &QTabWidget::tabCloseRequested,
            [=](const int idx) { closeTab(getTab(idx)); });

    /* update UI on tab change */
    connect(main, &QTabWidget::currentChanged, [=](const int idx) {
        if (idx < 0) {
            return;
        }

        Tab const *const tab = getTab(idx);
        setWindowTitle(tab);
        const bool available = tab->hasSelectedCode();
        cutAct->setEnabled(available);
        copyAct->setEnabled(available);

    });
}

Tab *MainWindow::currentTab() const
{
    return dynamic_cast<Tab *>(main->currentWidget());
}

Tab *MainWindow::getTab(const int idx) const
{
    return dynamic_cast<Tab *>(main->widget(idx));
}

void MainWindow::setWindowTitle(Tab const *const tab)
{
    QString shownName{filename(tab->fileName())};
    if (shownName.isEmpty())
        shownName = untitled;

    static_cast<QMainWindow *>(this)->setWindowTitle(
        tr("%1[*] - %2").arg(shownName).arg(tr("Application")));
    setWindowModified(tab->hasModifiedCode());
}

void MainWindow::setTabText(Tab *const tab, const QString &filename)
{
    main->setTabText(main->indexOf(tab), filename);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    for (; main->count();) {
        const auto tab = currentTab();
        if (!closeTab(tab)) {
            event->ignore();
            return;
        }
    }
    event->accept();
}

bool MainWindow::closeTab(Tab *const tab)
{
    const int idx = main->indexOf(tab);
    if (maybeSave(tab) && idx != -1) {
        main->removeTab(idx);
        delete tab;
        return true;
    }

    return false;
}

void MainWindow::newFile()
{
    Tab *tab = new Tab();
    const int idx = main->addTab(tab, untitled);
    main->setCurrentIndex(idx);
    tab->setFocus();
    setWindowTitle(tab);

    connect(tab, &Tab::copyAvailable, [=](const bool available) {
        cutAct->setEnabled(available);
        copyAct->setEnabled(available);
    });
}

void MainWindow::open()
{
    const auto tab = currentTab();
    if (maybeSave(tab)) {
        QSettings settings{"ImplicitCAD", "ExplicitCAD"};
        const auto lastDir = settings.value("directory/open").toString();
        QString fileName = QFileDialog::getOpenFileName(
            this, tr("Open ImplicitCAD File"), lastDir);
        if (!fileName.isEmpty()) {
            settings.setValue("directory/open",
                              QFileInfo(fileName).dir().absolutePath());
            const bool success = tab->open(fileName);
            if (success) {
                setWindowTitle(tab);
                setTabText(tab, filename(fileName));
            }
        }
    }
}

bool MainWindow::maybeSave(Tab *const tab)
{
    if (tab && tab->hasModifiedCode()) {
        const int ret = QMessageBox::warning(
            this, tr("Application"),
            tr("The document has been modified.\n"
               "Do you want to save your changes?"),
            QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,
            QMessageBox::Cancel | QMessageBox::Escape);
        if (ret == QMessageBox::Yes)
            return save(tab);
        else if (ret == QMessageBox::Cancel)
            return false;
    }
    return true;
}

bool MainWindow::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this);
    if (fileName.isEmpty())
        return false;

    const auto tab = currentTab();
    tab->setFileName(fileName);

    save(tab);
    return true;
}

bool MainWindow::save(Tab *const tab)
{
    if (!tab->hasFile()) {
        QString fileName = QFileDialog::getSaveFileName(this);
        if (fileName.isEmpty())
            return false;
        tab->setFileName(fileName);
    }

    const bool success = tab->save();
    if (success) {
        statusBar()->showMessage(tr("File saved"), 2000);
        setWindowTitle(tab);
        setTabText(tab, filename(tab->fileName()));
    }
    return success;
}

void MainWindow::about()
{
   QMessageBox::about(this, tr("About Application"),
            tr("The <b>Application</b> example demonstrates how to "
               "write modern GUI applications using Qt, with a menu bar, "
               "toolbars, and a status bar."));
}

void MainWindow::documentWasModified()
{
    setWindowModified(true);
}

bool MainWindow::exportSTL()
{
    QString fileName = QFileDialog::getSaveFileName(this);
    if (fileName.isEmpty())
        return false;
    currentTab()->render(fileName, 0.5);
    return true;
}

void MainWindow::createActions()
{
    newAct = new QAction(QIcon(":/images/new.png"), tr("&New"), this);
    newAct->setShortcut(tr("Ctrl+N"));
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, SIGNAL(triggered()), this, SLOT(newFile()));

    openAct = new QAction(QIcon(":/images/open.png"), tr("&Open..."), this);
    openAct->setShortcut(tr("Ctrl+O"));
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    saveAct = new QAction(QIcon(":/images/save.png"), tr("&Save"), this);
    saveAct->setShortcut(tr("Ctrl+S"));
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, &QAction::triggered, [=] { save(currentTab()); });

    saveAsAct = new QAction(tr("Save &As..."), this);
    saveAsAct->setStatusTip(tr("Save the document under a new name"));
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

    closeTabAct = new QAction(tr("Close Tab"), this);
    closeTabAct->setShortcut(tr("Ctrl+W"));
    closeTabAct->setStatusTip(
        tr("Close the document in the currently active tab"));
    connect(closeTabAct, &QAction::triggered,
            [=](const int idx) { closeTab(currentTab()); });

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    prefAct = new QAction(tr("Preferences"), this);
    prefAct->setShortcut(tr("Ctrl+,"));
    prefAct->setStatusTip(tr("Open Preference Dialog"));
    connect(prefAct, &QAction::triggered, [this] { preferences->show(); });

    cutAct = new QAction(QIcon(":/images/cut.png"), tr("Cu&t"), this);
    cutAct->setShortcut(tr("Ctrl+X"));
    cutAct->setStatusTip(tr("Cut the current selection's contents to the "
                            "clipboard"));
    connect(cutAct, &QAction::triggered, [=] { currentTab()->cut(); });

    copyAct = new QAction(QIcon(":/images/copy.png"), tr("&Copy"), this);
    copyAct->setShortcut(tr("Ctrl+C"));
    copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                             "clipboard"));
    connect(copyAct, &QAction::triggered, [=] { currentTab()->copy(); });

    pasteAct = new QAction(QIcon(":/images/paste.png"), tr("&Paste"), this);
    pasteAct->setShortcut(tr("Ctrl+V"));
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                              "selection"));
    connect(pasteAct, &QAction::triggered, [=] { currentTab()->paste(); });

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    cutAct->setEnabled(false);
    copyAct->setEnabled(false);

    renderAct = new QAction(tr("Render"),this);
    renderAct->setShortcut(tr("F5"));
    renderAct->setStatusTip(tr("Render the script to an STL and display it"));
    connect(renderAct, &QAction::triggered, [=] { currentTab()->preview(); });

    exportAct = new QAction(tr("Render+Export"),this);
    exportAct->setShortcut(tr("F6"));
    exportAct->setStatusTip(tr("Render the script to a high resolution STL and display it"));
    connect(exportAct, SIGNAL(triggered()),this,SLOT(exportSTL()));
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addAction(renderAct);
    fileMenu->addAction(exportAct);
    fileMenu->addSeparator();
    fileMenu->addAction(closeTabAct);
    fileMenu->addAction(exitAct);

    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(cutAct);
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);

    helpMenu->addSeparator();

    helpMenu->addAction(prefAct);
}

void MainWindow::createToolBars()
{
    fileToolBar = addToolBar(tr("File"));
    fileToolBar->addAction(newAct);
    fileToolBar->addAction(openAct);
    fileToolBar->addAction(saveAct);

    editToolBar = addToolBar(tr("Edit"));
    editToolBar->addAction(cutAct);
    editToolBar->addAction(copyAct);
    editToolBar->addAction(pasteAct);
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::readSettings()
{
    QSettings settings("ImplicitCAD", "ExplicitCAD");
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();

    if (!settings.contains("directory/open")) {
        settings.setValue("directory/open",
                          QStandardPaths::standardLocations(
                              QStandardPaths::DocumentsLocation)[0]);
    }

    resize(size);
    move(pos);
}

void MainWindow::writeSettings()
{
    QSettings settings("ImplicitCAD", "ExplicitCAD");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
}

