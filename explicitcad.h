#ifndef EXPLICITCAD_H
#define EXPLICITCAD_H

#include <QMainWindow>

namespace Ui {
class ExplicitCad;
}

class ExplicitCad : public QMainWindow
{
    Q_OBJECT

public:
    explicit ExplicitCad(QWidget *parent = 0);
    ~ExplicitCad();

private:
    Ui::ExplicitCad *ui;
};

#endif // EXPLICITCAD_H
