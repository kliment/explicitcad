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
#include <QFileInfo>
#include <QFileDialog>
#include <QIcon>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPoint>
#include <QSettings>
#include <QSize>
#include <QStatusBar>
#include <QTextStream>
#include <QToolBar>

#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexercpp.h>

#include "mainwindow.h"
#include "canvas.h"
#include "loader.h"
#include "preferences.h"

#include <iostream>
#include <string>

using namespace std;

MainWindow::MainWindow()
{
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(2, 1);
    format.setProfile(QSurfaceFormat::CoreProfile);

    QSurfaceFormat::setDefaultFormat(format);
    textEdit = new QsciScintilla;
    textEdit->setWrapMode(QsciScintilla::WrapWord);
    textEdit->setWrapVisualFlags(QsciScintilla::WrapFlagByBorder);
    textEdit->setIndentationsUseTabs(false);
    textEdit->setTabWidth(2);
    textEdit->setIndentationGuides(true);
    textEdit->setAutoIndent(true);
    textEdit->setCaretLineVisible(true);
    textEdit->setMarginType(1,QsciScintilla::NumberMargin);
    lexer=new QsciLexerCPP();
    textEdit->setLexer(lexer);

    outputcon=new QTextEdit(this);

    canvas = new Canvas(format, this);
    splitter=new QSplitter(this);
    splitterR=new QSplitter(this);
    splitterR->setOrientation(Qt::Vertical);
    splitterR->addWidget(canvas);
    splitterR->addWidget(outputcon);
    splitterR->setSizes({200,200});
    splitter->addWidget(textEdit);
    splitter->addWidget(splitterR);
    splitter->setSizes({200,200});
    textEdit->setFocus();
    outputcon->setReadOnly(true);
    outputcon->setAcceptRichText(true);
    outputcon->append("Program started");
    outputcon->append("What now?");

    preferences = new Preferences(this);

    setCentralWidget(splitter);

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    readSettings();

    renderer.process.setProgram("extopenscad");

    connect(&renderer.process, &QProcess::readyReadStandardError,
            [=] { logError(renderer.process.readAllStandardError()); });
    connect(&renderer.process, &QProcess::readyReadStandardOutput,
            [=] { updateLog(renderer.process.readAllStandardOutput()); });
    connect(&renderer.process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
              if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                load_stl(renderer.stl.fileName());
                updateLog("Rendering done.");
              } else {
                logError("Rendering failed.");
              }
            });

    connect(textEdit, SIGNAL(textChanged()),
            this, SLOT(documentWasModified()));

    connect(this, SIGNAL(newlogmsg(QString)), this, SLOT(updateLog(QString)));

    //load_stl(":testfile.stl");
    //load_stl("/home/kliment/designs/implicitcad/ImplicitCAD/testfile.stl");

    setCurrentFile("");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        writeSettings();
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::newFile()
{
    if (maybeSave()) {
        textEdit->clear();
        setCurrentFile("");
    }
}

void MainWindow::open()
{
    if (maybeSave()) {
        QSettings settings{"ImplicitCAD", "ExplicitCAD"};
        const auto lastDir = settings.value("directory/open").toString();
        QString fileName = QFileDialog::getOpenFileName(
            this, tr("Open ImplicitCAD File"), lastDir);
        if (!fileName.isEmpty()) {
            settings.setValue("directory/open",
                              QFileInfo(fileName).dir().absolutePath());
            loadFile(fileName);
        }
    }
}

bool MainWindow::save()
{
    if (curFile.isEmpty()) {
        return saveAs();
    } else {
        return saveFile(curFile);
    }
}

bool MainWindow::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this);
    if (fileName.isEmpty())
        return false;

    return saveFile(fileName);
}

void MainWindow::openPreferences()
{
    preferences->show();
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
    setWindowModified(textEdit->isModified());
}

