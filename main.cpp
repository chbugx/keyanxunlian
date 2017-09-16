#include "mainwindow.h"
#include <QApplication>
#include <iostream>
//#include "c_log2.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
//        log_apt lg;
//        lg.load();
//        lg.get_info();
        std::cout << "end!" << std::endl;
    return a.exec();
}
