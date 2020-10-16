#pragma once

#include <QWidget>

class Canvas;
class Mesh;
class QToolBar;

class ViewWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit ViewWidget(QWidget *parent = nullptr);

  public slots:
    void update_mesh(Mesh *m, const bool isReload = false);

  private:
    Canvas *canvas;
    QToolBar *toolbar;
};