bool MainWindow::exportSTL(){
    QString fileName = QFileDialog::getSaveFileName(this);
    if (fileName.isEmpty())
        return false;
    on_render(fileName,0.5);
    return true;
}

void MainWindow::render(const QString exportname)
{
    on_render(exportname);
}

void MainWindow::on_render(const QString exportname, float res)
{
    if (renderer.process.state() == QProcess::ProcessState::Running) {
        updateLog("Renderer already running.");
        return;
    }

    if (!renderer.stl.isOpen()) {
        // this actually creates the temporary filename
        renderer.stl.open();
    }

    statusBar()->showMessage(tr("Everyday I'm rendering."));

    renderer.mode = exportname.isEmpty() ? Renderer::Mode::Preview : Renderer::Mode::Export;

    const QString tempfilename = QDir::tempPath() + "/explicitcadtemp.escad";
    saveFile(tempfilename, false);

    auto args = QStringList{tempfilename, "-f", "stl", "-o", exportname.isEmpty() ? renderer.stl.fileName() : exportname};
    if(res>0){
        args.append("-r");
        args.append(QString::number(res));
    }

    renderer.process.setArguments(args);
    renderer.process.start();
    renderer.process.waitForStarted();
}

void MainWindow::logError(const QString & text) {
    outputcon->setTextColor(Qt::red);
    updateLog(text);
    outputcon->setTextColor(Qt::black);
}

void MainWindow::updateLog(const QString &text){
    outputcon->append(text);
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
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    saveAsAct = new QAction(tr("Save &As..."), this);
    saveAsAct->setStatusTip(tr("Save the document under a new name"));
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    prefAct = new QAction(tr("Preferences"), this);
    prefAct->setShortcut(tr("Ctrl+,"));
    prefAct->setStatusTip(tr("Open Preference Dialog"));
    connect(prefAct, SIGNAL(triggered()), this, SLOT(openPreferences()));

    cutAct = new QAction(QIcon(":/images/cut.png"), tr("Cu&t"), this);
    cutAct->setShortcut(tr("Ctrl+X"));
    cutAct->setStatusTip(tr("Cut the current selection's contents to the "
                            "clipboard"));
    connect(cutAct, SIGNAL(triggered()), textEdit, SLOT(cut()));

    copyAct = new QAction(QIcon(":/images/copy.png"), tr("&Copy"), this);
    copyAct->setShortcut(tr("Ctrl+C"));
    copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                             "clipboard"));
    connect(copyAct, SIGNAL(triggered()), textEdit, SLOT(copy()));

    pasteAct = new QAction(QIcon(":/images/paste.png"), tr("&Paste"), this);
    pasteAct->setShortcut(tr("Ctrl+V"));
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                              "selection"));
    connect(pasteAct, SIGNAL(triggered()), textEdit, SLOT(paste()));

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    cutAct->setEnabled(false);
    copyAct->setEnabled(false);
    connect(textEdit, SIGNAL(copyAvailable(bool)),
            cutAct, SLOT(setEnabled(bool)));
    connect(textEdit, SIGNAL(copyAvailable(bool)),
            copyAct, SLOT(setEnabled(bool)));

    renderAct = new QAction(tr("Render"),this);
    renderAct->setShortcut(tr("F5"));
    renderAct->setStatusTip(tr("Render the script to an STL and display it"));
    connect(renderAct, SIGNAL(triggered()),this,SLOT(render()));

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

bool MainWindow::maybeSave()
{
    if (textEdit->isModified()) {
        int ret = QMessageBox::warning(this, tr("Application"),
                     tr("The document has been modified.\n"
                        "Do you want to save your changes?"),
                     QMessageBox::Yes | QMessageBox::Default,
                     QMessageBox::No,
                     QMessageBox::Cancel | QMessageBox::Escape);
        if (ret == QMessageBox::Yes)
            return save();
        else if (ret == QMessageBox::Cancel)
            return false;
    }
    return true;
}

