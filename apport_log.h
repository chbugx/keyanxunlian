#include "c_log.h"
// #include "c_date.h"
#include <vector>
#include <cstring>
#ifndef C_LOG2
#define C_LOG2


//base.text 
class log_apport:public log // apport log
{
	std::vector<std::string> pid, time, detail;
	public :
		log_apport() {
			log();
			path = "/var/log/apport.log";
		}
		int get_info();
};

int log_apport::get_info() {
	for (int i = 0; i < text.length()-22; i++) {
		 bool f;
                  f=0;
		 if (text.substr(i, 3) == "pid") {
                        i+=4;
			std::string n = "";
			while (text[i] != ')') {
				n += text[i];
				i++;
			}
			pid.push_back(n);
                        f=1;
		}
                 if (f==1){
                        i+=2;
			time.push_back(text.substr(i, 24));

                        i+=26;

			std::string s = "";
			while (text[i] != '\n') {
				s += text[i];
				i++;
			}
			detail.push_back(s);
                        count++;

		}
	}

	std:: cout << std:: endl;
	std::cout << "total: " << count << std:: endl;
	std:: cout << std:: endl;
	for (int i = 0; i < time.size(); i++) {
		std:: cout << "ERROR." << i+1 << std::endl;
		std:: cout << "pid: "<< pid[i] << std::endl;
		std:: cout << "date: "<< time[i] << std::endl;
		std:: cout << "detail: " << detail[i] << std::endl;

		std:: cout << std:: endl;
	}
	return 1;
}

//to do...

#endif
