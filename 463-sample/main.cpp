/* main.cpp
 * CSCE 463 Sample Code 
 * by Dmitri Loguinov
 */
#include "pch.h"
#include "URLParser.h"
#include "Socket.h"
#include "HTMLParserBase.h"
#include <string.h>
#include <iostream>
#include <Windows.h>
#include <cstring>
#include <stdio.h>
#include <queue>
#include <fstream>
#include <unordered_set>
#include <thread>
#include <sys/types.h>

using namespace std;
// this class is passed to all threads, acts as shared memory


// function inside winsock.cpp
void winsock_test (void);

// this function is where threadA starts
UINT threadA (LPVOID pParam)
{
	Parameters *p = ((Parameters*)pParam);

	// wait for mutex, then print and sleep inside the critical section
	WaitForSingleObject (p->mutex, INFINITE);					// lock mutex
	printf ("threadA %d started\n", GetCurrentThreadId ());		// print inside critical section to avoid screen garbage
	Sleep (1000);												// sleep 1 second
	ReleaseMutex (p->mutex);									// release critical section

	// signal that this thread has finished its job
	ReleaseSemaphore (p->finished, 1, NULL);

	// wait for threadB to allow us to quit
	WaitForSingleObject (p->eventQuit, INFINITE);

	// print we're about to exit
	WaitForSingleObject (p->mutex, INFINITE);					
	printf ("threadA %d quitting on event\n", GetCurrentThreadId ());		
	ReleaseMutex (p->mutex);										

	return 0;
}

// this function is where threadB starts
UINT threadB (LPVOID pParam)
{
	Parameters *p = ((Parameters*)pParam);

	// wait for both threadA threads to quit
	WaitForSingleObject (p->finished, INFINITE);
	WaitForSingleObject (p->finished, INFINITE);

	printf ("threadB woken up!\n");				// no need to sync as only threadB can print at this time
	Sleep (3000);

	printf ("threadB setting eventQuit\n"); 	// no need to sync as only threadB can print at this time

	// force other threads to quit
	SetEvent (p->eventQuit);

	return 0;
}



bool main_function(string url_input, unordered_set<string>* seenhosts, unordered_set<string>* seenIPs, Parameters* p) {
	URLParser url = URLParser(url_input);
	
	Socket socket = Socket();
	Socket socket2 = Socket();
	clock_t time;
	time = clock();
	double request_time;

	//check hostname uniqueness
	size_t prevSize = seenhosts->size();
	seenhosts->insert(url_input);
	//cout << "\tChecking host uniqueness... ";
	if (seenhosts->size() > prevSize) {
		//cout << "passed" << endl;
		p->unique_hosts++;
	}
	else {
		///cout << "failed" << endl;
		return false;
	}

	struct hostent* remote;

	// structure for connecting to server
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(url.port);		// host-to-network flips the byte order


	// first assume that the string is an IP address

	DWORD IP = inet_addr(url.original_url.c_str());
	//cout << "\tDoing DNS... ";
	if (IP == INADDR_NONE)
	{
		// if not a valid IP, then do a DNS lookup
		if ((remote = gethostbyname(url.hostname.c_str())) == NULL)
		{
			//printf("Invalid string: neither FQDN, nor IP address\n");
			return false;
		}
		else // take the first IP address and copy into sin_addr
			p->DNS_lookup++;
			memcpy((char*)&(server.sin_addr), remote->h_addr, remote->h_length);
	}
	else
	{
		// if a valid IP, directly drop its binary version into sin_addr
		server.sin_addr.S_un.S_addr = IP;
	}
	request_time = (((double)(clock() - time)) / CLOCKS_PER_SEC) * 1000; //get time in ms
	//printf("done in %f ms, found %s\n", request_time, inet_ntoa(server.sin_addr));


	//check IP uniqueness
	size_t prevSizeIP = seenIPs->size();
	seenIPs->insert(inet_ntoa(server.sin_addr));
	//cout << "\tChecking IP uniqueness... ";
	if (seenIPs->size() > prevSizeIP) {
		//cout << "passed" << endl;
		p->unique_IP++;
	}
	else {
		//cout << "failed" << endl;
		return false;
	}
	
	string HTTPRobots = url.buildHTTPRequest(true); //true = robots
	string HTTPRequest = url.buildHTTPRequest(false);

	if (socket.SendHTTPRequest(server, HTTPRobots, true, p) == false) {
		return 0;
	}
	if (socket2.SendHTTPRequest(server, HTTPRequest, false, p) == false) {
		return 0;
	}


	HTMLParserBase* HTMLparser = new HTMLParserBase();

	string stripped_url = "http://" + url.hostname;
	char* url_buffer = new char[stripped_url.length() + 1];
	strcpy_s(url_buffer, (int)strlen(stripped_url.c_str()) + 1, stripped_url.c_str());

	//cout << "\t+ Parsing page...";

		
	time = clock();
	int link_count = 0;
	double parse_time;
	char* socket_buffer = socket2.buf;
	string server_response = socket2.buf;
	string response_header = server_response.substr(0, server_response.find("\r\n\r\n"));
	char* parsed_html = HTMLparser->Parse(socket_buffer, socket2.allocatedSize, url_buffer, (int)strlen(stripped_url.c_str()), &link_count);
	p->links_found += link_count;
	parse_time = (((double)(clock() - time)) / CLOCKS_PER_SEC) * 1000;
	//printf("done in %f ms with %d links\n", parse_time, link_count);
		
	//printf("------------------------------------------\n");
	//cout << response_header << endl << endl;
	return true;
}


