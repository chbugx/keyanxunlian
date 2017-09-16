#ifndef C_LOG2
#define C_LOG2
#include "c_log.h"
// #include "c_date.h"
#include <vector>
#include <cstring>
#include <cstdio>
#include <sstream>
//base.text
class log_apt:public log // apt log
{
public: std::vector<std::string> Start_time, End_time, Command, User;
        std::string info;
    public :
        log_apt() {
            log();
            path = "/var/log/apt/history.log";
        }
        int get_info();
};

int log_apt::get_info() {
    for (int i = 0; i < text.length()-22; i++) {
        bool f;
        if (text.substr(i, 11) == "Start-Date:") {
            Start_time.push_back(text.substr(i+12, 20));
            f = 1;
        }
        if (text.substr(i, 12) == "Commandline:") {
            i += 13;
            std::string s = "";
            while (text[i] != '\n') {
                s += text[i];
                i++;
            }
            Command.push_back(s);
        }
        if (text.substr(i, 13) == "Requested-By:") {
            f = 0;
            i += 14;
            std::string s = "";
            while (text[i] != ' ') {
                s += text[i];
                i++;
            }
            User.push_back(s);
        }
        if (text.substr(i, 9) == "End-Date:") {
            End_time.push_back(text.substr(i+10, 20));
            if (f == 1) {
                User.push_back("System");
            }
            count ++;
            f = 0;
            while (Command.size() < Start_time.size()) {
                Command.push_back("unknown");
            }
        }
    }

    std:: cout << "hehe" << std::endl;
    std:: cout << Start_time.size() << ' ' << Command.size() << ' ' << User.size() << std:: endl;
    std:: cout << std:: endl;
    std::cout << "total: " << count << std:: endl;
    std:: cout << std:: endl;
    std::stringstream str_out;
    for (int i = 0; i < Start_time.size(); i++) {
        std:: cout << "No." << i+1 << std::endl;
        std:: cout << "Start_date: "<< Start_time[i] << std::endl;
        std:: cout << "Command: " << Command[i] << std::endl;
        std:: cout << "User: " << User[i] << std::endl;
        std:: cout << "End_date: " << End_time[i] << std::endl;
        std:: cout << std:: endl;
        str_out << "No." << i+1 << '\n';
        str_out << "Start_date: "<< Start_time[i] << '\n';
        str_out << "Command: " << Command[i] << '\n';
        str_out << "User: " << User[i] << '\n';
        str_out << "End_date: " << End_time[i] << '\n';
        str_out << std:: endl;
    }
    info = str_out.str();
    return 1;
}

//to do...

#endif
