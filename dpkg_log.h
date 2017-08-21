#include "c_log.h"
// #include "c_date.h"
#include <vector>
#include <cstring>
#ifndef C_LOG2
#define C_LOG2


//base.text 
class log_dpkg:public log // dpkg log
{
	std::vector<std::string> time, detail;
	public :
		log_dpkg() {
			log();
			path = "/var/log/dpkg.log";
		}
		int get_info();
};

int log_dpkg::get_info() {
	for (int i = 0; i < 3000; i++) {
		 
			time.push_back(text.substr(i, 20));
                        i+=20;

			std::string s = "";
			while (text[i] != '\n') {
				s += text[i];
				i++;
			}
			detail.push_back(s);
                        count++;

	}

	std:: cout << std:: endl;
	std::cout << "total: " << count << std:: endl;
	std:: cout << std:: endl;
	for (int i = 0; i < time.size(); i++) {
		std:: cout << "No." << i+1 << std::endl;
		std:: cout << "data: "<< time[i] << std::endl;
		std:: cout << "detail: " << detail[i] << std::endl;

		std:: cout << std:: endl;
	}
	return 1;
}

//to do...

#endif
