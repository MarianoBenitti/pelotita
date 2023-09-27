#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qpaintbox.h>
#include <QPainter>
#include <QTimer>
#include <QtSerialPort/QSerialPort>



//declaramos la estructura de la pelotita
typedef struct {
    QPoint centro;//centro de la pelotita actual
    QPoint centroI;//centro de la pelotita inicial
    double posAntX;
    double posAntY;
    double posIX;
    double posIY;
    double anguloI;//angulo inicial
    double velI;//velocidad inicial
    double velIY;
    double velIX;
    double velX;//velocidad en X
    double velY;//velocidad en Y
    double gravedad;
    double friccion;
    double tx;
    double ty;
    int radio;//radio de la pelotita
    int alfha;//porcentaje de perdida de energia
    double escMetro;//a cuanto equivale 1 metro en pixeles
    uint8_t InMove;//vale 1 si esta en movimiento o 0 si esta detenida

}s_pelota;

typedef enum{EST_U=0,EST_N,EST_E,EST_R,NUMBYTES,TOKEN,PAYLOAD,CHECKSUM}e_estCMD;
//____________________________________________________________________________
typedef union{
    struct {
        uint8_t b0:1;//se usa para saber si hay bytes leidos
        uint8_t b1:1;
        uint8_t b2:1;
        uint8_t b3:1;
        uint8_t b4:1;
        uint8_t b5:1;
        uint8_t b6:1;
        uint8_t b7:1;
    }bit;
    uint8_t byte;
}band;
//______________________________________________________________________________
typedef struct __attribute__((packed,aligned(1))){
    uint8_t iRL;//indice de lectura
    uint8_t iRE;//indice de escritura
    e_estCMD estDecode;//estado del decodificado del comando
    uint8_t nBytes;//numero de bytes de datos
    uint8_t iDatos;//inicio de los datos en el buffer
    uint8_t idCMD;//id del comando leido
    uint8_t *bufL;//puntero al buffer con los datos leidos
    uint8_t checksum;//checksum del comando
    uint8_t iChecksum;//indice del checksum
    uint8_t tamBuffer;//tamaño que limita el buffer debe ser de 2 a la n para que funcione correctamente la compuerta and de reinicio
}s_LDatos;//estructura de lectura

typedef struct __attribute__((packed,aligned(1))){
    uint8_t iTL;//indice de escritura en el buffer
    uint8_t iTE;//indice de lectura para transmitir los datos
    uint8_t checksum;//checksum del comando
    uint8_t *bufE;//puntero al buffer de escritura
    uint8_t tamBuffer;//tamaño del buffer de escritura, debe ser de 2 a la n
}s_EDatos;//estructura de escritura

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
    void RecalPelota2(s_pelota *s_Pelota);//recalcula la posicion segun el tiempo
    void BorrarPantalla();
private slots:
    void on_BotonLanzar_clicked();

    void on_BotonAngulo_clicked();

    void on_BotonVelocidad_clicked();

    void on_BotonPosY_clicked();

    void resizeEvent(QResizeEvent *event);

    void on_BotonPosX_clicked();

    void OnRxQSerialPort1();

    void on_OpenCloseButton_clicked();

    void on_SendButton_clicked();

     /** \fn DecodeCMD
     * \brief
     *  Esta funcion revisa si se ingresa un comando, detecta el header y una vez detectado
     *  prepara la estructura que se utilizara para analizarlo posteriormente en otra funcion
     * */
        void DecodeCMD();

     /** \fn ExecuteCMD
     * \brief
     *  Esta funcion ejecuta el comando con los datos ingresados en la estructura
     *  una vez determinado el comando por su id procede a ejecutarlo
     * \param[in]: datosCMD es la estructura donde estan todos los datos leidos
     * */
        void ExecuteCMD(s_LDatos *datosCMD);

     /** \fn ColocarHeader
     * \brief
     *  Esta funcion carga en el buffer de escritura el proximo comando a enviar a la pc
     *
     * \param[in]: datosE es la estructura del buffer de escritura
     * \param[in]: ID es el id del comando que se preparara para enviar
     * \param[in]: nbytes es el numero de bytes de datos del comando es decir el length
     * */
            void ColocarHeader(s_EDatos *datosE,uint8_t ID,uint8_t nBytes);

     /** \fn ColocarPayload
     * \brief
     *  Esta funcion carga en el buffer de escritura el proximo comando a enviar a la pc
     *
     * \param[in]: datosE es la estructura del buffer de escritura
     * \param[in]: ID es el id del comando que se preparara para enviar
     * \param[in]: string es la cadena de datos sin el id
     * \param[in]: nDatos es la cantidad de datos de la cadena
     * */
            void ColocarPayload(s_EDatos *datosE,uint8_t *string,uint8_t nDatos);

    //COMANDOS
            //COMANDOS DE RECEPCION
            /** \fn Ack
             * \brief
             *  Esta funcion muestra el resultado que llego de la placa y lo muestra en el plainTextEdit
             * \param[in]: datosCMD es la estructura del buffer donde se cargaran los comandos
             * */
            void Ack(s_LDatos *datosCMD);
            /** \fn Firmware
             * \brief
             *  Esta funcion muestra el firmware recibido desde la placa
             * \param[in]: datosCMD es la estructura del buffer donde se cargaran los comandos
             * */
            void Firmware(s_LDatos *datosCMD);
            /** \fn Ack
             * \brief
             *  Esta funcion verifica el comando desconocido que le llego a la placa e imprime la id del comando recibido en el plaintextedit
             * \param[in]: datosCMD es la estructura del buffer donde se decodifico el comando
             * */
            void ComandoDesconocido(s_LDatos *datosCMD);
            /** \fn LedsCommand
             * \brief
             *  Esta funcion verifica el comando desconocido que le llego a la placa e imprime la id del comando recibido en el plaintextedit
             * \param[in]: LedstoAct los cuatro bytes menos significativos de esta variable indican que leds se actualizaran
             *  \param[in]: LedstoAct los cuatro bytes menos significativos de esta variable indican el valor que se le colocara a los leds a actualizar
             * */

            //COMANDOS DE ENVIO
            void LedsCommand(uint8_t LedstoAct,uint8_t LedsValue);
            /** \fn WallCommand
             * \brief
             *  Esta funcion envia a la placa el muro en el cual la pelota choco 0(Derecha),1(Izquierda),2(Arriba) y 3(Abajo)
             * \param[in]: wall es el numero correspondiente al muro chocado
             * */
            void WallCommand(uint8_t wall);
            /** \fn ButtonsState
             * \brief
             *  Esta funcion envia a la placa una peticion del estado de los pulsadores
             * */
            void ButtonsState();

            void on_ClearConsoleButton_clicked();

            void on_Checkgravedad_clicked();

        private:
    Ui::MainWindow *ui;
    s_pelota s_pelotita;
    QPaintBox *QPaintBall;//pantalla donde se pintara
    QTimer *TimerGen;//timer general
    QSerialPort *QSerialPort1;

    s_EDatos datosEsc;
    uint8_t bufferE[256];
    s_LDatos datosLec;
    uint8_t bufferL[256];
    band banderas;
    double gravedad;
};
#endif // MAINWINDOW_H
