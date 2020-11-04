#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QSettings>
#include <QSplitter>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QToolBar>

#include <Qsci/qscilexercpp.h>
#include <Qsci/qscilexer.h>

#include <cmath>

#include "loader.h"
#include "tab.h"
#include "canvas.h"

Tab::Tab(QWidget *parent)
    : QWidget(parent), code(new QsciScintilla()), lexer(new QsciLexerCPP()),
      canvas(new Canvas(
          [] {
              QSurfaceFormat format;
              format.setDepthBufferSize(24);
              format.setStencilBufferSize(8);
              format.setVersion(2, 1);
              format.setProfile(QSurfaceFormat::CoreProfile);

              QSurfaceFormat::setDefaultFormat(format);
              return format;
          }(),
          this)),
      toolbar(new QToolBar(this)), console(new QTextEdit()),
      v_splitter(new QSplitter()), h_splitter(new QSplitter())
{

    code->setWrapMode(QsciScintilla::WrapWord);
    code->setWrapVisualFlags(QsciScintilla::WrapFlagByBorder);
    code->setIndentationsUseTabs(false);
    code->setTabWidth(2);
    code->setIndentationGuides(true);
    code->setAutoIndent(true);
    code->setCaretLineVisible(true);
    code->setMarginType(1, QsciScintilla::NumberMargin);
    code->setLexer(lexer);

    connect(code, &QsciScintilla::copyAvailable,
            [=](const bool available) { emit copyAvailable(available); });
    connect(code, &QsciScintilla::linesChanged, [=] {
        const auto lines = code->lines();
        const auto digits = std::floor(std::log10(lines) + 1);
        code->setMarginWidth(1, QString("0").repeated(digits+1));
    });

    toolbar->addAction(tr("Orthographic"),
                       [=] { canvas->view_orthographic(); });
    toolbar->addAction(tr("Perspective"), [=] { canvas->view_perspective(); });
    toolbar->addAction(tr("Reset Cam"), canvas, &Canvas::reset_cam);

    toolbar->addAction(
        tr("Front"), [=] { canvas->setCameraAngle(Canvas::Direction::Front); });
    toolbar->addAction(
        tr("Back"), [=] { canvas->setCameraAngle(Canvas::Direction::Back); });
    toolbar->addAction(tr("Top"),
                       [=] { canvas->setCameraAngle(Canvas::Direction::Top); });
    toolbar->addAction(tr("Bottom"), [=] {
        canvas->setCameraAngle(Canvas::Direction::Bottom);
    });
    toolbar->addAction(
        tr("Left"), [=] { canvas->setCameraAngle(Canvas::Direction::Left); });
    toolbar->addAction(
        tr("Right"), [=] { canvas->setCameraAngle(Canvas::Direction::Right); });

    auto preview_and_controls = new QWidget();
    auto preview_layout = new QVBoxLayout();
    preview_layout->setContentsMargins(0, 0, 0, 0);
    preview_layout->addWidget(canvas);
    preview_layout->addWidget(toolbar);
    preview_and_controls->setLayout(preview_layout);

    console->setReadOnly(true);
    console->setAcceptRichText(true);

    v_splitter->setOrientation(Qt::Vertical);
    v_splitter->addWidget(preview_and_controls);
    v_splitter->addWidget(console);
    v_splitter->setSizes({200, 200});
    h_splitter->addWidget(code);
    h_splitter->addWidget(v_splitter);
    h_splitter->setSizes({200, 200});

    QGridLayout *layout = new QGridLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(h_splitter);
    setLayout(layout);

    process.setProgram("extopenscad");

    connect(&process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                log(process.readAllStandardOutput());
                logError(process.readAllStandardError());
                if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                    load_stl(stl.fileName(), reload);
                    reload = true;
                    log("Rendering done.");
                } else {
                    logError("Rendering failed.");
                }
                canvas->set_status("");
            });

    setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(code);
    code->setFocus();
}

void Tab::log(const QString &str) const { console->append(str); }

void Tab::logError(const QString &str) const
{
    console->setTextColor(Qt::red);
    log(str);
    console->setTextColor(Qt::black);
}

bool Tab::hasModifiedCode() const { return code->isModified(); }
bool Tab::hasSelectedCode() const { return code->hasSelectedText(); }

void Tab::newFile()
{
    console->clear();
    reload = false;
}

bool Tab::hasFile() const { return !curFile.isEmpty(); }
void Tab::setFileName(const QString &filename)
{
    curFile = filename;
    emit fileNameChanged(curFile);
}

