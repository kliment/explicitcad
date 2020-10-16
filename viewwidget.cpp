#include "viewwidget.h"

#include "canvas.h"

ViewWidget::ViewWidget(QWidget *parent)
    : QWidget(parent), canvas(new Canvas(
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
      toolbar(new QToolBar(this))
{
    toolbar->addAction(tr("Orthographic"),
                       [=] { canvas->view_orthographic(); });
    toolbar->addAction(tr("Perspective"), [=] { canvas->view_perspective(); });
    toolbar->addAction(tr("Reset Cam"), canvas, &Canvas::reset_cam);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(canvas);
    mainLayout->addWidget(toolbar);

    setLayout(mainLayout);
}

void ViewWidget::update_mesh(Mesh *m, const bool isReload)
{
    canvas->load_mesh(m, isReload);
}