void MainWindow::loadFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::warning(this, tr("Application"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }

    QTextStream in(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    textEdit->setText(in.readAll());
    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File loaded"), 2000);
}

bool MainWindow::saveFile(const QString &fileName, bool setname)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox::warning(this, tr("Application"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }

    QTextStream out(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    out << textEdit->text();
    QApplication::restoreOverrideCursor();

    if(setname)setCurrentFile(fileName);
    statusBar()->showMessage(tr("File saved"), 2000);
    return true;
}

void MainWindow::setCurrentFile(const QString &fileName)
{
    curFile = fileName;
    textEdit->setModified(false);
    setWindowModified(false);

    QString shownName;
    if (curFile.isEmpty())
        shownName = "untitled.txt";
    else
        shownName = strippedName(curFile);

    setWindowTitle(tr("%1[*] - %2").arg(shownName).arg(tr("Application")));
}

QString MainWindow::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}


void MainWindow::on_bad_stl()
{
    QMessageBox::critical(this, "Error",
                          "<b>Error:</b><br>"
                          "This <code>.stl</code> file is invalid or corrupted.<br>"
                          "Please export it from the original source, verify, and retry.");
}

void MainWindow::on_empty_mesh()
{
    QMessageBox::critical(this, "Error",
                          "<b>Error:</b><br>"
                          "This file is syntactically correct<br>but contains no triangles.");
}

void MainWindow::on_confusing_stl()
{
    QMessageBox::warning(this, "Warning",
                         "<b>Warning:</b><br>"
                         "This <code>.stl</code> file begins with <code>solid </code>but appears to be a binary file.<br>"
                         "<code>fstl</code> loaded it, but other programs may be confused by this file.");
}

void MainWindow::on_missing_file()
{
    QMessageBox::critical(this, "Error",
                          "<b>Error:</b><br>"
                          "The target file is missing.<br>");
}

void MainWindow::set_watched(const QString& filename)
{
    (&filename);
    /*const auto files = watcher->files();
    if (files.size())
    {
        watcher->removePaths(watcher->files());
    }
    watcher->addPath(filename);
    */
}


void MainWindow::on_watched_change(const QString& filename)
{

        load_stl(filename, true);
}

void MainWindow::on_autoreload_triggered(bool b)
{
    if (b)
    {
        on_reload();
    }
}


void MainWindow::on_reload()
{
    /*
    auto fs = watcher->files();
    if (fs.size() == 1)
    {
        load_stl(fs[0], true);
    }
    */
}

bool MainWindow::load_stl(const QString& filename, bool is_reload)
{

    canvas->set_status("Loading " + filename);

    Loader* loader = new Loader(this, filename, is_reload);

    connect(loader, &Loader::got_mesh,
            canvas, &Canvas::load_mesh);
    connect(loader, &Loader::error_bad_stl,
              this, &MainWindow::on_bad_stl);
    connect(loader, &Loader::error_empty_mesh,
              this, &MainWindow::on_empty_mesh);
    connect(loader, &Loader::warning_confusing_stl,
              this, &MainWindow::on_confusing_stl);
    connect(loader, &Loader::error_missing_file,
              this, &MainWindow::on_missing_file);

    connect(loader, &Loader::finished,
            loader, &Loader::deleteLater);
    //connect(loader, &Loader::finished,
    //          this, &Window::enable_open);
    connect(loader, &Loader::finished,
            canvas, &Canvas::clear_status);

    if (filename[0] != ':')
    {
        //connect(loader, &Loader::loaded_file,
        //          this, &MainWindow::setWindowTitle);
        connect(loader, &Loader::loaded_file,
                  this, &MainWindow::set_watched);
        //connect(loader, &Loader::loaded_file,
          //        this, &MainWindow::on_loaded);
        //autoreload_action->setEnabled(true);
        //reload_action->setEnabled(true);
    }

    loader->start();
    return true;
}
