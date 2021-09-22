# Web-Client
This repository is dedicated to the web client assignment for CSCE463: Networks and Distributed Processing

This C++ web client reads in a thread count variable and a url filename from the command line and spawns a specified number of threads to retrieve data from each url in the input file. In this iteration, all that is printed from the command line is a statistics threads that shows the download rate, remaining urls, number of links pulled, and more.

The current executable can be found in /x64/Release using the command template ./463-sample.exe <num_threads> <input_filename>. Example input files are included.
