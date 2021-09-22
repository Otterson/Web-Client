#pragma once

#include "pch.h"
#include <Windows.h>
#include <queue>
#include <unordered_set>

class Parameters {
	public:
		HANDLE	mutex;
		HANDLE	finished;
		HANDLE	eventQuit;
		queue<string>* url_queue;
		unordered_set<string>* seen_hosts;
		unordered_set<string>* seen_IP;

	//for stats
		int queue_size, extracted_url, unique_hosts, DNS_lookup, unique_IP, robots_passed, valid_http, links_found, bytes_read, pages_read;
		int http2xx, http3xx, http4xx, http5xx, other;
};





