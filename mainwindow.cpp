#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QtMath"

#define ISNEWCMD banderas.bit.b0
//enumeracion con las id de los comandos
typedef enum{ALIVEID=0xF0,ACKID=0xF0,FIRMWAREID=0XF1,LEDSID=0x10,BUTTONSID=0x12,WALLID=0x18}e_IDSCMD;


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QPaintBall = new QPaintBox(0,0,ui->widget);//instancio la pantalla al tamaño actual del widget
    TimerGen = new QTimer(this);
    connect(TimerGen, &QTimer::timeout,this,&MainWindow::TimerGen1);
    TimerGen->start(10);
    QSerialPort1= new QSerialPort(this);
    connect(QSerialPort1,&QSerialPort::readyRead,this,&MainWindow::OnRxQSerialPort1);

    //INICIALIZAMOS BUFFER DE ENTRADA
    datosLec.iRL=0;
    datosLec.iRE=0;
    datosLec.nBytes=0;
    datosLec.iDatos=0;
    datosLec.checksum=0;
    datosLec.estDecode=EST_U;
    datosLec.tamBuffer=255;
    datosLec.bufL=bufferL;
    datosLec.iChecksum=0;
    //INICIALIZAOS EL BUFFER DE SALIDA
    datosEsc.iTL=0;
    datosEsc.iTE=0;
    datosEsc.checksum=0;
    datosEsc.bufE=bufferE;
    datosEsc.tamBuffer=255;


    s_pelotita.InMove=0;
    s_pelotita.alfha=10;
    s_pelotita.anguloI=-40;
    s_pelotita.escMetro=100;
    s_pelotita.radio=10;
    s_pelotita.centroI.setX(10+s_pelotita.radio);
    s_pelotita.centroI.setY(-(10+s_pelotita.radio));
    s_pelotita.velI=10;
    s_pelotita.gravedad=9.8;
    s_pelotita.friccion=0.05;
    s_pelotita.tx=0;
    s_pelotita.ty=0;
    gravedad=0;
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::TimerGen1(){
    QPoint CentroAnt;
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

        if(ui->CheckTrayectoria->isChecked()){

                pen.setColor(Qt::white);
                brush.setColor(Qt::cyan);
                QPainterEsp.setPen(pen);
                QPainterEsp.setBrush(brush);
                QPainterEsp.drawEllipse(s_pelotita.centro,s_pelotita.radio,s_pelotita.radio);


        }
        //recalculamos la posicion
        RecalPelota2(&s_pelotita);
        //la dibujamos en la posicion actual
        pen.setColor(Qt::white);
        brush.setColor(Qt::blue);
        QPainterEsp.setPen(pen);
        QPainterEsp.setBrush(brush);
        QPainterEsp.drawEllipse(s_pelotita.centro,s_pelotita.radio,s_pelotita.radio);

        ui->Consola->setText(QString("VelocidadInicial:%1\nPosX:%2\nPosY:%3\nAngulo:%4\nVelX:%5\nVelY:%6").arg(s_pelotita.anguloI).arg(s_pelotita.centro.x()).arg(s_pelotita.centro.y()).arg(s_pelotita.anguloI*180/M_PI).arg(s_pelotita.velX).arg(s_pelotita.velY));
    }else{
        QPainterEsp.drawEllipse(s_pelotita.centroI,s_pelotita.radio,s_pelotita.radio);
    }


    if (datosLec.iRE!=datosLec.iRL){
        DecodeCMD();
        ui->commandsConsole->appendPlainText("llega comando");
    }
    if(ISNEWCMD){
        ExecuteCMD(&datosLec);
        ISNEWCMD=0;
    }


    ui->widget->update();

}





