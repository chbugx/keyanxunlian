#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "c_log2.h"

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

void MainWindow::on_pushButton_clicked()
{
    log_apt lg;
    lg.load();
    lg.get_info();
    ui->textBrowser->setText(lg.info.c_str());
   // ui->tab->set
}

void MainWindow::on_pushButton_4_clicked()
{
    ui->textBrowser->setText("");
}
