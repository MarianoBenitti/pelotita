#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QtMath"





MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QPaintBall = new QPaintBox(0,0,ui->widget);//instancio la pantalla al tamaño actual del widget
    TimerGen = new QTimer(this);
    connect(TimerGen, &QTimer::timeout,this,&MainWindow::TimerGen1);
    TimerGen->start(10);
    s_pelotita.InMove=0;
    s_pelotita.alfha=50;
    s_pelotita.anguloI=-40;
    s_pelotita.centroI.setX(15);
    s_pelotita.centroI.setY(-15);
    s_pelotita.velI=10;
    s_pelotita.radio=10;
    s_pelotita.gravedad=0.09;
    s_pelotita.friccion=0.05;
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::TimerGen1(){

    QPainter QPainterEsp(QPaintBall->getCanvas());
    QPainterEsp.translate(0,ui->widget->height());
    QPen pen;
    QBrush brush;
    pen.setColor(Qt::white);
    brush.setColor(Qt::blue);
    brush.setStyle(Qt::SolidPattern);
    QPainterEsp.setPen(pen);
    QPainterEsp.setBrush(brush);


    if(s_pelotita.InMove){
        //borramos la pelotita en la posicion anterior
        pen.setColor(Qt::black);
        brush.setColor(Qt::black);
        QPainterEsp.setPen(pen);
        QPainterEsp.setBrush(brush);
        QPainterEsp.drawEllipse(s_pelotita.centro,s_pelotita.radio,s_pelotita.radio);
        //recalculamos la posicion
        RecalPelota(&s_pelotita);
        //la dibujamos en la posicion actual
        pen.setColor(Qt::white);
        brush.setColor(Qt::blue);
        QPainterEsp.setPen(pen);
        QPainterEsp.setBrush(brush);
        QPainterEsp.drawEllipse(s_pelotita.centro,s_pelotita.radio,s_pelotita.radio);

        ui->Consola->setText(QString("VelocidadInicial:%1\nPosX:%2\nPosY:%3\nAngulo:%4\nVelX:%5\nVelY:%6").arg(s_pelotita.anguloI).arg(s_pelotita.centro.x()).arg(s_pelotita.centro.y()).arg(s_pelotita.angulo*180/M_PI).arg(s_pelotita.velX).arg(s_pelotita.velY));
    }else{
        QPainterEsp.drawEllipse(s_pelotita.centroI,s_pelotita.radio,s_pelotita.radio);
    }




    ui->widget->update();

}





void MainWindow::on_BotonLanzar_clicked()
{
    if(s_pelotita.InMove){
        s_pelotita.InMove=0;
        ui->BotonLanzar->setText("LANZAR");
        BorrarPantalla();
    }else{
        ui->BotonLanzar->setText("RESETEAR");
        s_pelotita.InMove=1;
        s_pelotita.velX=s_pelotita.velI*cos(s_pelotita.anguloI/180*M_PI);
        s_pelotita.velY=s_pelotita.velI*sin(s_pelotita.anguloI/180*M_PI);
        s_pelotita.velY=-s_pelotita.velY;//invertimos el signo porque el eje y esta invertido
        s_pelotita.centro.setX(s_pelotita.centroI.x());
        s_pelotita.centro.setY(s_pelotita.centroI.y());
    }
}

