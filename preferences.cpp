#include "preferences.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QSettings>
#include <QVBoxLayout>

Preferences::Preferences(QWidget *parent, Qt::WindowFlags f)
    : QDialog(parent, f)
{
    QSettings settings("ImplicitCAD", "ExplicitCAD");
    const auto autoRenderSetting = settings.value("autorender", false).toBool();
    autoRender = new QCheckBox("Render preview when saving file");
    autoRender->setChecked(autoRenderSetting);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, &QDialogButtonBox::accepted, [=] {
        QSettings settings("ImplicitCAD", "ExplicitCAD");
        settings.setValue("autorender", autoRender->isChecked());
    });
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(autoRender);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
}
