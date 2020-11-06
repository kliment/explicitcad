#include <QMouseEvent>
#include <QtGlobal>

#include <cmath>

#include "canvas.h"
#include "backdrop.h"
#include "glmesh.h"
#include "mesh.h"

Canvas::Canvas(const QSurfaceFormat &format, QWidget *parent)
    : QOpenGLWidget(parent), mesh(nullptr), scale(1), zoom(1), tilt(90), yaw(0),
      perspective(0.25), mode(RenderMode::Solid), anim(this, "perspective"),
      status(" ")
{
	setFormat(format);
    QFile styleFile(":/qt/style.qss");
    styleFile.open( QFile::ReadOnly );
    setStyleSheet(styleFile.readAll());

    anim.setDuration(100);
}

Canvas::~Canvas()
{
	makeCurrent();
	delete mesh;
	doneCurrent();
}

void Canvas::view_anim(float v)
{
    anim.setStartValue(perspective);
    anim.setEndValue(v);
    anim.start();
}

void Canvas::view_orthographic()
{
    view_anim(0);
}

void Canvas::view_perspective()
{
    view_anim(0.25);
}

void Canvas::draw_shaded()
{
    set_renderMode(RenderMode::Solid);
}

void Canvas::draw_wireframe()
{
    set_renderMode(RenderMode::Wireframe);
}

void Canvas::reset_cam()
{
    center = meshCenter;
    scale = meshScale;

    // Reset other camera parameters
    zoom = 1;
    setCameraAngle(Direction::Front);
}

void Canvas::setCameraAngle(const enum Direction direction)
{
    switch (direction) {
    case Direction::Front:
        yaw = 0;
        tilt = 90;
        break;
    case Direction::Back:
        yaw = 180;
        tilt = 90;
        break;
    case Direction::Top:
        yaw = 0;
        tilt = 0;
        break;
    case Direction::Bottom:
        yaw = 0;
        tilt = 180;
        break;
    case Direction::Left:
        yaw = -90;
        tilt = 90;
        break;
    case Direction::Right:
        yaw = 90;
        tilt = 90;
        break;
    }

    update();
}

void Canvas::load_mesh(Mesh* m, bool is_reload)
{
    mesh = new GLMesh(m);

    if (!is_reload)
    {
        QVector3D lower(m->xmin(), m->ymin(), m->zmin());
        QVector3D upper(m->xmax(), m->ymax(), m->zmax());
        meshCenter = (lower + upper) / 2;
        meshScale = 2 / (upper - lower).length();

        reset_cam();
    }

    update();

    delete m;
}

void Canvas::set_status(const QString &s)
{
    status = s;
    update();
}

void Canvas::set_perspective(float p)
{
    perspective = p;
    update();
}

void Canvas::set_renderMode(const enum RenderMode mode)
{
    this->mode = mode;
    update();
}

void Canvas::clear_status()
{
    status = "";
    update();
}

void Canvas::initializeGL()
{
    initializeOpenGLFunctions();

    mesh_shader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/gl/mesh.vert");
    mesh_shader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/gl/mesh.frag");
    mesh_shader.link();
    mesh_wireframe_shader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/gl/mesh.vert");
    mesh_wireframe_shader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/gl/mesh_wireframe.frag");
    mesh_wireframe_shader.link();

    small_axes_shader.addShaderFromSourceFile(QOpenGLShader::Vertex,
                                              ":/gl/small_axes.vert");
    small_axes_shader.addShaderFromSourceFile(QOpenGLShader::Fragment,
                                              ":/gl/small_axes.frag");
    small_axes_shader.link();
    small_axes_shader.bind();

    small_axes_vao.create();
    small_axes_vao.bind();

    /* format: R, G, B, x, y, z; 6 times */
    float colors_and_vertices[] = {
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    };

    /* stride is in bytes */
    const auto stride = 6 * sizeof(GL_FLOAT);
    const auto colorOffset = 0;
    const auto vertexOffset = 3 * sizeof(GL_FLOAT);

    small_axes_vertices.create();
    small_axes_vertices.bind();
    small_axes_vertices.setUsagePattern(QOpenGLBuffer::StaticDraw);
    small_axes_vertices.allocate(colors_and_vertices,
                                 sizeof(colors_and_vertices));
    small_axes_shader.enableAttributeArray("vertex_position");
    small_axes_shader.setAttributeBuffer("vertex_position", GL_FLOAT,
                                         vertexOffset, 3, stride);

    small_axes_shader.enableAttributeArray("vertex_color");
    small_axes_shader.setAttributeBuffer("vertex_color", GL_FLOAT, colorOffset,
                                         3, stride);

    small_axes_vertices.release();
    small_axes_vao.release();
    small_axes_shader.release();


    backdrop = new Backdrop();
}


void Canvas::paintGL()
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	backdrop->draw();
	if (mesh)  draw_mesh();

	draw_small_axes();

	if (status.isNull())  return;

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setPen(Qt::white);
	painter.drawText(10, height() - 10, status);
}

