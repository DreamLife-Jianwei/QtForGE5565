#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

#define OFFSET 0x1000
#define BUFFERSIZE 1024

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("GE5565");
    connect(ui->pushButton_open,SIGNAL(clicked(bool)),this,SLOT(onpushbuttonopen()));
    connect(ui->pushButton_send,SIGNAL(clicked(bool)),this,SLOT(onpushbuttonSend()));
    m_Handle = 0;
    m_timer = new QTimer(this);
    connect(m_timer,SIGNAL(timeout()),this,SLOT(onTimerOut()));
}

MainWindow::~MainWindow()
{
    delete ui;
    if(m_Handle)
    {
        RFM2gClose(&m_Handle);
    }
}

void MainWindow::onpushbuttonopen()
{
    RFM2G_STATUS   result;
    QString strpath = "\\\\.\\rfm2g";
    strpath += ui->lineEdit_num->text();
    QByteArray ba = strpath.toLatin1();
    char *device = ba.data();
    result = RFM2gOpen( device, &m_Handle);
    if(result != RFM2G_SUCCESS)
    {
        qDebug() << device;
        return;
    }
    m_timer->start(100);
}

void MainWindow::onpushbuttonSend()
{
    RFM2G_STATUS result;
    QString strWrite = ui->lineEdit_write->text();
    ui->lineEdit_write->clear();
    QByteArray ba = strWrite.toLatin1();
    char* data = ba.data();
    result = RFM2gWrite(m_Handle,OFFSET,data,BUFFERSIZE);
}

void MainWindow::onTimerOut()
{
    RFM2G_STATUS result;
    char buff[BUFFERSIZE];
    result = RFM2gRead(m_Handle,OFFSET,buff,BUFFERSIZE);
    QString strtext(buff);
    if(strtext != m_strLast)
    {
        ui->textEdit_read->setText(strtext);
        m_strLast = strtext;
    }
}
