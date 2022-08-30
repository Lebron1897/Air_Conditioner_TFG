#include "user_interface.h"
#include "ui_user_interface.h"
#include "remotelink_messages.h"

#include <QSerialPort>      // Comunicacion por el puerto serie
#include <QSerialPortInfo>  // Comunicacion por el puerto serie
#include <QMessageBox>      // Se deben incluir cabeceras a los componentes que se vayan a crear en la clase
#include <QJsonObject>
#include <QJsonDocument>
#include <QDate>

#include<stdint.h>      // Cabecera para usar tipos de enteros con tamaño
#include<stdbool.h>     // Cabecera para usar booleanos

User_Interface::User_Interface(QMainWindow *parent)
    : QMainWindow(parent)
    , ui(new Ui::User_Interface)
{
    ui->setupUi(this);

    ui->Estado_Aire->setChecked(false);
    ui->Led_Prog_Enc->setChecked(false);
    ui->Led_Prog_Apa->setChecked(false);

    ui->Activa_Man_Enc->setEnabled(false);
    ui->Activa_Man_Apa->setEnabled(false);
    ui->Activa_Prog_Enc->setEnabled(false);
    ui->Activa_Prog_Apa->setEnabled(false);
    ui->Cancela_Programado->setEnabled(false);

    QDate actual = actual.currentDate();
    ui->Fecha_Prog_Enc->setDate(actual);
    ui->Fecha_Prog_Apa->setDate(actual);

    setWindowTitle(tr("Interfaz de Control")); // Título de la ventana

    //Inicializa la ventana de PopUp que se muestra cuando llega la respuesta al PING
    ventanaPopUp.setIcon(QMessageBox::Information);
    ventanaPopUp.setText(tr("Te has dejado la ventana abierta.\n Ve a cerrarla o se apagara el aire")); //Este es el texto que muestra la ventana
    ventanaPopUp.setStandardButtons(QMessageBox::Ok);
    ventanaPopUp.setWindowTitle(tr("Evento"));
    ventanaPopUp.setParent(this,Qt::Popup);

    _client = new QMQTT::Client(QHostAddress::LocalHost, 1883);

    connect(_client, SIGNAL(connected()), this, SLOT(onMQTT_Connected()));
    connect(_client, SIGNAL(received(const QMQTT::Message &)), this, SLOT(onMQTT_Received(const QMQTT::Message &)));
    connect(_client, SIGNAL(subscribed(const QString &)), this, SLOT(onMQTT_subscribed(const QString &)));

    connOpen();

    connected=false;
}

User_Interface::~User_Interface()
{
    connClose();
    delete ui;
}

void User_Interface::startClient()
{
    _client->setHostName(ui->Text_Broker->text());
    _client->setPort(1883);
    _client->setKeepAlive(300);
    _client->setCleanSession(true);
    _client->connectToHost();

}

void User_Interface::processError(const QString &s)
{
    //activateRunButton(); // Activa el botón RUN
    // Muestra en la etiqueta de estado la razón del error (notese la forma de pasar argumentos a la cadena de texto)
    ui->statusLabel->setText(tr("Status: Not running, %1.").arg(s));
}

MESSAGE_ENCENDIDO_MANUAL_PARAMETROS mes_enc_man;
MESSAGE_APAGADO_MANUAL_PARAMETROS mes_apa_man;
MESSAGE_CAMBIO_TEMPERATURA_PARAMETROS mes_cam_temp;
MESSAGE_ENCENDIDO_PROGRAMADO_PARAMETROS mes_enc_prog;
MESSAGE_APAGADO_PROGRAMADO_PARAMETROS mes_apa_prog;