void stats_function(Parameters* p) {
	int time = 0;
	
	int mbps, pps;
	while (true) {
		//TODO links found
		printf("[%3d]  %6d Q %7d E %6d H %6d D %5d I %5d R %5d C %4d L\n", time, p->queue_size, p->extracted_url, p->unique_hosts, p->DNS_lookup, p->unique_IP, p->robots_passed, p->valid_http, p->links_found);
		pps = ((p->pages_read) / 2);
		mbps = ((p->bytes_read ) / 2 / 1000);
		p->pages_read = 0;
		p->bytes_read = 0;
		printf("      *** crawling %.2f pps @ %.2f Mbps\n", (float)pps, (float)mbps);
		time += 2;
		Sleep (2000);
		if (p->url_queue->size() == 0) {
			SetEvent(p->eventQuit);
			break;
		}
	}
	int placeholder = 0;
	cout << endl;
	printf("Extracted %d URLs @ %d/s\n", p->extracted_url, placeholder);
	printf("Looked up %d DNS names @ %d/s\n", p->DNS_lookup, placeholder);
	printf("Attempted %d robots @ %d/s\n", p->robots_passed, placeholder);
	printf("Crawled %d pages @ %d/s\n", p->valid_http, placeholder);
	printf("Parsed %d links @ %d/s\n", p->links_found, placeholder);
	printf("HTTP codes: 2xx = %d, 3xx = %d, 4xx = %d, 5xx = %d, other = %d\n\n", p->http2xx, p->http3xx, p->http4xx, p->http5xx, p->other);

}

int crawler_function(Parameters* p) {
	//printf("Starting thread %d \n", GetCurrentThreadId());		// print inside critical section to avoid screen garbage
	// wait for mutex, then print and sleep inside the critical section
	while (true) {
		WaitForSingleObject(p->mutex, INFINITE);					// lock mutex
		if (p->url_queue->size() == 0) {
			ReleaseMutex(p->mutex);									// release critical section

			break;
		}
		string url = p->url_queue->front();
		p->url_queue->pop();

		p->queue_size = p->url_queue->size();
		p->extracted_url++;
		ReleaseMutex(p->mutex);									// release critical section
		main_function(url, p->seen_hosts, p->seen_IP, p);
	}
	WaitForSingleObject(p->mutex, INFINITE);
	//printf("thread %d quitting on event\n", GetCurrentThreadId());
	ReleaseMutex(p->mutex);
	
	return 0;
}



