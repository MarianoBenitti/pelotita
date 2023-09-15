#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qpaintbox.h>
#include <QPainter>
#include <QTimer>



//declaramos la estructura de la pelotita
typedef struct {
    QPoint centro;//centro de la pelotita actual
    QPoint centroI;//centro de la pelotita inicial
    float angulo;//angulo actual
    float anguloI;//angulo inicial
    float velI;//velocidad inicial
    int vel;//velocidad actual
    float velX;//velocidad en X
    float velY;//velocidad en Y
    float gravedad;
    float friccion;
    int radio;//radio de la pelotita
    int alfha;//porcentaje de perdida de energia
    uint8_t InMove;//vale 1 si esta en movimiento o 0 si esta detenida
}s_pelota;


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void TimerGen1();//metodo enlazado con el timer
    void RecalPelota(s_pelota *s_Pelota);//recalcula la posicion de la pelota segun los parametros
    void BorrarPantalla();
private slots:
    void on_BotonLanzar_clicked();

    void on_BotonAngulo_clicked();

    void on_BotonVelocidad_clicked();

    void on_BotonPosY_clicked();

private:
    Ui::MainWindow *ui;
    s_pelota s_pelotita;
    QPaintBox *QPaintBall;//pantalla donde se pintara
    QTimer *TimerGen;//timer general

};
#endif // MAINWINDOW_H