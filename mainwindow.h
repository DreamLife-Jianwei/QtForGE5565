#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>


#include <QTimer>
#include "rfm2g_windows.h"
#include "rfm2g_api.h"



namespace Ui {
class MainWindow;
}

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private slots:
    void onpushbuttonopen();
    void onpushbuttonSend();
    void onTimerOut();

private:
    Ui::MainWindow *ui;
    RFM2GHANDLE    m_Handle;
    QTimer* m_timer;
    QString m_strLast;
};

#endif // MAINWINDOW_H
