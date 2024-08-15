#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTcpSocket>
#include <QTcpServer>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    Ui::Widget *ui;

public:
    void disablestart(bool);

public slots:
    void startconnect();

public slots: // tcp_start
    void connecting();
    void readyRead();
    void readySend();
    void quit();
public:
    void send(QString);
    void sendname(QString,bool);
};
#endif // WIDGET_H
