#include <string>
#include <istream>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <iterator>
#include <regex>
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>
#include <include/WebServer.h>


#define BUFSIZE 32000

#define WWWROOT ".\\wwwroot"

std::string createResponse(int statusCode, std::string contentType, std::string& content)
{
	std::ostringstream oss;
	oss << "HTTP/1.1 " << statusCode << " OK\r\n";
	oss << "Cache-Control: no-cache, private\r\n";
	oss << "Content-Type: " << contentType << "\r\n";
	oss << "Content-Length: " << content.size() << "\r\n";
	oss << "\r\n";
	oss << content;

	return oss.str();
};

// write to `content` from file with name like `route`
// and set `statusCode`
int fillContent(const std::string& route, std::string& content)
{
	int statusCode = 404;

	content.clear();

	std::string fileRoute = route;
	std::ifstream f(WWWROOT + std::string("\\") + route.substr(1, route.length()), std::ios::binary);
	if (f.good())
	{
		std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(f), {});
		for (char c : buffer)
		{
			content.push_back(c);
		}
		statusCode = 200;
	}
	else {
		content = "<h4>404 Not Found </h4>";
	}
	f.close();

	return statusCode;
}

std::vector<std::string> split(std::string s, std::regex r)
{
	std::vector<std::string> splits;
	std::smatch m;

	while (std::regex_search(s, m, r))
	{
		int split_on = m.position();
		splits.push_back(s.substr(0, split_on));
		s = s.substr(split_on + m.length());
	}

	if (!s.empty()) {
		splits.push_back(s);
	}

	return splits;
}

