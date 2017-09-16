#ifndef C_LOG
#define C_LOG
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>

class log  // base : log class
{
    protected:
        int type_id;
        int row_num;
        int count;
        std::string type;
        std::vector<std::string> details;
        std::string text;
        std::ifstream f_log;
        std::string path;
    public:
        log(int type_id) {
            log();
            this->type_id = type_id;
        }
        log() {
            type_id = 1;
            details.clear();
            row_num = 0;
            count = 0;
        }
        ~log() {
            if (f_log.is_open()) {
                f_log.close();
            }
        }
        int set_path();
        int open();
        int load();
        virtual int get_info();
};

int log::load()
{
    //std::cout << row_num << ' ' << path << std::endl;
    open();
    std::string str;
    while (!f_log.eof()) {
        row_num++;
        std::getline(f_log, str);
        details.push_back(str);
        text += str;
        text += "\n";
    }
    return row_num;
}

int log::open()
{
    f_log.open(path.c_str(), std::ios::in);
    // "/var/log/apt/history.log"
    // f_log.open("/var/log/apt/history.log", std::ios::in);
    if (!f_log.is_open()) {
        std::cout << "file not open" << std::endl;
        return 0;
    }
    std::cout << "file opened" << std::endl;
    return 1;
}

int log::get_info()
{
    std::cout << text << std::endl;
}
#endif
