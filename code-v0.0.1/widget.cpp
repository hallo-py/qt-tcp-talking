#include "widget.h"
#include "./ui_widget.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>

#include <QNetworkInterface>
#include <QHostAddress>

#include <QMessageBox>

#include <QObject>

#include <QDebug>

bool IsAdmin=false;
bool StartSocket=false;
QTcpServer *server;
QTcpSocket *socket;
QList<QTcpSocket*> people; // if is admin
QString ip,name;

QHostAddress gethostIP();

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    ip=gethostIP().toString();

    this->setWindowTitle("开房/连接");

    disablestart(false);
    QPushButton *bind = ui->bind;
    connect(bind,SIGNAL(clicked()),this,SLOT(startconnect()));
}

void Widget::startconnect(){
    QLineEdit *ip = ui->ip_inp;
    QLineEdit *port = ui->port_inp;
    QComboBox *method = ui->method;

    QString connectip=ip->text();
    quint16 connectport;
    try{
        connectport=port->text().toInt();
        if (!(9999>=connectport&&connectport>=1000)||connectport==0){
            QMessageBox::information(this,"port是4位数字!","port是4位数字!");
            return;
        }
    }catch (std::exception &e){
        QMessageBox::information(this,"port是4位数字!","port是4位数字!");
        return;
    }
    name=ui->name->text();
    int connectmethod=method->currentIndex();

    IsAdmin=connectmethod==1;

    if (connectmethod == 0){
        // 连接
        socket=new QTcpSocket;
        socket->connectToHost(connectip,connectport);
        if (socket->waitForConnected()){
            QMessageBox::information(this,"注意","连接成功");
            send(name);
            disablestart(true);
        } else {
            QMessageBox::information(this,"注意","连接失败");
            disablestart(false);
            return;
        }
    } else {
        // 开房
        server=new QTcpServer;
        bool res=server->listen((QHostAddress)connectip,connectport);
        if (res){
            QMessageBox::information(this,"注意","开房成功");
            disablestart(true);
        } else {
            QMessageBox::information(this,"注意","开房失败");
            disablestart(false);
            return;
        }
    }
    this->setWindowTitle("连接 - "+ip->text());
    // 绑定信号和槽
    if (IsAdmin){
        connect(server,&QTcpServer::newConnection,this,&Widget::connecting);
    } else {
        connect(socket,&QTcpSocket::readyRead,this,&Widget::readyRead);
    }
    connect(ui->send,&QPushButton::clicked,this,&Widget::readySend);
    connect(ui->disconnect,&QPushButton::clicked,this,&Widget::quit);
}

void Widget::connecting(){
    if (!IsAdmin) return;

    QTcpSocket* connection=server->nextPendingConnection();
    people.push_back(connection);

    QEventLoop *loop = new QEventLoop;
    // 连接信号到QEventLoop的退出
    connect(connection, &QTcpSocket::readyRead, loop, &QEventLoop::quit);
    // 开始等待
    loop->exec();

    QString usurname = connection->readAll();
    send(QString("欢迎 %1(%2)").arg(usurname).arg(connection->peerAddress().toString()));
    ui->information->appendPlainText(QString("欢迎 %1(%2)").arg(usurname).arg(connection->peerAddress().toString()));
    qDebug()<<connection->peerAddress().toString();

    while (connection->isValid()){
        // 要读取时退出
        loop->exec();
        qDebug()<<"reading!";
        // 读取
        QString text = connection->readAll();
        qDebug()<<text;
        send(text);
        ui->information->appendPlainText(text);
    }
}

void Widget::send(QString str){
    if (IsAdmin){
        for (int i=0;i<people.size();i++){
            people[i]->write(str.toUtf8().data());
        }
    } else {
        socket->write(str.toUtf8().data());
    }
}

void Widget::sendname(QString str,bool isClent=false){
    if (isClent){
        send("系统消息]"+name+QString("(%1)").arg(ip)+str);
    } else {
        send(name+QString("(%1)").arg(ip)+str);
    }
}

void Widget::readyRead(){
    if (IsAdmin) return;

    QString text = socket->readAll();
    ui->information->appendPlainText(text);
}

void Widget::readySend(){
    QString text=ui->text->text();
    ui->text->setText("");

    if (IsAdmin){
        sendname(text);
        ui->information->appendPlainText(name+QString("(%1)").arg(ip)+text);
    } else {
        sendname(text);
    }
}

void Widget::disablestart(bool set){
    // true->禁用 false->启用
    ui->start->setDisabled(set);
    ui->connect->setDisabled(!set);
}

QHostAddress gethostIP()
{
    auto list = QNetworkInterface::allInterfaces();
    foreach (QNetworkInterface interface, list) {
        // 1. 首先判断是不是以太网，过滤WiFi
        if (interface.type() != QNetworkInterface::Ethernet)
            continue;

        // 2. 如果有安装VMware虚拟机的话，会出现两个虚拟网卡
        // 匹配关键字"VMware"。来过滤虚拟网卡
        if (interface.humanReadableName().contains("VMware"))
            continue;

        // 3. 一般都会有两个ip地址，一个ipv4一个ipv6地址
        // 根据协议版本，来过滤掉ipv6地址
        foreach (auto entry ,interface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
                return entry.ip();
        }
    }

    return QHostAddress();
}

void Widget::quit(){
    this->~Widget();
}

Widget::~Widget()
{
    if (IsAdmin){
        send("服务器断开连接!");
    } else {
        sendname("断开连接",true);
    }
    delete socket;
    delete server;
    delete ui;
}