void User_Interface::SendMessage()
{
    //Modificar para mandar los mensajes con todas las variables
    //Se sigue mandando un json, pero con otros valores y que se modificara dependiendo de lo que se envie en cada caso

    QJsonObject objeto_json;
    objeto_json["Marca_Aire"] = marca_aire;

    if(mes_enc_man.val_ejec == 1 && mes_cam_temp.val_ejec == 1){
        objeto_json["Encendido"] = mes_enc_man.val_ejec;
        objeto_json["Cambio_Temperatura"] = mes_cam_temp.value_temp;
    }
    else if(mes_apa_man.val_ejec == 1){
        objeto_json["Apagado"] = mes_apa_man.val_ejec;
        objeto_json["Cambio_Temperatura"] = 0;
    }

    if(mes_enc_prog.val_ejec == 1){
        objeto_json["Encendido_Programado_Dia"] = mes_enc_prog.val_fecha_dia;
        objeto_json["Encendido_Programado_Mes"] = mes_enc_prog.val_fecha_mes;
        objeto_json["Encendido_Programado_Ano"] = mes_enc_prog.val_fecha_ano;
        objeto_json["Encendido_Programado_Hora"] = mes_enc_prog.val_reloj_hora;
        objeto_json["Encendido_Programado_Min"] = mes_enc_prog.val_reloj_min;
        objeto_json["Encendido_Programado_Temp"] = mes_enc_prog.val_temp;
        objeto_json["Encendido_Programado_Ejec"] = mes_enc_prog.val_ejec;
    }

    if(mes_apa_prog.val_ejec == 1){
        objeto_json["Apagado_Programado_Dia"] = mes_apa_prog.val_fecha_dia;
        objeto_json["Apagado_Programado_Mes"] = mes_apa_prog.val_fecha_mes;
        objeto_json["Apagado_Programado_Ano"] = mes_apa_prog.val_fecha_ano;
        objeto_json["Apagado_Programado_Hora"] = mes_apa_prog.val_reloj_hora;
        objeto_json["Apagado_Programado_Min"] = mes_apa_prog.val_reloj_min;
        objeto_json["Apagado_Programado_Ejec"] = mes_apa_prog.val_ejec;
    }

    QJsonDocument mensaje(objeto_json); //crea un objeto de tivo QJsonDocument conteniendo el objeto objeto_json (necesario para obtener el mensaje formateado en JSON)
    QMQTT::Message msg(0, ui->Topic_Conex->text(), mensaje.toJson()); //Crea el mensaje MQTT contieniendo el mensaje en formato JSON

    //Publica el mensaje
    _client->publish(msg);

}

void User_Interface::onMQTT_subscribed(const QString &topic)
{
     ui->statusLabel->setText(tr("subscribed %1").arg(topic));
}

void User_Interface::onMQTT_Connected()
{
    QString topic(ui->Topic_Conex->text());
    QString topic2("/TFG/Estados");

    ui->Activa_Conex->setEnabled(false);

    // Se indica que se ha realizado la conexión en la etiqueta 'statusLabel'
    ui->statusLabel->setText(tr("Ejecucion, conectado al servidor"));

    connected=true;

    _client->subscribe(topic,0);
    _client->subscribe(topic2,0);
}

void User_Interface::onMQTT_Received(const QMQTT::Message &message)
{
    bool previousblockinstate;
    double checked_ventana, checked_estado;
    if (connected)
    {
        QJsonParseError error;
        QJsonDocument mensaje=QJsonDocument::fromJson(message.payload(),&error);

        if ((error.error==QJsonParseError::NoError)&&(mensaje.isObject()))
        { //Tengo que comprobar que el mensaje es del tipo adecuado y no hay errores de parseo...

            QJsonObject objeto_json = mensaje.object();
            QJsonValue entrada_ventana = objeto_json["EstadoVentana"]; //Devuelve el estado en el que se encuentra la ventana
            QJsonValue estado_aire = objeto_json["EstadoAire"]; //Devuelve el estado en el que esta el aire,
                                                               //teniendo en cuenta si esta encendido/apagado y
                                                               //la temperatura a la que esta en ese momento

            mes_enc_man.est_ventana = entrada_ventana.toDouble();

            if (estado_aire.isDouble())
            {   //Compruebo que es booleano...

                checked_estado=estado_aire.toDouble(); //Leo el valor de objeto (si fuese entero usaria toInt(), toDouble() si es doble....
                previousblockinstate=ui->Estado_Aire->blockSignals(true);

                if (checked_estado == 1)
                {
                    ui->Estado_Aire->setEnabled(true);
                    if(mes_enc_man.est_ventana  == 1){
                        ui->Estado_Aire->setEnabled(false);
                        ventanaPopUp.setStyleSheet("background-color: lightgrey");
                        ventanaPopUp.setModal(true); //CAMBIO: Se sustituye la llamada a exec(...) por estas dos.
                        ventanaPopUp.show();
                        mes_enc_man.val_ejec = 0;
                        mes_apa_man.val_ejec = 1;
                        mes_cam_temp.val_ejec = 0;
                        SendMessage();

                    }else{
                        ui->Estado_Aire->setEnabled(true);
                    }
                }else{
                    ui->Estado_Aire->setEnabled(false);
                }
                ui->Estado_Aire->blockSignals(previousblockinstate);
            }
        }
    }
}

