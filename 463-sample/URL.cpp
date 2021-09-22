#include <iostream>
#include <stdio.h>
#include<sstream>
#include <string>
#include "URLParser.h"
#include "pch.h"

using namespace std;


URLParser::URLParser() {}



URLParser::URLParser(string url) {
	valid = true;
	//cout << "URL: " << url << endl;
	//cout << "\tParsing URL...";

	if (url.substr(0, 5) == "https") {
		valid = false;
		//cout << " failed with invalid scheme" << endl;
		return;
	}
	url = url.substr(7); 
	hostname = findHost(url);
	port = findPort(url);
	if (port == 0) {
		//cout << "failed with invalid port" << endl;
		valid = false;
	}

	query = findQuery(url);
	fragment = findFragment(url);
	path = findPath(url);

	//cout << " host " << hostname << ", port " << port << endl;
	




}

string URLParser::findHost(string url) {
	string host = "";
	if (url.find(":") != string::npos) {
		host = url.substr(0, url.find(":"));
	}
	else if (url.find("/") != string::npos) {
		host = url.substr(0, url.find("/"));
	}
	else if (url.find("?") != string::npos) {
		host = url.substr(0, url.find("?"));
	}
	else if (url.find("#") != string::npos) {
		host = url.substr(0, url.find("#"));
	}
	else host = url;
	return host;
}
int URLParser::findPort(string url) {
	string portString;
	int portNum = 80;
	if (int portPosition = url.find(":") !=string::npos) {
		portString = url.substr(0, url.find("/"));
		portString = url.substr(url.find(":") + 1);
		portNum = atoi(portString.c_str());
	}
	return portNum;
}

string URLParser::findPath(string url) {
	string pathString = "/";
	if (url.find("/") != string::npos) {
		if (url.find("?") != string::npos) {
			pathString = url.substr(url.find("/"), url.find("?")-url.find("/"));
		}
		else if (url.find("#") != string::npos) {
			pathString = url.substr(url.find("/"), url.find("#")-url.find("/"));
		}
		else {
			pathString = url.substr(url.find("/"));
		}
	}
	
	return pathString;
}

string URLParser::findFragment(string url) {
	string fragString = "";
	if(url.find("#") != string::npos){
		fragString = url.substr(url.find("#"));
	}
	return fragString;

}
string URLParser::findQuery(string url) {
	string queryString = "";
	if (url.find("?") != string::npos) {
		if (url.find("#") != string::npos) {
			queryString = url.substr(url.find("?"), url.find("#")-url.find("?"));
		}
		else {
			queryString = url.substr(url.find("?"));
		}
	}
	return queryString;
}

string URLParser::buildHTTPRequest(bool robots) {
	string full_request;
	string http_request;
	string req_type;
	if (robots) {
		req_type = "HEAD ";
		http_request = http_request + "/robots.txt";
	}
	else {
		req_type = "GET ";
		http_request = http_request + path;
		if (query != "") {
			http_request = http_request + "?" + query;
		}
	}
	
	
	full_request = req_type +http_request+" HTTP/1.0\r\nUser-agent: TAMUCrawler/1.2\r\nHost: " +
		hostname + "\r\nConnection: close\r\n\r\n";
	//cout << "\n\nHTTPREQUEST: " << full_request << endl << endl;
	return full_request;
}
void URLParser::display_url() {
	cout << "Whole URL: " << original_url << endl << "Port: " << port << endl << "Path: " << path << endl << "Query: " << query << endl << "Fragment: " << fragment << endl;
}