void Canvas::draw_mesh()
{
    QOpenGLShaderProgram* selected_mesh_shader = NULL;
    // Set gl draw mode
    switch (mode) {
    case RenderMode::Wireframe:
        selected_mesh_shader = &mesh_wireframe_shader;
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        break;
    case RenderMode::Solid:
        selected_mesh_shader = &mesh_shader;
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;
    }

    selected_mesh_shader->bind();

    // Load the transform and view matrices into the shader
    glUniformMatrix4fv(
                selected_mesh_shader->uniformLocation("transform_matrix"),
                1, GL_FALSE, transform_matrix().data());
    glUniformMatrix4fv(
                selected_mesh_shader->uniformLocation("view_matrix"),
                1, GL_FALSE, view_matrix().data());

    // Compensate for z-flattening when zooming
    glUniform1f(selected_mesh_shader->uniformLocation("zoom"), 1/zoom);

    // Find and enable the attribute location for vertex position
    const GLuint vp = selected_mesh_shader->attributeLocation("vertex_position");
    glEnableVertexAttribArray(vp);

    // Then draw the mesh with that vertex position
    mesh->draw(vp);

    // Reset draw mode for the background and anything else that needs to be drawn
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Clean up state machine
    glDisableVertexAttribArray(vp);
    selected_mesh_shader->release();
}

void Canvas::draw_small_axes()
{
    // orthographic projection via view_matrix
    const float dpi = 1.0f;
    const float aspectratio = (width() * 1.0f) / height();
    auto scale = 9.0f;

    QMatrix4x4 projection;
    QMatrix4x4 view;
    projection.translate(-0.8f, -0.8f, 0.0f);
    projection.ortho(-scale * dpi * aspectratio, scale * dpi * aspectratio,
                     -scale * dpi, scale * dpi, -scale * dpi, scale * dpi);
    const auto model = [this] {
        QMatrix4x4 m;
        m.rotate(-tilt, QVector3D(1, 0, 0));
        m.rotate(-yaw, QVector3D(0, 0, 1));
        return m;
    }();

    small_axes_shader.bind();
    small_axes_vao.bind();

    QMatrix4x4 identity;
    glUniformMatrix4fv(small_axes_shader.uniformLocation("model_matrix"),
                       1, GL_FALSE, model.data());
    glUniformMatrix4fv(small_axes_shader.uniformLocation("view_matrix"), 1,
                       GL_FALSE, view.data());
    glUniformMatrix4fv(small_axes_shader.uniformLocation("projection_matrix"),
                       1, GL_FALSE, projection.data());

    glDrawArrays(GL_LINES, 0, 6);

    small_axes_shader.release();
    small_axes_vao.release();

}

QMatrix4x4 Canvas::transform_matrix() const
{
    QMatrix4x4 m;
    m.rotate(tilt, QVector3D(1, 0, 0));
    m.rotate(yaw,  QVector3D(0, 0, 1));
    m.scale(-scale, scale, -scale);
    m.translate(-center);
    return m;
}

QMatrix4x4 Canvas::view_matrix() const
{
    QMatrix4x4 m;
    if (width() > height())
    {
        m.scale(-height() / float(width()), 1, 0.5);
    }
    else
    {
        m.scale(-1, width() / float(height()), 0.5);
    }
    m.scale(zoom, zoom, 1);
    m(3, 2) = perspective;
    return m;
}

void Canvas::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton ||
        event->button() == Qt::RightButton)
    {
        mouse_pos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void Canvas::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton ||
        event->button() == Qt::RightButton)
    {
        unsetCursor();
    }
}

void Canvas::mouseMoveEvent(QMouseEvent* event)
{
    auto p = event->pos();
    auto d = p - mouse_pos;


    if (event->buttons() & Qt::LeftButton)
    {
        yaw = fmod(yaw - d.x(), 360);
        tilt = fmod(tilt - d.y(), 360);
        update();
    }
    else if (event->buttons() & Qt::RightButton)
    {
        center = transform_matrix().inverted() *
                 view_matrix().inverted() *
                 QVector3D(-d.x() / (0.5*width()),
                            d.y() / (0.5*height()), 0);
        update();
    }
    mouse_pos = p;
}

void Canvas::wheelEvent(QWheelEvent *event)
{
    // Find GL position before the zoom operation
    // (to zoom about mouse cursor)
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    const auto p = event->pos();
#else
    const auto p = event->position();
#endif
    QVector3D v(1 - p.x() / (0.5*width()),
                p.y() / (0.5*height()) - 1, 0);
    QVector3D a = transform_matrix().inverted() *
                  view_matrix().inverted() * v;
    const auto angle = event->angleDelta().y();

    if (angle < 0)
    {
        for (int i=0; i > angle; --i)
            zoom /= 1.001;
    }
    else if (angle > 0)
    {
        for (int i=0; i < angle; ++i)
            zoom *= 1.001;
    }

    // Then find the cursor's GL position post-zoom and adjust center.
    QVector3D b = transform_matrix().inverted() *
                  view_matrix().inverted() * v;
    center += b - a;
    update();
}

void Canvas::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}