void MainWindow::on_BotonLanzar_clicked()
{
    if(s_pelotita.InMove){
        s_pelotita.InMove=0;
        if(ui->Checkgravedad->isChecked()){
                gravedad=s_pelotita.gravedad;
        }else{
                gravedad=0;
        }
        ui->BotonLanzar->setText("LANZAR");
        BorrarPantalla();
    }else{
        ui->BotonLanzar->setText("RESETEAR");
        s_pelotita.InMove=1;
        s_pelotita.velX=s_pelotita.velI*qCos(s_pelotita.anguloI/180*M_PI);
        s_pelotita.velY=s_pelotita.velI*qSin(s_pelotita.anguloI/180*M_PI);
        s_pelotita.velY=-s_pelotita.velY;//invertimos el signo porque el eje y esta invertido
        s_pelotita.velIX=s_pelotita.velX;
        s_pelotita.velIY=s_pelotita.velY;
        s_pelotita.centro.setX(s_pelotita.centroI.x());
        s_pelotita.centro.setY(s_pelotita.centroI.y());
        s_pelotita.posAntX=s_pelotita.centroI.x();
        s_pelotita.posAntY=s_pelotita.centroI.y();
        s_pelotita.posIX=s_pelotita.centroI.x();
        s_pelotita.posIY=s_pelotita.centroI.y();
        s_pelotita.tx=0;
        s_pelotita.ty=0;

    }
}

