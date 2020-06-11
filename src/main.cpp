#include <include/WebServer.h>
#include <iostream>
#include <string>

int main(const int argc, const char **argv)
{
	std::string port;

	if (argc != 4)
	{
		std::cerr << "use with params: --PORT "  << std::endl;
		// return -1;
		port = "8080";
	}
	else {
		port = argv[1];
	}

	WebServer webServer("0.0.0.0", atoi(port.c_str()));
	if (webServer.init() != 0)
		return -1;

	std::cout << "Configure WebServer done!" << std::endl;
	

	std::cout << "WebServer run on localhost " + port + " port" << std::endl;
	webServer.run();

	system("pause");

	return 0;
}