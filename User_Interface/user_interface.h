#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include <QMainWindow>
#include <QWidget>
#include <QMessageBox>
#include <QtSerialPort/qserialport.h>
#include <QDialog>
#include <QDebug>
#include <QFileInfo>
#include <QtSql>
#include <QStandardPaths>
#include "qmqtt.h"

QT_BEGIN_NAMESPACE
namespace Ui { class User_Interface; }
QT_END_NAMESPACE

class User_Interface : public QMainWindow
{
    Q_OBJECT

public:
    QSqlDatabase myDB;
    void connClose(){
        myDB.close();
        myDB.removeDatabase("inventario.db");
    }
    bool connOpen(){
        myDB = QSqlDatabase::addDatabase("QSQLITE");
        myDB.setDatabaseName("inventario.db");
        if(myDB.open()){
                qDebug() << "No Connection to DataBase";
                return false;
        }else{
            qDebug() << "DataBase Connected";
            return true;
        }
    }

public:
    explicit User_Interface(QMainWindow *parent = nullptr);
    ~User_Interface();

private slots:

    void onMQTT_Received(const QMQTT::Message &message);
    void onMQTT_Connected(void);
    void onMQTT_subscribed(const QString &topic);

    void on_Activa_Conex_clicked();
    void on_Conf_Temp_Man_valueChanged(int arg1);
    void on_Activa_Man_Enc_clicked();
    void on_Activa_Man_Apa_clicked();
    void on_Activa_Prog_Enc_clicked();
    void on_Activa_Prog_Apa_clicked();
    void on_Cancela_Programado_clicked();

private:
    void startClient();
    void processError(const QString &s);
    void activateRunButton();
    void SendMessage();

private:
    Ui::User_Interface *ui;
    int transactionCount;
    QMessageBox ventanaPopUp;
    QMQTT::Client *_client;
    bool connected;
    int marca_aire = 0;
};

#endif // USER_INTERFACE_H
