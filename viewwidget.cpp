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

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(canvas);
    mainLayout->addWidget(toolbar);

    setLayout(mainLayout);
}

void ViewWidget::update_mesh(Mesh *m, const bool isReload)
{
    canvas->load_mesh(m, isReload);
}
