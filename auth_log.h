#include "c_log.h"
// #include "c_date.h"
#include <vector>
#include <cstring>
#ifndef C_LOG2
#define C_LOG2


//base.text 
class log_auth:public log // auth log
{
	std::vector<std::string> time, machine,detail;
	public :
		log_auth() {
			log();
			path = "/var/log/auth.log";
		}
		int get_info();
};

int log_auth::get_info() {
	for (int i = 0; i < 3000; i++) {
	
			time.push_back(text.substr(i, 16));
		        i+=16;
		
			std::string s = "";
			while (text[i] != ' ') {
				s += text[i];
				i++;
			}
			machine.push_back(s);
                        std::string n = "";
			while (text[i] != '\n') {
				n += text[i];
				i++;
			}
			detail.push_back(n);
		
	}
	std:: cout << std:: endl;
	std::cout << "total: " << count << std:: endl;
	std:: cout << std:: endl;
	for (int i = 0; i < time.size(); i++) {
		std:: cout << "No." << i+1 << std::endl;
		std:: cout << "data: "<< time[i] << std::endl;
		std:: cout << "machine: " << machine[i] << std::endl;
		std:: cout << "detail: "<< detail[i] << std::endl;

		std:: cout << std:: endl;
	}
	return 1;
}

//to do...

#endif