bool populate_queue(string filename, queue<string>* urls) {
	ifstream input_file(filename);
	if (input_file.is_open()) {
		string url;
		cout << "Opened: " << filename << endl;
		while (getline(input_file, url)) {
			urls->push(url);
		}
		input_file.close();
		//cout << "Queue size: " << urls->size() << endl;
		return true;
	}
	else {
		cout << "Invalid input file\n";
		return false;
	}
}

int main(int argc, char** argv)
{

	//if (argc != 3) {
	//	cout << "Invalid parameters: ./463-sample.exe <num threads> <filename.txt>" << endl;
	//	return 0;
	//}

	//int num_threads = atoi(argv[1]); //may need char* to int conversion
	//
	//string url_filename = argv[2];

	int num_threads = 4000;
	//string url_filename = "URL-input-100.txt";
	string url_filename = "URL-input-1M.txt";
	//string url_filename = "URL-input-1M-2019.txt";

	queue<string>* url_list = new queue<string>;

	if (!populate_queue(url_filename, url_list)) {
		cout << "Invalid Filename" << endl;
		return 0;
	}


	unordered_set<string>* seenhosts = new unordered_set<string>;
	unordered_set<string>* seenIPs = new unordered_set<string>;


	HANDLE* handles = new HANDLE[num_threads+1];
	Parameters p;

	// create a mutex for accessing critical sections (including printf); initial state = not locked
	p.mutex = CreateMutex(NULL, 0, NULL);
	// create a semaphore that counts the number of active threads; initial value = 0, max = 2
	p.finished = CreateSemaphore(NULL, 0, num_threads, NULL);
	// create a quit event; manual reset, initial state = not signaled
	p.eventQuit = CreateEvent(NULL, true, false, NULL);

	p.url_queue = url_list;
	p.seen_hosts = seenhosts;
	p.seen_IP = seenIPs;

	handles[num_threads] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)stats_function, &p, 0, NULL);
	for (int i = 0; i < num_threads; i++) {
		handles[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)crawler_function, &p, 0, NULL);
	}
	
	for (int i = 0; i < num_threads; i++)
	{
		WaitForSingleObject(handles[i], INFINITE);
		CloseHandle(handles[i]);
	}

	// connect to a server; test basic winsock functionality
	//winsock_test ();

	//printf ("-----------------\n");

	// print our primary/secondary DNS IPs
	DNS dns;
	dns.printDNSServer();

	//printf ("-----------------\n");

	CPU cpu;
	// run a loop printing CPU usage 10 times
	for (int i = 0; i < 10; i++)
	{
		// average CPU utilization over 200 ms; must sleep at least a few milliseconds *after* the constructor 
		// of class CPU and between calls to GetCpuUtilization
		Sleep(200);
		// now print
		double util = cpu.GetCpuUtilization(NULL);
		// -2 means the kernel counters did not accumulate enough to produce a result
		if (util != -2)
			printf("current CPU utilization %f%%\n", util);
	}

	printf("-----------------\n");

	// thread handles are stored here; they can be used to check status of threads, or kill them
	//HANDLE* handles = new HANDLE[3];
	//Parameters p;

	//// create a mutex for accessing critical sections (including printf); initial state = not locked
	//p.mutex = CreateMutex(NULL, 0, NULL);
	//// create a semaphore that counts the number of active threads; initial value = 0, max = 2
	//p.finished = CreateSemaphore(NULL, 0, 2, NULL);
	//// create a quit event; manual reset, initial state = not signaled
	//p.eventQuit = CreateEvent(NULL, true, false, NULL);

	//// get current time; link with winmm.lib
	//clock_t t = clock();

	//// structure p is the shared space between the threads
	//handles[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadA, &p, 0, NULL);		// start threadA (instance #1) 
	//handles[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadA, &p, 0, NULL);		// start threadA (instance #2)
	//handles[2] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadB, &p, 0, NULL);		// start threadB 

	//// make sure this thread hangs here until the other three quit; otherwise, the program will terminate prematurely
	//for (int i = 0; i < 3; i++)
	//{
	//	WaitForSingleObject(handles[i], INFINITE);
	//	CloseHandle(handles[i]);
	//}

	//printf("terminating main(), completion time %.2f sec\n", (double)(clock() - t) / CLOCKS_PER_SEC);
	return 0;
}