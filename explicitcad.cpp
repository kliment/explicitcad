#include "explicitcad.h"
#include "ui_explicitcad.h"

ExplicitCad::ExplicitCad(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ExplicitCad)
{
    ui->setupUi(this);
}

ExplicitCad::~ExplicitCad()
{
    delete ui;
}
