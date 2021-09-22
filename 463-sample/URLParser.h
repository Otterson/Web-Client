#pragma once
#include <iostream>
#include <string>
#include <stdlib.h>
/**
	Austin Peterson
	926006358
	CSCE463
**/
using namespace std;

class URLParser {
public:
	string original_url;
	string hostname;
	int port;
	string path;
	string query;
	string fragment;
	string protocol;
	string request;
	bool valid;

	URLParser();
	//~URL();
	URLParser(string url);
	string buildHTTPRequest(bool robots);
	int findPort(string url);
	string findPath(string url);
	string findFragment(string url);
	string findQuery(string url);
	string findHost(string url);
	void display_url();



};