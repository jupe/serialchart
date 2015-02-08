#include <QDebug>
#include "portrs232.h"

PortRs232::PortRs232(Configuration* config)
    : PortBase(config)
{
    isRunning = false;
    serialPort = 0;
}


void PortRs232::requestToStop(){
    isRunning = false;
}

PortRs232::~PortRs232(){

}

void PortRs232::portSetup(){
    QSerialPort::BaudRate baudRate = QSerialPort::Baud9600;
    switch(config->get("_setup_","baudrate").toInt()){
        case 1200: baudRate = QSerialPort::Baud1200; break;
        case 2400: baudRate = QSerialPort::Baud2400; break;
        case 9600: baudRate = QSerialPort::Baud9600; break;
        case 19200: baudRate = QSerialPort::Baud19200; break;
        case 38400: baudRate = QSerialPort::Baud38400; break;
        case 57600: baudRate = QSerialPort::Baud57600; break;
        case 115200: baudRate = QSerialPort::Baud115200; break;
    }
    QString portname = config->get("_setup_","port");
    serialPort->setPortName( portname );
    serialPort->setBaudRate(baudRate);
    serialPort->setFlowControl(  QSerialPort::NoFlowControl );
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setDataBits( QSerialPort::Data8 );
    serialPort->setStopBits( QSerialPort::OneStop );
}


void PortRs232::run(){
    if(isRunning) return;
    serialPort = new QSerialPort();
    portSetup();

    if(serialPort->open( QIODevice::ReadWrite )){;
        //Note: on Windows settings must be called after serialPort->open, otherwise we get garbage when device is plugged for the fist time
        // -> I believe QSerialPort library handle this right way already so comment this out
        //portSetup();
        isRunning = true;
        serialPort->flush();
        while(isRunning && serialPort && serialPort->isOpen()){
            if(serialPort->bytesAvailable() > 0) emit newData(serialPort->readAll());
            qApp->processEvents();
        }
    }else{
        emit message("Could not open port "+config->get("_setup_","port")+"\n"
                     "Error code: "+serialPort->errorString()+"\n"+
                     "Make sure the port is available and not used by other applications.","critical");
    }

    isRunning = false;
    if(serialPort->isOpen()) serialPort->close();
    delete serialPort;
    emit stopped();
}



void PortRs232::send(const QString & str){
    QByteArray data;
    QRegExp rx("^(char|short|long|hex)\\:(.*)$");
    if(rx.exactMatch(str)){
        QString format = rx.capturedTexts()[1];
        QStringList items = rx.capturedTexts()[2].split(QRegExp("\\s*,\\s*"));
        qDebug(format.toUtf8());
        for(int i=0;i<items.length();i++){
            qDebug(("["+items[i]+"]").toUtf8());
            if("char"==format){
                short num = items[i].toShort();
                data.append((char)num);
            }else if("short"==format){
                short num = items[i].toShort();
                data.append((char*)(&num),sizeof(num));
            }else if("long"==format){
                long num = items[i].toLong();
                data.append((char*)(&num),sizeof(num));
            }else if("hex"==format){
                data.append(QByteArray::fromHex(items[i].toUtf8()));
            }
        };
    }else{
        data.append(str);
    }
    qDebug(QString(data).toUtf8());
    if(isRunning) serialPort->write(data);

}