// Router for parse HTTP 
class Router
{
public:
	Router(const char* msg)
	{
		std::istringstream iss(msg);

		// parse as GET /index HTTP 1.1
		std::vector<std::string> parsed((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());

		if (parsed.size() < 3) {
			throw std::exception("msg invalid");
		}

		this->fullMessage = msg;

		this->method = parsed[0];

		std::smatch m;
		std::string query;
		// Initialization `route` from `parsed`
		if (std::regex_search(parsed[1], std::regex("\\/\\S+")))
		{
			if (std::regex_search(parsed[1], m, std::regex("\\?")))
			{
				this->route = parsed[1].substr(0, m.position());
				query = parsed[1].substr(m.position(), parsed[1].length());
			}
			else {
				this->route = parsed[1];
			}
		}
		else 
		{	
			this->route = "/";
		};

		// Initialization `queryParams` vector with parsed query on vector of strings
		if (query.length() != 1 && query.length() != 0)
		{

			std::vector<std::string> splitVector = split(query.substr(1, query.length()), std::regex("&+"));

			for (auto &i : splitVector)
			{
				std::vector<std::string> splitParam = split(i, std::regex("=+"));

				std::vector<std::string> tmp;
				int count = 0;
				for (auto &j : splitParam)
				{
					tmp.push_back(j);
					count++;
				}

				if (count == 1) {
					tmp.pop_back();
				}

				queryParams.push_back(tmp);
			}
		}

	};

	~Router() {};


	std::string getfullRoute() const
	{
		std::string result = route;
		std::string query;
		for (auto &i : this->queryParams)
		{
			query = i[0] + "=" + i[1];
		}

		if (query.length() != 0)
		{
			result += "?" + query;
		}

		return result;
	};

	std::string getMethod() const
	{
		return this->method;
	};
	
	std::vector<std::vector<std::string>> getQueryParams() const
	{
		return this->queryParams;
	}

	std::string getRoute() const
	{
		return this->route;
	}

	const char* getFullMessage() const
	{
		return this->fullMessage;
	}

private:
	std::string route;
	std::vector<std::vector<std::string>> queryParams;
	std::string method;
	const char* fullMessage;
};

// Handler for when a message is received from the client
void WebServer::onMessageReceived(int clientSocket, const char* msg, int length)
{
	const std::regex cssRegex("\/.[^\/]*css$");
	const std::regex jsRegex("\/.[^\/]*js$");
	const std::regex htmlRegex("\/.[^\/]*html$");
	const std::regex cgiPhpRegex("^\/.[^\/]*php");

	std::string contentType = "text/html";
	int statusCode = 404;
	std::string content;

	const Router router(msg);
	std::string route = router.getRoute();
	std::string method = router.getMethod();
	
	std::cout << std::endl << method << " " << router.getfullRoute() << std::endl;
	
	if (router.getQueryParams().size() == 0)
	{
		std::cout << "Query params: false" << std::endl;
	}
	else std::cout << "Query params: " << std::endl;

	for (auto &i : router.getQueryParams())
	{
		std::cout << i[0] << " = " << i[1] << std::endl;
	}

	// ex: /images/picture.png
	if (std::regex_search(route, std::regex("^\/images\/.[^\/]*(png|jpeg|jpg)$")) && method == "GET")
	{
		std::cout << "/images route enabled!" << std::endl;

		if (std::regex_search(route, std::regex(".*png$")))
		{
			contentType = "image/png";
		}
		else if (std::regex_search(route, std::regex(".*(jpeg|jpg)$")))
		{
			contentType = "image/jpeg";
		}
		else {
			content = "<h4>404 Not Found </h4>";
			std::string response = createResponse(statusCode, contentType, content);
			sendToClient(clientSocket, response.c_str(), response.size());
			return;
		}

		statusCode = fillContent(route, content);
		std::string response = createResponse(statusCode, contentType, content);
		sendToClient(clientSocket, response.c_str(), response.size());
		return;
	}
	
	// ex: /index.html or /index.php?a=12b=23
	if (std::regex_search(router.getRoute(), std::regex("^\/[^\/]*$")))
	{
		std::cout << "/... enabled!" << std::endl;

		// "/" path -> "/index.html"
		if (route == "/" && method == "GET")
		{
			std::cout << "index" << std::endl;
			statusCode = fillContent("/index.html", content);
			std::string response = createResponse(statusCode, contentType, content);
			sendToClient(clientSocket, response.c_str(), response.size());
			return;
		}

		if (std::regex_search(route, htmlRegex))
		{
			contentType = "text/html";

			std::cout << "HTML enabled!" << std::endl;

			if (method == "GET")
			{
				statusCode = fillContent(route, content);
				std::string response = createResponse(statusCode, contentType, content);
				sendToClient(clientSocket, response.c_str(), response.size());
				return;
			}
			else if (method == "POST")
			{
				statusCode = 400;
				content = "<h4>400 Bad Request</h4>";
				std::string response = createResponse(statusCode, contentType, content);
				sendToClient(clientSocket, response.c_str(), response.size());
				return;
			}
			else if (method == "DELETE")
			{
				statusCode = 400;
				content = "<h4>400 Bad Request</h4>";
				std::string response = createResponse(statusCode, contentType, content);
				sendToClient(clientSocket, response.c_str(), response.size());
				return;
			}
			else if (method == "PUT")
			{
				statusCode = 400;
				content = "<h4>400 Bad Request</h4>";
				std::string response = createResponse(statusCode, contentType, content);
				sendToClient(clientSocket, response.c_str(), response.size());
				return;
			}
		}

		// CSS load
		if (std::regex_search(route, cssRegex) && method == "GET")
		{
			std::cout << "CSS enabled!" << std::endl;

			contentType = "text/css";
			statusCode = fillContent(route, content);
			std::string response = createResponse(statusCode, contentType, content);
			sendToClient(clientSocket, response.c_str(), response.size());
			return;
		}

		// JS load
		if (std::regex_search(route, jsRegex) && method == "GET")
		{
			std::cout << "JS enabled!" << std::endl;

			contentType = "text/javascript";
			statusCode = fillContent(route, content);
			std::string response = createResponse(statusCode, contentType, content);
			sendToClient(clientSocket, response.c_str(), response.size());
			return;
		}

		if (std::regex_search(route, cgiPhpRegex))
		{
			std::cout << "php-cgi enabled!" << std::endl;

			contentType = "text/html";

			if (method == "GET")
			{
				char psBuffer[BUFSIZE];
				FILE *pPipe;

				std::string cmd = "php-cgi " + std::string(WWWROOT + std::string("\\php\\")) + route.substr(1, route.length());

				for (auto &i : router.getQueryParams())
				{
					cmd += " " + i[1];
				}

				std::cout << "CMD: " << cmd << std::endl;

				pPipe = _popen(cmd.c_str(), "rt");

				while (fgets(psBuffer, BUFSIZE, pPipe))
				{
					content += psBuffer;
				}


				if (feof(pPipe))
				{
					std::cout << "php-cgi returned " << _pclose(pPipe) << std::endl;

					statusCode = 200;
					std::smatch m;
					if (std::regex_search(content, m, std::regex("<html>")))
					{
						content = content.substr(m.position(), content.length());
					}
					std::string response = createResponse(statusCode, contentType, content);
					sendToClient(clientSocket, response.c_str(), response.size());
					return;
				}
				else {
					std::cout << "Error: php-cgi failed to read the pipe to the end" << std::endl;

					content = "<h4>500 Internal Server Error</h4>";
					statusCode = 500;
					std::string response = createResponse(statusCode, contentType, content);
					sendToClient(clientSocket, response.c_str(), response.size());
					return;
				}

			}
			else if (method == "POST")
			{
				if (std::regex_search(route, std::regex("\/form.php$")))
				{

					std::istringstream iss(msg);
					std::vector<std::string> parsed((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());

					std::ofstream f("names.txt", std::ios::app);

					auto nameVector = split(parsed[parsed.size() - 1], std::regex("\="));
					auto name = nameVector[1];

					if (!f.is_open())
					{
						statusCode = 500;
						content = "<h4>500 Internal Server Error</h4>";
						std::string response = createResponse(statusCode, contentType, content);
						sendToClient(clientSocket, response.c_str(), response.size());
						return;
					}

					f << name << '\n';
					f.close();

					statusCode = 200;
					content = name;
					std::string response = createResponse(statusCode, contentType, content);
					sendToClient(clientSocket, response.c_str(), response.size());
					return;
				}

				statusCode = 400;
				content = "<h4>400 Bad Request</h4>";
				std::string response = createResponse(statusCode, contentType, content);
				sendToClient(clientSocket, response.c_str(), response.size());
				return;
			}
			else if (method == "PUT")
			{
				if (std::regex_search(route, std::regex("\/form.php$")))
				{

					std::istringstream iss(msg);
					std::vector<std::string> parsed((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());

					std::fstream f("names.txt", std::ios::app);

					auto nameVector = split(parsed[parsed.size() - 1], std::regex("\="));
					auto name = nameVector[1];
					auto newNameVector = split(parsed[parsed.size() - 2], std::regex("\="));
					auto newName = newNameVector[1];

					if (!f.is_open())
					{
						statusCode = 500;
						content = "<h4>500 Internal Server Error</h4>";
						std::string response = createResponse(statusCode, contentType, content);
						sendToClient(clientSocket, response.c_str(), response.size());
						return;
					}

				/*	std::string buf;
					while (getline(f, buf))
					{
						if (buf)
					}
*/
					f << name << '\n';
					f.close();

					statusCode = 200;
					content = name;
					std::string response = createResponse(statusCode, contentType, content);
					sendToClient(clientSocket, response.c_str(), response.size());
					return;
				}

				statusCode = 400;
				content = "<h4>400 Bad Request</h4>";
				std::string response = createResponse(statusCode, contentType, content);
				sendToClient(clientSocket, response.c_str(), response.size());
				return;
			}
			else if (method == "DELETE")
			{
				statusCode = 400;
				content = "<h4>400 Bad Request</h4>";
				std::string response = createResponse(statusCode, contentType, content);
				sendToClient(clientSocket, response.c_str(), response.size());
				return;
			}
		}
	}

	contentType = "text/html";
	content = "<h4>404 Not Found </h4>";
	statusCode = 404;
	std::string response = createResponse(statusCode, contentType, content);
	sendToClient(clientSocket, response.c_str(), response.size());
	return;
}

// Handler for client connections
void WebServer::onClientConnected(int clientSocket)
{
	std::cout << "--> Client connected: " << clientSocket << std::endl;
}

// Handler for client disconnections
void WebServer::onClientDisconnected(int clientSocket)
{
	std::cout << "--> Client disconnected: " << clientSocket << std::endl;
}

