#pragma once

#include <QProcess>
#include <QString>
#include <QTemporaryFile>
#include <QWidget>

#include <Qsci/qsciscintilla.h>

class ViewWidget;
class QTextEdit;
class ViewWidget;
class QSplitter;
class QsciLexer;
class Canvas;

class Tab : public QWidget
{
    Q_OBJECT

  public:
    Tab(QWidget *parent = nullptr);

  private:
    QsciScintilla *code;
    QsciLexer *lexer;
    Canvas *canvas;
    QToolBar *toolbar;
    QTextEdit *console;
    QSplitter *v_splitter;
    QSplitter *h_splitter;

    QString curFile;

    QProcess process;
    QTemporaryFile stl;
    bool reload = false;
    QString stderr_;
    QString stdout_;

    void log(const QString &) const;
    void logError(const QString &) const;
    std::pair<bool, QString> writeFile(const QString &) const;
    void call_implicitcad(const QString &inputFile, const QString outputFile,
                          const float resolution = 0,
                          const QString &format = "stl");
    void load_stl(const QString &filename, const bool reload = false);
  signals:
    void fileNameChanged(const QString &fileName);
    void copyAvailable(bool) const;

  public:
    bool hasModifiedCode() const;
    bool hasSelectedCode() const;
    void newFile();
    bool hasFile() const;
    void setFileName(const QString &fileName);
    const QString &fileName() const;
    bool open(const QString& fileName);

  public slots:
    bool save();
    void preview(float res = 0);
    void render(const QString &fileName, float res = 0.5);
    void cut();
    void copy();
    void paste();
};
