#pragma once

#include <QDialog>

class QCheckBox;
class QDialogButtonBox;

class Preferences : public QDialog
{
    Q_OBJECT

public:
    Preferences(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

//public slots:

private:
    QCheckBox *autoRender;
    QDialogButtonBox *buttonBox;
};

