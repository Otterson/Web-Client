#include <iostream>
#include <string>
#include <time.h>
#include "pch.h"
#include <Windows.h>
#include "Socket.h"
#include "HTMLParserBase.h"
#include "URLParser.h"
#include <queue>
#include <unordered_set>


using namespace std;


//Adapted from winsock
Socket::Socket() {
	WSADATA wsaData;
	curPos = 0;
	//Initialize WinSock; once per program run
	WORD wVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionRequested, &wsaData) > 0) {
		printf("WSAStartup error %d\n", WSAGetLastError());
		WSACleanup();
		return;
	}
	//sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// open a TCP socket
	if (sock == INVALID_SOCKET)
	{
		printf("socket() generated error %d\n", WSAGetLastError());
		WSACleanup();
		return;
	}
	allocatedSize = 8192; //8MB initial buffer size
	buf = new char[allocatedSize];
}


//includes code adapted from winsock test
//clock reference: http://www.cplusplus.com/reference/ctime/clock/
bool Socket::SendHTTPRequest(sockaddr_in server, string HTTPrequest, bool robots, Parameters* p) {
	clock_t time;
	time = clock();
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	double request_time;
	struct hostent* remote;
	//sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (robots) {
		//cout << "\tConnecting on robots...";
	}
	else{
		//cout << "\t*Connecting on page...";
		//cout << endl << "REQUEST: " << HTTPrequest << endl;
	}
	// connect to the server on url port, default 80
	if (connect(sock, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
	{
		//printf("failed with error: %d\n", WSAGetLastError());
		return false;
	}

	request_time = (((float)(clock() - time)) / CLOCKS_PER_SEC) * 1000; //get time in ms
	//printf("done in %f ms\n", request_time);


	

	if (send(sock, HTTPrequest.c_str(), strlen(HTTPrequest.c_str())+1,0) == SOCKET_ERROR) {
		//cout << "Error sending request: " << WSAGetLastError() << endl;
		return false;
	}
	if (!Read(robots, p)) { return false; }
	closesocket(sock);

	int upper_bounds, lower_bounds;
	if (robots) {
		upper_bounds = 500;
		lower_bounds = 400;
	}
	else {
		upper_bounds = 300;
		lower_bounds = 200;
	}
		string server_response = buf;
		string response_header = server_response.substr(0, server_response.find("\r\n\r\n"));
		//cout << "RESPONSE HEADER: " << response_header << endl;
		//cout << "\tVerifying header...";
		if (response_header.size() == 0) {
			//cout << "failed with invalid header" << endl;
			return false;
		}
		string status_string = response_header.substr(9, 3);
		int HTTP_code = atoi(status_string.c_str());
		if (HTTP_code < 99 || HTTP_code >= 600) {
			if (!robots) {
				p->other++;
			}
			//cout << " failed with non-HTTP header" << endl;
			//cout << " status code " << HTTP_code << endl;
			return 0;
		}
		else {
			if (!robots) {
				if (HTTP_code >= 200 && HTTP_code < 300) {
					p->http2xx++;
				}
				else if (HTTP_code >= 300 && HTTP_code < 400) {
					p->http3xx++;
				}
				else if (HTTP_code >= 400 && HTTP_code < 500) {
					p->http4xx++;
				}
				else if (HTTP_code >= 500 && HTTP_code < 600) {
					p->http5xx++;
				}
			}
			//cout << " status code " << HTTP_code << endl;
			if (HTTP_code >= upper_bounds || HTTP_code < lower_bounds) {
				if (robots) {
					p->robots_passed++;
				}
				else {
					p->valid_http++;
				}
				//cout << "------------------------------------------\n" << response_header << endl;
				return false;
			}
		
		}

	return true;
}

//adapted from handout
bool Socket::Read(bool robots, Parameters* p)
{
	p->pages_read++;
	int max_file_size;
	if (robots) {
		max_file_size = 16384;
	}
	else {
		max_file_size = 2097152;
	}
	clock_t time;
	double read_time;
	time = clock();
	timeval timeout;
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;
	
	int ret = 0;
	fd_set fd;
	
	// set timeout to 10 seconds
	//cout << "\tLoading...";
	while (true)
	{
		FD_ZERO(&fd);
		FD_SET(sock, &fd);
		// wait to see if socket has any data (see MSDN)
		if ((ret = select(0, &fd, 0,0, &timeout)) > 0)
		{
			// new data available; now read the next segment
			int bytes = recv(sock, buf + curPos, allocatedSize - curPos, 0);
			p->bytes_read += bytes;
			if (bytes < 0) {
				//cout << "failed with error "<<WSAGetLastError() << " on recv"<<endl;
				break;
			}
			if (bytes == 0) {
				// NULL-terminate buffer
				buf[curPos + 1] = '\0';
				read_time = (((double)(clock() - time)) / CLOCKS_PER_SEC) * 1000;
				//printf("done in %f ms with %d bytes\n", read_time, curPos);
				return true; // normal completion
			}
			curPos += bytes; // adjust where the next recv goes
			if (allocatedSize - curPos < 2048) {
				//cout << "reallocating buffer" << endl;
				// resize buffer; you can use realloc(), HeapReAlloc(), or
				// memcpy the buffer into a bigger array
				if (allocatedSize == max_file_size) {
					//cout << "failed with exceeding max file size" << endl;
					return false;
				}
				buf = (char*)realloc(buf, allocatedSize *2);
				allocatedSize = allocatedSize*2;
			}

		}
		else if ((clock()-time)/CLOCKS_PER_SEC*1000000 > 5) {
			//cout << "Time limit reached" << endl;
			return false;
		}
		// report timeout
		else {
			// print WSAGetLastError()
			//cout <<"failed: error "<< WSAGetLastError() << endl;
			return false;
		}
	}

	return false;
}