void MainWindow::RecalPelota(s_pelota *s_Pelota){
    int posAntX;
    int posAntY;
    double porcRecVel;//porcentaje de distancia de velocidad recorrida antes del choque
    float distPared;
    posAntX=s_Pelota->centro.x();
    posAntY=s_Pelota->centro.y();

    //calculamos el angulo de movimiento actual
    s_Pelota->angulo=qAtan(-s_Pelota->velY/s_Pelota->velX);

    //cambiamos la posicion
    s_Pelota->centro.setX((s_Pelota->centro.x()+s_Pelota->velX));
    s_Pelota->centro.setY((s_Pelota->centro.y()+s_Pelota->velY));

    //si choca la pared derecha
    if((s_Pelota->centro.x()+s_Pelota->radio)>=ui->widget->width()){
        s_Pelota->velX=(-s_Pelota->velX);//se invierte la velocidad
        //calculo la nueva posicion en base al choque
        //en este caso la posicion anterior en x va a ser la distancia a la pared
        distPared=(posAntX+s_Pelota->radio)-ui->widget->width();
        porcRecVel=qAbs((distPared)/s_Pelota->velX);//se calcula el porcentaje de velocidad recorrido
        //ui->AnguloLineEdit->setText(QString("%1").arg(porcRecVel));

        s_Pelota->velX=s_Pelota->velX-((s_Pelota->alfha/100.0)*s_Pelota->velX);//reducimos la velocidad sumando porque es negativa
        if(s_Pelota->velX>=0){//es decir si se pierde toda la energia
            s_Pelota->velX=0;
        }
        //se calcula la nueva posicion desde la pared tomando el radio y el porcentaje de velocidad que falta por recorrer luego de reducir la misma
        if(porcRecVel>0){
        s_Pelota->centro.setX(qCeil(ui->widget->width()-s_Pelota->radio+s_Pelota->velX*(1-porcRecVel)));
        }

}
    //si choca la pared izquierda
    if((s_Pelota->centro.x()-s_Pelota->radio)<=0){
        s_Pelota->velX=(-s_Pelota->velX);
        //CALCULAMOS LA PERDIDA DE VELOCIDAD
        distPared=(posAntX-s_Pelota->radio);
        porcRecVel=qAbs((distPared)/s_Pelota->velX);//se calcula el porcentaje de velocidad recorrido(DISTANCIA HACIA LA PARED RECORRIDA SOBRE LA VELOCIDAD)
       // ui->AnguloLineEdit->setText(QString("%1").arg(porcRecVel));

        s_Pelota->velX=s_Pelota->velX-((s_Pelota->alfha/100.0)*s_Pelota->velX);//reducimos la velocidad
        if(s_Pelota->velX<=0){//es decir si se pierde toda la energia
        s_Pelota->velX=0;
        }
        //se calcula la nueva posicion desde la pared tomando el radio y el porcentaje de velocidad que falta por recorrer luego de reducir la misma
        if(porcRecVel>0){
        //COLOCAMOS LA NUEVA POSICION
        s_Pelota->centro.setX(qCeil(s_Pelota->radio+s_Pelota->velX*(1-porcRecVel)));
        }

    }

    //si choca el techo
    if((s_Pelota->centro.y()-s_Pelota->radio)<=(-ui->widget->height())){
        s_Pelota->velY=(-s_Pelota->velY);
        //CALCULAMOS LA PERDIDA DE VELOCIDAD
        distPared=(posAntY+ui->widget->height()-s_Pelota->radio);
        porcRecVel=qAbs((distPared)/s_Pelota->velY);//se calcula el porcentaje de velocidad recorrido(DISTANCIA HACIA LA PARED RECORRIDA SOBRE LA VELOCIDAD)
       // ui->AnguloLineEdit->setText(QString("%1").arg(porcRecVel));

        s_Pelota->velY=s_Pelota->velY-((s_Pelota->alfha/100.0)*s_Pelota->velY);//reducimos la velocidad
        if(s_Pelota->velY<=0){//es decir si se pierde toda la energia
        s_Pelota->velY=0;
        }

        s_Pelota->centro.setY(qCeil((-ui->widget->height())+s_Pelota->radio+s_Pelota->velY*(1-porcRecVel)));

    }

    //si choca el pizo
    if((s_Pelota->centro.y()+s_Pelota->radio)>=0){
        s_Pelota->velY=(-s_Pelota->velY);

        //CALCULAMOS LA PERDIDA DE VELOCIDAD
        distPared=(posAntY+s_Pelota->radio);
        porcRecVel=qAbs((distPared)/s_Pelota->velY);//se calcula el porcentaje de velocidad recorrido(DISTANCIA HACIA LA PARED RECORRIDA SOBRE LA VELOCIDAD)
        //ui->AnguloLineEdit->setText(QString("%1").arg(porcRecVel));

        s_Pelota->velY=s_Pelota->velY-((s_Pelota->alfha/100.0)*s_Pelota->velY);//reducimos la velocidad
        if(s_Pelota->velY>=0){//es decir si se pierde toda la energia
        s_Pelota->velY=0;
        }
        if(porcRecVel>0){
        s_Pelota->centro.setY(qCeil((-s_Pelota->radio)+s_Pelota->velY*(1-porcRecVel)));
        }
       }

    //con gravedad
       if((s_Pelota->centro.y()+s_Pelota->radio)<0 && ui->Checkgravedad->isChecked()){//si la pelota no toca el piso tiene aceleracion de la gravedad
        s_Pelota->velY=s_Pelota->velY+s_Pelota->gravedad;
    }
    //con coeficiente de friccion
    if((s_Pelota->centro.y()+s_Pelota->radio)==0 && ui->CheckFriccion->isChecked()){//si la pelota no toca el piso tiene aceleracion de la gravedad
        if(s_Pelota->velX>0){
        s_Pelota->velX=s_Pelota->velX-s_Pelota->friccion;
            if(s_Pelota->velX<=0){//es decir si se pierde toda la energia
                s_Pelota->velX=0;
            }
        }else{
            s_Pelota->velX=s_Pelota->velX+s_Pelota->friccion;
            if(s_Pelota->velX>=0){//es decir si se pierde toda la energia
                s_Pelota->velX=0;
            }
        }
    }
}

void MainWindow::BorrarPantalla(){
    QPainter QPaintEsp(QPaintBall->getCanvas());
    QBrush brush;
    QPen pen;
    pen.setColor(Qt::black);
    brush.setColor(Qt::black);
    brush.setStyle(Qt::SolidPattern);
    QPaintEsp.setPen(pen);
    QPaintEsp.setBrush(brush);

    QPaintEsp.drawRect(0,0,ui->widget->width(),ui->widget->height());
}




void MainWindow::on_BotonAngulo_clicked()
{
    int angulo;
    QString str;
    str=ui->AnguloLineEdit->text();
    angulo=str.toInt();
    s_pelotita.anguloI=angulo;
}


void MainWindow::on_BotonVelocidad_clicked()
{
    int velocidad;
    QString str;
    str=ui->VelocidadLineEdit->text();
    velocidad=str.toInt();
    s_pelotita.velI=velocidad;
}


void MainWindow::on_BotonPosY_clicked()
{

}
