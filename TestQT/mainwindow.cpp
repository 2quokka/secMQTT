#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


#define FILEPATH "pub.exe"
#define ON "ON"
#define OFF "OFF"

void gen_random(char *s, const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}

using namespace std;
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_lamp_on_clicked()
{
    int pid, status=0;
    if((pid = fork()) == 0){ //child
        execl(FILEPATH, "pub.exe", ui->host->text().toStdString().c_str(), "LAMP", ON, ui->keyval->text().toStdString().c_str(), (char*)0);
    }

    waitpid(pid, &status, 0);
    if( status== 0 )
        ui->log->append("[ Success ] LAMP ON");
    else
        ui->log->append("[ Fail ] LAMP ON");
}

void MainWindow::on_lamp_off_clicked()
{
    int pid, status=0;
    if((pid = fork()) == 0){ //child
        execl(FILEPATH, "pub.exe", ui->host->text().toStdString().c_str(), "LAMP", OFF, ui->keyval->text().toStdString().c_str(), (char*)0);
    }

    waitpid(pid, &status, 0);
    if( status== 0 )
        ui->log->append("[ Success ] LAMP OFF");
    else
        ui->log->append("[ Fail ] LAMP OFF");
}

void MainWindow::on_tv_on_clicked()
{
    int pid, status=0;
    if((pid = fork()) == 0){ //child
        execl(FILEPATH, "pub.exe", ui->host->text().toStdString().c_str(), "TV", ON, ui->keyval->text().toStdString().c_str(), (char*)0);
    }

    waitpid(pid, &status, 0);
    if( status== 0 )
        ui->log->append("[ Success ] TV ON");
    else
        ui->log->append("[ Fail ] TV OFF");
}

void MainWindow::on_tv_off_clicked()
{
    int pid, status=0;
    if((pid = fork()) == 0){ //child
        execl(FILEPATH, "pub.exe", ui->host->text().toStdString().c_str(), "TV", OFF, ui->keyval->text().toStdString().c_str(), (char*)0);
    }

    waitpid(pid, &status, 0);
    if( status== 0 )
        ui->log->append("[ Success ] TV OFF");
    else
        ui->log->append("[ Fail ] TV OFF");
}

void MainWindow::on_boiler_on_clicked()
{
    int pid, status=0;
    if((pid = fork()) == 0){ //child
        execl(FILEPATH, "pub.exe", ui->host->text().toStdString().c_str(), "BOILER", ON, ui->keyval->text().toStdString().c_str(), (char*)0);
    }

    waitpid(pid, &status, 0);
    if( status== 0 )
        ui->log->append("[ Success ] BOILER ON");
    else
        ui->log->append("[ Fail ] BOILER ON");
}

void MainWindow::on_boiler_off_clicked()
{
    int pid, status=0;
    if((pid = fork()) == 0){ //child
        execl(FILEPATH, "pub.exe", ui->host->text().toStdString().c_str(), "BOILER", OFF, ui->keyval->text().toStdString().c_str(), (char*)0);
    }

    waitpid(pid, &status, 0);
    if( status== 0 )
        ui->log->append("[ Success ] BOILER OFF");
    else
        ui->log->append("[ Fail ] BOILER OFF");
}

void MainWindow::on_pushButton_2_clicked()
{
    ui->keyval->setText("123456789");
}

void MainWindow::on_pushButton_3_clicked()
{
    char rkey[11];
    gen_random(rkey, 10);
    ui->keyval->setText(rkey);
}