void MainWindow::RecalPelota(s_pelota *s_Pelota){
    float posAntX;
    float posAntY;
    double porcRecVel;//porcentaje de distancia de velocidad recorrida antes del choque
    float distPared;


    posAntX=s_Pelota->posAntX;
    posAntY=s_Pelota->posAntY;

    //con gravedad
    if((s_Pelota->centro.y()+s_Pelota->radio)<0 && ui->Checkgravedad->isChecked()){//si la pelota no toca el piso tiene aceleracion de la gravedad
        s_Pelota->velY=s_Pelota->velY+(s_Pelota->gravedad/100*s_Pelota->escMetro);
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
    //cambiamos la posicion
    s_Pelota->posAntX=s_Pelota->posAntX+s_Pelota->velX*s_Pelota->escMetro/100.0;
    s_Pelota->posAntY=s_Pelota->posAntY+s_Pelota->velY*s_Pelota->escMetro/100.0;
    s_Pelota->centro.setX(qCeil(s_Pelota->posAntX+0.5));//se divide por 100 porque es la cantidad de veces que entran 10 ms en un segundo
    s_Pelota->centro.setY(qCeil(s_Pelota->posAntY-0.5));

    //si choca la pared derecha
    if((s_Pelota->centro.x()+s_Pelota->radio)>=ui->widget->width()){
        s_Pelota->velX=(-s_Pelota->velX);//se invierte la velocidad
        //calculo la nueva posicion en base al choque
        //en este caso la posicion anterior en x va a ser la distancia a la pared
        distPared=(posAntX+s_Pelota->radio)-ui->widget->width();
        porcRecVel=qAbs((distPared)/s_Pelota->velX);//se calcula el porcentaje de velocidad recorrido
        //ui->AnguloLineEdit->setText(QString("%1").arg(porcRecVel));

        s_Pelota->velX=s_Pelota->velX-((s_Pelota->alfha/100.0)*s_Pelota->velX);//reducimos la velocidad
        if(s_Pelota->velX>=0){//es decir si se pierde toda la energia
            s_Pelota->velX=0;
        }
        //se calcula la nueva posicion desde la pared tomando el radio y el porcentaje de velocidad que falta por recorrer luego de reducir la misma
        if(porcRecVel>0 && porcRecVel<=1){
            s_Pelota->posAntX=(ui->widget->width()-s_Pelota->radio+(s_Pelota->velX/100*s_Pelota->escMetro)*(1-porcRecVel))-0.5;
            s_Pelota->centro.setX(qFloor(s_Pelota->posAntX));
        }
        WallCommand(0);
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
        if(porcRecVel>0 && porcRecVel<=1){
        //COLOCAMOS LA NUEVA POSICION
        s_Pelota->posAntX=s_Pelota->radio+(s_Pelota->velX/100*s_Pelota->escMetro)*(1-porcRecVel);
        s_Pelota->centro.setX(qCeil(s_Pelota->posAntX));
        }
        WallCommand(1);
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
        if(porcRecVel && porcRecVel<=1){
        s_Pelota->posAntY=(-ui->widget->height())+s_Pelota->radio+(s_Pelota->velY/100*s_Pelota->escMetro)*(1-porcRecVel);
        s_Pelota->centro.setY(qCeil(s_Pelota->posAntY));
        }
        WallCommand(2);
    }

    //si choca el pizo
    if((s_Pelota->centro.y()+s_Pelota->radio)>=0){
        s_Pelota->velY=(-s_Pelota->velY);

        //CALCULAMOS LA PERDIDA DE VELOCIDAD
        distPared=(posAntY+s_Pelota->radio);
        porcRecVel=qAbs((distPared)/s_Pelota->velY);//se calcula el porcentaje de velocidad recorrido(DISTANCIA HACIA LA PARED RECORRIDA SOBRE LA VELOCIDAD)
        //ui->AnguloLineEdit->setText(QString("%1").arg(porcRecVel));

        if(porcRecVel>-1){
            s_Pelota->velY=s_Pelota->velY-((s_Pelota->alfha/100.0)*s_Pelota->velY)+(s_Pelota->gravedad/100*s_Pelota->escMetro)*(1-porcRecVel);//reducimos la velocidad y restamos tambien la gravedad recorrida calculada
        }else{
            s_Pelota->velY=0;
        }

        if(s_Pelota->velY>=0){//es decir si se pierde toda la energia
        s_Pelota->velY=0;
        }
        if(porcRecVel>0 && porcRecVel<=1){
        s_Pelota->posAntY=(-s_Pelota->radio)+(s_Pelota->velY/100*s_Pelota->escMetro)*(1-porcRecVel);
        s_Pelota->centro.setY(qCeil(s_Pelota->posAntY));
        }
       WallCommand(3);
    }


}

void MainWindow::RecalPelota2(s_pelota *s_Pelota){
    float posAntX;
    float posAntY;
    double tiempoalchoque;
    double distPared;

    s_Pelota->tx=s_Pelota->tx+0.01;
    s_Pelota->ty=s_Pelota->ty+0.01;

    posAntX=s_Pelota->posAntX;
    posAntY=s_Pelota->posAntY;


    //con coeficiente de friccion
    if((s_Pelota->centro.y()+s_Pelota->radio)>=0 && ui->CheckFriccion->isChecked()){//si la pelota no toca el piso tiene aceleracion de la gravedad
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
       s_Pelota->velIX=s_Pelota->velX;
       s_Pelota->posIX=s_Pelota->posAntX;
       s_Pelota->tx=0;
    }
    //cambiamos la posicion
    if(s_Pelota->posAntY<0){
    s_Pelota->velY=s_Pelota->velIY+gravedad*s_Pelota->ty;
    }
    s_Pelota->posAntY=s_Pelota->posIY+(s_Pelota->velIY*s_Pelota->escMetro*s_Pelota->ty)+((gravedad*s_Pelota->escMetro*(s_Pelota->ty*s_Pelota->ty))/2);
    s_Pelota->posAntX=s_Pelota->posIX+(s_Pelota->velIX*s_Pelota->escMetro*s_Pelota->tx);

    //si choca la pared derecha
    if((s_Pelota->posAntX+s_Pelota->radio)>=ui->widget->width() && s_Pelota->velX>0){

       distPared=ui->widget->width()-posAntX+s_Pelota->radio;
       tiempoalchoque=(distPared/s_Pelota->velX)*0.01;

       s_Pelota->velX=(-s_Pelota->velX);

       //se reduce la velocidad
       s_Pelota->velX=s_Pelota->velX-s_Pelota->velX*(s_Pelota->alfha/100.0);

       s_Pelota->posAntX=ui->widget->width()+(s_Pelota->velX*(0.01-tiempoalchoque))-s_Pelota->radio;

       s_Pelota->posIX=s_Pelota->posAntX;
       s_Pelota->velIX=s_Pelota->velX;
       s_Pelota->tx=0;
       WallCommand(0);
    }

    //si choca la pared izquierda
    if(((s_Pelota->posAntX)-s_Pelota->radio)<=0 && s_Pelota->velX<0){

       distPared=posAntX-s_Pelota->radio;
       tiempoalchoque=(distPared/s_Pelota->velX)*0.01;

       s_Pelota->velX=(-s_Pelota->velX);

       //se reduce la velocidad
       s_Pelota->velX=s_Pelota->velX-s_Pelota->velX*(s_Pelota->alfha/100.0);

       s_Pelota->posAntX=(s_Pelota->velX*(0.01-tiempoalchoque))+s_Pelota->radio;

       s_Pelota->posIX=s_Pelota->posAntX;
        s_Pelota->velIX=s_Pelota->velX;
        s_Pelota->tx=0;
        WallCommand(1);
    }

    //si choca el techo
    if((s_Pelota->posAntY-s_Pelota->radio)<=(-ui->widget->height()) && s_Pelota->velY<0){


        distPared=ui->widget->height()+posAntY-s_Pelota->radio;
        tiempoalchoque=-((distPared/s_Pelota->velY)*0.01);

        s_Pelota->velY=(-s_Pelota->velY);

        //se reduce la velocidad
        s_Pelota->velY=s_Pelota->velY-s_Pelota->velY*((s_Pelota->alfha/100.0));

        s_Pelota->posAntY=s_Pelota->radio+(-ui->widget->height())+(s_Pelota->velY*(0.01-tiempoalchoque));

        s_Pelota->posIY=s_Pelota->posAntY;
        s_Pelota->velIY=s_Pelota->velY;
        s_Pelota->ty=0;
        WallCommand(2);
    }

    //si choca el pizo
    if((s_Pelota->posAntY+s_Pelota->radio)>=0 && s_Pelota->velY>0){

       distPared=-(posAntY+s_Pelota->radio);
       tiempoalchoque=((distPared/s_Pelota->velY)*0.01);

       s_Pelota->velY=(-s_Pelota->velY);

       s_Pelota->ty=0.01-tiempoalchoque;

       //se reduce la velocidad
       s_Pelota->velY=s_Pelota->velY-s_Pelota->velY*(s_Pelota->alfha/100.0);
       if(tiempoalchoque<=0){
        s_Pelota->velY=0;
        gravedad=0;
       }

       s_Pelota->velIY=s_Pelota->velY;
       s_Pelota->posAntY=-s_Pelota->radio+(s_Pelota->velIY*s_Pelota->escMetro*s_Pelota->ty)+((gravedad*s_Pelota->escMetro*(s_Pelota->ty*s_Pelota->ty))/2);
       s_Pelota->ty=0;
       s_Pelota->posIY=s_Pelota->posAntY;

       WallCommand(3);
    }
    s_Pelota->centro.setX(qCeil(s_Pelota->posAntX+0.5));
    s_Pelota->centro.setY(qCeil(s_Pelota->posAntY-0.5));

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
    QString str;
    int y;

    str=ui->PosYLineEdit->text();

    y=str.toInt();
    y=-y;
    if(y<(0-s_pelotita.radio) && y>(-ui->widget->height()+s_pelotita.radio)){
        s_pelotita.centroI.setY(y);
    }
    BorrarPantalla();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    QPaintBall->resize(ui->widget->width(),ui->widget->height());

}

void MainWindow::on_BotonPosX_clicked()
{
    QString str;
    int x;

    str=ui->PosYLineEdit->text();

    x=str.toInt();
    if(x>(s_pelotita.radio) && x<(ui->widget->width()-s_pelotita.radio)){
        s_pelotita.centroI.setX(x);
    }
    BorrarPantalla();
}

void MainWindow::OnRxQSerialPort1(){
    uint8_t cont;
    uint8_t buffer[256];
    int i;

    cont=(QSerialPort1->bytesAvailable());
    if(cont>0){
        QSerialPort1->read((char*)buffer,cont);

        for(i=0;i<cont;i++){
        datosLec.bufL[datosLec.iRE]=buffer[i];
            datosLec.iRE++;
            if(datosLec.iRE>=datosLec.tamBuffer){
            datosLec.iRE=0;

            }
        }
    }
}

void MainWindow::on_OpenCloseButton_clicked()
{
    if(QSerialPort1->isOpen()){
        QSerialPort1->close();

        ui->OpenCloseButton->setText("OPEN");
    }else{
        QSerialPort1->setPortName(ui->PortLinEdit->text());
        QSerialPort1->setBaudRate(QSerialPort::Baud115200);

        if(QSerialPort1->open(QSerialPort::ReadWrite)){
        ui->OpenCloseButton->setText("CLOSE");
        }
    }
}

void MainWindow::on_SendButton_clicked()
{   uint8_t str[256];

    const char *comandos[4];
    const char ALIVE[]="ALIVE";
    const char FIRMWARE[]="FIRMWARE";
    const char DESCONOCIDO[]="DESCONOCIDO";
    comandos[0]=ALIVE;
    comandos[1]=FIRMWARE;
    comandos[2]=DESCONOCIDO;
    int comand=0;
    int i;


    for(i=0;i<4;i++){
        if(QString::compare(QString(comandos[i]),QString(ui->CommandlineEdit->text()),Qt::CaseSensitive)==0){
            comand=i;
            switch(comand){
            case 0: //ui->commandsConsole->appendPlainText("ALIVE");
                    ColocarHeader(&datosEsc,ALIVEID,2);
                    ColocarPayload(&datosEsc,str,0);

                break;
            case 1: //ui->commandsConsole->appendPlainText("FIRMWARE");
                    ColocarHeader(&datosEsc,FIRMWAREID,2 );
                    ColocarPayload(&datosEsc,str,0);
                break;
            case 2:ui->commandsConsole->appendPlainText("DESCONOCIDO");
                    ColocarHeader(&datosEsc,0x07,2);
                    ColocarPayload(&datosEsc,str,0);
                break;
            default:

            break;

            }
      }
    }

    if(QSerialPort1->isOpen()){
      i=0;
      while(datosEsc.iTE!=datosEsc.iTL){
            //PASAMOS LOS DATOS A UN PEQUEÑO BUFFER INTERMEDIO PARA ENVIARLOS
            str[i]=datosEsc.bufE[datosEsc.iTL];
            datosEsc.iTL++;
            if(datosEsc.iTL>=datosEsc.tamBuffer){
            datosEsc.iTL=0;
            }
            i++;
      }
      QSerialPort1->write((char *)str,i+1);//enviamos la cantidad de bytes disponibles para enviar
    }
}

void MainWindow::on_ClearConsoleButton_clicked()
{
    ui->commandsConsole->clear();
}

void MainWindow::DecodeCMD()
{
    uint8_t indiceE;
    indiceE=datosLec.iRE;//guardamos la posicion del buffer de escritura hasta donde esta actualmente
    while(datosLec.iRL!=indiceE){
        switch (datosLec.estDecode){
        case EST_U:
        if(datosLec.bufL[datosLec.iRL]=='U'){
                    datosLec.estDecode=EST_N;
            printf("%x",datosLec.bufL[datosLec.iRL]);
        }

        break;
        case EST_N:if(datosLec.bufL[datosLec.iRL]=='N'){
                    datosLec.estDecode=EST_E;

                    printf("%x",datosLec.bufL[datosLec.iRL]);
        }else{
                    if(datosLec.iRL>0){
                        datosLec.iRL--;
                    }else{
                        datosLec.iRL=datosLec.tamBuffer-1;
                    }
                    datosLec.estDecode=EST_U;
        }
        break;
        case EST_E:if(datosLec.bufL[datosLec.iRL]=='E'){
                    datosLec.estDecode=EST_R;
                   printf("%x",datosLec.bufL[datosLec.iRL]);
                     }else{
                    if(datosLec.iRL>0){
                        datosLec.iRL--;
                    }else{
                        datosLec.iRL=datosLec.tamBuffer-1;
                    }
                    datosLec.estDecode=EST_U;
                     }
        break;
        case EST_R:if(datosLec.bufL[datosLec.iRL]=='R'){
                    datosLec.estDecode=NUMBYTES;
                   printf("%x",datosLec.bufL[datosLec.iRL]);
        }else{
                    if(datosLec.iRL>0){
                        datosLec.iRL--;
                    }else{
                        datosLec.iRL=datosLec.tamBuffer-1;
                    }
                    datosLec.estDecode=EST_U;
        }
        break;
        case NUMBYTES: datosLec.nBytes=datosLec.bufL[datosLec.iRL];
                        datosLec.estDecode=TOKEN;
                    printf("%x",datosLec.bufL[datosLec.iRL]);
        break;
        case TOKEN:if(datosLec.bufL[datosLec.iRL]==':'){
                    datosLec.estDecode=PAYLOAD;
                    printf("%x",datosLec.bufL[datosLec.iRL]);
        }else{
                    if(datosLec.iRL>0){
                        datosLec.iRL--;
                    }else{
                        datosLec.iRL=datosLec.tamBuffer-1;
                    }
                    datosLec.estDecode=EST_U;
        }
        break;
        case PAYLOAD:
                    printf("%x",datosLec.bufL[datosLec.iRL]);
        datosLec.idCMD=datosLec.bufL[datosLec.iRL];//guardo la id del comando
        datosLec.iDatos=datosLec.iRL;//guardo la posicion de los datos
        datosLec.checksum='U'^'N'^'E'^'R'^datosLec.nBytes^':'^datosLec.idCMD;//inicializamos el checksum
        datosLec.iChecksum=datosLec.iDatos+datosLec.nBytes-1;//guardo la posicion del checksum esperada
        datosLec.estDecode=CHECKSUM;
        break;
        case CHECKSUM:
                    if(datosLec.iRL!=datosLec.iChecksum){
                    datosLec.checksum=datosLec.checksum^datosLec.bufL[datosLec.iRL];
                    }else{
                    printf("%x",datosLec.bufL[datosLec.iRL]);
                    printf("%x-%x",datosLec.bufL[datosLec.iRL],datosLec.checksum);
                    if(datosLec.bufL[datosLec.iRL]==datosLec.checksum){
                        ISNEWCMD=1;

                    }else{
                        if(datosLec.iRL>0){//VOLVEMOS A VERIFICAR SI ES UNA U
                            datosLec.iRL--;
                        }else{
                            datosLec.iRL=datosLec.tamBuffer-1;
                        }
                    }
                    datosLec.estDecode=EST_U;
                    }

        break;
        default:datosLec.estDecode=EST_U;
        break;
        }

        datosLec.iRL++;
        if(datosLec.iRL>=datosLec.tamBuffer){
        datosLec.iRL=0;
        }
    }
}

void MainWindow::ExecuteCMD(s_LDatos *datosCMD)
{
    switch (datosCMD->idCMD){
    case ACKID:Ack(datosCMD);
        break;
    case FIRMWAREID:Firmware(datosCMD);
        break;
    case BUTTONSID:ui->commandsConsole->appendPlainText("botones");
        break;
    default:ComandoDesconocido(datosCMD);
        break;
    }
}

void MainWindow::ColocarHeader(s_EDatos *datosE, uint8_t ID, uint8_t nBytes)
{
    datosE->bufE[datosE->iTE]='U';
    datosE->iTE++;
    if(datosE->iTE>=datosE->tamBuffer){
        datosE->iTE=0;
    }
    datosE->bufE[datosE->iTE]='N';
    datosE->iTE++;
    if(datosE->iTE>=datosE->tamBuffer){
        datosE->iTE=0;
    }
    datosE->bufE[datosE->iTE]='E';
    datosE->iTE++;
    if(datosE->iTE>=datosE->tamBuffer){
        datosE->iTE=0;
    }
    datosE->bufE[datosE->iTE]='R';
    datosE->iTE++;
    if(datosE->iTE>=datosE->tamBuffer){
        datosE->iTE=0;
    }
    datosE->bufE[datosE->iTE]=nBytes;
    datosE->iTE++;
    if(datosE->iTE>=datosE->tamBuffer){
        datosE->iTE=0;
    }
    datosE->bufE[datosE->iTE]=':';
    datosE->iTE++;
    if(datosE->iTE>=datosE->tamBuffer){
        datosE->iTE=0;
    }
    datosE->bufE[datosE->iTE]=ID;
    datosE->iTE++;
    if(datosE->iTE>=datosE->tamBuffer){
        datosE->iTE=0;
    }
    datosE->checksum='U'^'N'^'E'^'R'^nBytes^':'^ID;//inicializamos el checksum
}

void MainWindow::ColocarPayload(s_EDatos *datosE, uint8_t *string, uint8_t nDatos)
{
    uint8_t i;
    for(i=0;i<nDatos;i++){
        datosE->checksum=datosE->checksum^string[i];
        datosE->bufE[datosE->iTE]=string[i];
        datosE->iTE++;
        if(datosE->iTE>=datosE->tamBuffer){
        datosE->iTE=0;
        }
    }
    datosE->bufE[datosE->iTE]=datosE->checksum;
    datosE->iTE++;
    if(datosE->iTE>=datosE->tamBuffer){
        datosE->iTE=0;
    }
}

//COMANDOS DE RECEPCION
void MainWindow::Ack(s_LDatos *datosCMD)
{
    if(datosCMD->bufL[datosCMD->iDatos+1]==0x0D){
        ui->commandsConsole->appendPlainText("ESTOY VIVO");
    }
}

void MainWindow::Firmware(s_LDatos *datosCMD)
{  QString str;
    str="La version es:";
    str=str+QString(QChar(datosCMD->bufL[datosCMD->iDatos+1]));
    ui->commandsConsole->appendPlainText(str);

}

void MainWindow::ComandoDesconocido(s_LDatos *datosCMD)
{   QString str;

    if(datosCMD->bufL[datosCMD->iDatos+1]==0xFF){
        str=QString("El comando con la siguiente ID no existe:%1").arg(datosCMD->idCMD,0,16);
        ui->commandsConsole->appendPlainText(str);
    }
}


//COMANDOS DE ENVIO
void MainWindow::LedsCommand(uint8_t LedstoAct, uint8_t LedsValue)
{   uint8_t str[256];
    int i;
    str[0]=LedstoAct;
    str[1]=LedsValue;
    ColocarHeader(&datosEsc,LEDSID,4);
    ColocarPayload(&datosEsc,str,2);

    if(QSerialPort1->isOpen()){
        i=0;
        while(datosEsc.iTE!=datosEsc.iTL){
        //PASAMOS LOS DATOS A UN PEQUEÑO BUFFER INTERMEDIO PARA ENVIARLOS
        str[i]=datosEsc.bufE[datosEsc.iTL];
        datosEsc.iTL++;
        if(datosEsc.iTL>=datosEsc.tamBuffer){
                    datosEsc.iTL=0;
        }
        i++;
        }
        QSerialPort1->write((char *)str,i+1);//enviamos la cantidad de bytes disponibles para enviar
    }
}

void MainWindow::WallCommand(uint8_t wall)
{
    uint8_t str[256];
    int i;
    str[0]=wall;
    ColocarHeader(&datosEsc,WALLID,3);
    ColocarPayload(&datosEsc,str,1);

    if(QSerialPort1->isOpen()){
        i=0;
        while(datosEsc.iTE!=datosEsc.iTL){
        //PASAMOS LOS DATOS A UN PEQUEÑO BUFFER INTERMEDIO PARA ENVIARLOS
        str[i]=datosEsc.bufE[datosEsc.iTL];
        datosEsc.iTL++;
        if(datosEsc.iTL>=datosEsc.tamBuffer){
                    datosEsc.iTL=0;
        }
        i++;
        }
        QSerialPort1->write((char *)str,i+1);//enviamos la cantidad de bytes disponibles para enviar
    }
}

void MainWindow::ButtonsState()
{
    uint8_t str[256];
    int i;
    ColocarHeader(&datosEsc,BUTTONSID,2);
    ColocarPayload(&datosEsc,str,0);

    if(QSerialPort1->isOpen()){
        i=0;
        while(datosEsc.iTE!=datosEsc.iTL){
        //PASAMOS LOS DATOS A UN PEQUEÑO BUFFER INTERMEDIO PARA ENVIARLOS
        str[i]=datosEsc.bufE[datosEsc.iTL];
        datosEsc.iTL++;
        if(datosEsc.iTL>=datosEsc.tamBuffer){
                    datosEsc.iTL=0;
        }
        i++;
        }
        QSerialPort1->write((char *)str,i+1);//enviamos la cantidad de bytes disponibles para enviar
    }
}

void MainWindow::on_Checkgravedad_clicked()
{
    //con gravedad
    if(ui->Checkgravedad->isChecked()){
        gravedad=s_pelotita.gravedad;
    }else{
        gravedad=0;
    }
    s_pelotita.ty=0;
    s_pelotita.posIY=s_pelotita.posAntY;
    s_pelotita.velIY=s_pelotita.velY;
}

