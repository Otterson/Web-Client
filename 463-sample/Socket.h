#pragma once

#include "pch.h"
#include "Parameters.h"
//class Parameters {
//public:
//	HANDLE	mutex;
//	HANDLE	finished;
//	HANDLE	eventQuit;
//	queue<string>* url_queue;
//	unordered_set<string>* seen_hosts;
//	unordered_set<string>* seen_IP;
//
//	//for stats
//	int queue_size, extracted_url, unique_hosts, DNS_lookup, unique_IP, robots_passed, valid_http, links_found;
//};

class Socket {
public:
	SOCKET sock; // socket handle
	char* buf; // current buffer
	int allocatedSize; // bytes allocated for buf
	int curPos; // current position in buffer


	Socket();
	bool SendHTTPRequest(sockaddr_in server, string HTTPrequest, bool robots, Parameters* p);
	bool Read(bool robots, Parameters* p);
	
};