void User_Interface::on_Activa_Conex_clicked()
{
    //Realizar la conexion con el servidor web del broker
    startClient();
    //Realizar la lectura del broker
    connect(_client, SIGNAL(received(const QMQTT::Message &)), this, SLOT(onMQTT_Received(const QMQTT::Message &)));
    //Colocar los leds en los estados que les corresponda
    ui->List_Disp->setEnabled(false);
    marca_aire = ui->List_Disp->currentIndex();

    ui->Activa_Man_Enc->setEnabled(true);
    ui->Activa_Man_Apa->setEnabled(true);
    ui->Activa_Prog_Enc->setEnabled(true);
    ui->Activa_Prog_Apa->setEnabled(true);
    ui->Cancela_Programado->setEnabled(true);

}

void User_Interface::on_Activa_Man_Enc_clicked()
{

    mes_enc_man.val_ejec = 1;
    mes_apa_man.val_ejec = 0;
    mes_cam_temp.val_ejec = 1;
    mes_cam_temp.value_temp = ui->Conf_Temp_Man->value();

    ui->Estado_Aire->setChecked(true);

    //Envio del mensaje al broker
    SendMessage();
}


void User_Interface::on_Activa_Man_Apa_clicked()
{
    mes_apa_man.val_ejec = 1;
    mes_enc_man.val_ejec = 0;
    mes_cam_temp.val_ejec = 0;

    ui->Estado_Aire->setChecked(false);

    //Envio del mensaje al broker
    SendMessage();
}


void User_Interface::on_Conf_Temp_Man_valueChanged(int arg1)
{
    if(mes_cam_temp.val_ejec == 1){
        mes_cam_temp.value_temp =arg1; //ui->Conf_Temp_Man->value();
        mes_enc_man.val_ejec = 1;

        //Envio del mensaje al broker
        SendMessage();
    }else{
        mes_cam_temp.value_temp = arg1; //ui->Conf_Temp_Man->value()
    }

}

void User_Interface::on_Activa_Prog_Enc_clicked()
{
    mes_enc_prog.val_fecha_ano = ui->Fecha_Prog_Enc->date().year();
    mes_enc_prog.val_fecha_mes = ui->Fecha_Prog_Enc->date().month();
    mes_enc_prog.val_fecha_dia = ui->Fecha_Prog_Enc->date().day();
    mes_enc_prog.val_reloj_hora = ui->Hora_Prog_Enc->time().hour();
    mes_enc_prog.val_reloj_min = ui->Hora_Prog_Enc->time().minute();
    mes_enc_prog.val_temp = ui->Temp_Prog_Enc->value();
    mes_enc_prog.val_ejec = 1;

    ui->Led_Prog_Enc->setChecked(true);

    //Envio del mensaje al broker
    SendMessage();
}


void User_Interface::on_Activa_Prog_Apa_clicked()
{
    mes_apa_prog.val_fecha_ano = ui->Fecha_Prog_Apa->date().year();
    mes_apa_prog.val_fecha_mes = ui->Fecha_Prog_Apa->date().month();
    mes_apa_prog.val_fecha_dia = ui->Fecha_Prog_Apa->date().day();
    mes_apa_prog.val_reloj_hora = ui->Hora_Prog_Apa->time().hour();
    mes_apa_prog.val_reloj_min = ui->Hora_Prog_Apa->time().minute();
    mes_apa_prog.val_ejec = 1;
    ui->Fecha_Prog_Apa->date().currentDate();
    ui->Led_Prog_Apa->setChecked(true);

    //Envio del mensaje al broker
    SendMessage();
}


void User_Interface::on_Cancela_Programado_clicked()
{
    mes_enc_prog.val_fecha_ano = 0;
    mes_enc_prog.val_fecha_mes = 0;
    mes_enc_prog.val_fecha_dia = 0;
    mes_enc_prog.val_reloj_hora = 0;
    mes_enc_prog.val_reloj_min = 0;
    mes_enc_prog.val_temp = 0;
    mes_enc_prog.val_ejec = 0;

    mes_apa_prog.val_fecha_ano = 0;
    mes_apa_prog.val_fecha_mes = 0;
    mes_apa_prog.val_fecha_dia = 0;;
    mes_apa_prog.val_reloj_hora = 0;
    mes_apa_prog.val_reloj_min = 0;
    mes_apa_prog.val_ejec = 0;

    ui->Led_Prog_Enc->setChecked(false);
    ui->Led_Prog_Apa->setChecked(false);

    //Envio del mensaje al broker
    SendMessage();
}