const QString &Tab::fileName() const { return curFile; }

std::pair<bool, QString> Tab::writeFile(const QString &fileName) const
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly)) {
        return std::make_pair(false, file.errorString());
    }

    QTextStream out(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    out << code->text();
    QApplication::restoreOverrideCursor();

    return std::make_pair(true, "");
}

void Tab::call_implicitcad(const QString &inputFile, const QString outputFile,
                           const float resolution, const QString &format)
{
    if (process.state() == QProcess::ProcessState::Running) {
        log("Renderer already running.");
        return;
    }

    auto args = QStringList{inputFile, "-f", format, "-o", outputFile};

    if (resolution > 0) {
        args.append("-r");
        args.append(QString::number(resolution));
    }

    qDebug() << args;

    process.setArguments(args);
    process.start();
    process.waitForStarted();
}

static const QString err_bad_stl{
    "<b>Error:</b><br>"
    "This <code>.stl</code> file is invalid or corrupted.<br>"
    "Please export it from the original source, verify, and retry."};

static const QString err_empty_mesh{
    "<b>Error:</b><br>"
    "This file is syntactically correct<br>but contains no triangles."};

static const QString err_missing_file{"<b>Error:</b><br>"
                                      "The target file is missing.<br>"};

static const QString warn_confusing_stl{
    "<b>Warning:</b><br>"
    "This <code>.stl</code> file begins with <code>solid </code>but appears to "
    "be a binary file.<br>"
    "<code>fstl</code> loaded it, but other programs may be confused by this "
    "file."};

void Tab::load_stl(const QString &fileName, const bool reload)
{
    //canvas->set_status("Loading " + filename);

    Loader* loader = new Loader(this, fileName, reload);

    connect(loader, &Loader::got_mesh,
            canvas, &Canvas::load_mesh);
//
//    QMessageBox::critical(this, "Error",

    connect(
        loader, &Loader::error_bad_stl, this, [=] { logError(err_bad_stl); },
        Qt::QueuedConnection);
    connect(
        loader, &Loader::error_empty_mesh, this,
        [=] { logError(err_empty_mesh); }, Qt::QueuedConnection);
    connect(
        loader, &Loader::error_missing_file, this,
        [=] { logError(err_missing_file); }, Qt::QueuedConnection);
    connect(
        loader, &Loader::warning_confusing_stl, this,
        [=] { logError(warn_confusing_stl); }, Qt::QueuedConnection);
    // TODO: maybe we can re-use the loader obj
    connect(loader, &Loader::finished,
            loader, &Loader::deleteLater);
    //connect(loader, &Loader::finished,
    //          this, &Window::enable_open);
    //connect(loader, &Loader::finished,
    //        canvas, &Canvas::clear_status);

    loader->start();
}

bool Tab::save()
{
    const auto ret = writeFile(curFile);

    if (!ret.first) {
        QMessageBox::warning(this, tr("Application"),
                             tr("Cannot write file %1:\n%2.")
                                 .arg(curFile)
                                 .arg(ret.second));
        return false;
    }

    code->setModified(false);

    {
        QSettings settings("ImplicitCAD", "ExplicitCAD");
        if (settings.value("autorender", false).toBool()) {
            preview();
        }
    }

    return true;
}

bool Tab::open(const QString& fileName) {
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::warning(this, tr("Application"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }

    QTextStream in(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    code->setText(in.readAll());
    QApplication::restoreOverrideCursor();
    code->setModified(false);

    setFileName(fileName);

    return true;
}


void Tab::preview(const float res) {
    if (process.state() == QProcess::ProcessState::Running) {
        log("Renderer already running.");
        return;
    }

    if (!stl.isOpen()) {
        // this actually creates the temporary filename
        stl.open();
    }

    const QString tempfilename = QDir::tempPath() + "/explicitcadtemp.escad";
    const auto ret = writeFile(tempfilename);
    if (!ret.first) {
        // TODO: Display user massage failure
        return;
    }

    canvas->set_status("Rendering preview …");
    call_implicitcad(tempfilename, stl.fileName());
}

void Tab::render(const QString &fileName, const float res)
{
    // TODO save if 'curFile' has been modified or is empty …
    call_implicitcad(curFile, fileName, res);
}

void Tab::cut() { code->cut(); }
void Tab::copy() { code->copy(); }
void Tab::paste() { code->paste(); }
