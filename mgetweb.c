/*
	Patrick Sheehan
	CSCE 313H - MP4
	mgetweb.c - multi-threaded download for a single file over HTTP

	Resources: cboard.cprogramming.com, lecture slides, pubs.opengroup.org
							http://shoe.bocks.com/net, beej.us guides, 
							linux.die.net/man, cplusplus.com, linux.about.com
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>


#define BUFFER_SIZE 4096
#define THREAD_COUNT 10

int data_socket, bytes_per_thread;//, total_received = 0;

char *url, *host_name, *file_path, *file_name;

void copySection(FILE * file)
{	// copy a section of the file being downloaded

	int index, num_bytes;
	char buffer[BUFFER_SIZE];

// try 'locking' around recv, need mutex lock, and need to restructure while loop

  while((num_bytes = recv(data_socket, buffer, sizeof(buffer), 0)))
	{
		// total_received += num_bytes;
		// printf("bytes received = %d\n", num_bytes);
		char *temp;
		temp = buffer;

   	fwrite (temp, num_bytes, 1, file);
	}
};

// given a full URL, strip it down to set host name, file path, and file name
int parseURL(char* url)
{
	int num_bytes, i, httpOffset;
	char *temp;
	
	// trim http or https from url
	if (strstr(url, "http://") != NULL) httpOffset = 7;
	else if (strstr(url, "https://") != NULL)	httpOffset = 8;
	else httpOffset = 0;

	temp = malloc(sizeof(url));
	if ((temp = strchr(url + httpOffset, '/')) != NULL)
	{	
		// set the name of the host
		num_bytes = temp - url - httpOffset;
		host_name = malloc(num_bytes);
		memcpy(host_name, url + httpOffset, num_bytes);
		
		// full file name in website's directory
		file_path = malloc(strlen(temp));
		memcpy(file_path, temp, strlen(temp));

		if ((temp = strrchr(url, '/')) != NULL)
		{	// set the name of the file being downloaded
			temp++;
			printf("temp = %s\n", temp);
			file_name = malloc(sizeof(temp));
			memcpy(file_name, temp, strlen(temp));
			printf("local file_name = %s\n", file_name);

		}
		else 
		{
			printf("Error occurred while parsing the URL\n");
			exit(0);
		}
		return 0;
	}
	
	printf("Error occurred while parsing the URL\n");
	exit(0);
	return -1;
};


int main(int argc, char* argv[])
{	// download a file from the internet using the URL provided as the argument

	int header_socket,
			num_bytes,
			bytes_in_file,
			bytes_in_header,
			i;

	char 	*get_request,
				*head_request,
				buffer[BUFFER_SIZE];

	struct addrinfo hints,
								 	*address;
	
	FILE *file, *timing;

	pthread_t threads[THREAD_COUNT];

	clock_t startTime, endTime;

	// check for correct number of arguments
	if (argc != 2)
	{
		printf("Please provide a URL as a command line argument\n");
		exit(0);

		// http://faculty.cse.tamu.edu/guofei/paper/PoisonAmplifier-RAID12.pdf
		// http://robots.cs.tamu.edu/dshell/cs420/quizes/quiz1a.pdf
	}
	else
	{
		// read the url from command line arg
		url = argv[1];
	}
	

	parseURL(url);
			printf("global file_name = %s\n", file_name);
	// create a HEAD request;
	head_request = malloc(strlen(file_path) + strlen(host_name) + 30);
	strcpy(head_request, "HEAD ");
	strcat(head_request, file_path);
	strcat(head_request, " HTTP/1.1\r\nHost: ");
	strcat(head_request, host_name);
	strcat(head_request, "\r\n\r\n");
	
	printf("\n~~~~~~~HEAD Request~~~~~~~\n%s", head_request);

	// assign values for the (addrinfo) socket address struct
	memset(&hints, 0, sizeof(hints));
	hints.ai_family 		= AF_INET;
	hints.ai_socktype 	= SOCK_STREAM;
	
	getaddrinfo(host_name, "80", &hints, &address);

	// create header socket and data socket
	header_socket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
	data_socket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);

	if (header_socket < 0 || data_socket < 0)
	{
		printf("An error occurred while trying to create a header socket\n");
  	return 0;
	}

	// connect() to the server with header_socket
	if (connect(header_socket, address->ai_addr, address->ai_addrlen) < 0) 
	{
  	printf("An error occurred while trying to connect()\n");
  	return 0;
  }

  // connect() to the server with data_socket
	if (connect(data_socket, address->ai_addr, address->ai_addrlen) < 0) 
	{
  	printf("An error occurred while trying to connect()\n");
  	return 0;
  }

  // get the URL's file name and create a blank copy
  // getFileName(file_name, url);

// TODO: file_name is messed up sometimes
  // printf("file_name = %s\n", file_name);
  // printf("file_path = %s\n", file_path);
  // printf("host_name = %s\n", host_name);

  // file = fopen(file_name, "a");
  file = fopen("downloaded_file", "a");

	timing = fopen("Timing Log - Multi.txt", "a"); 
 
  

	startTime = clock();

  // send HEAD request, store response in buffer
  send(header_socket, head_request, strlen(head_request), 0);
	bytes_in_header = recv(header_socket, buffer, sizeof(buffer), 0);
	// printf("bytes in header = %d\n", bytes_in_header);

	// get number of bytes from the header
	if (strstr(buffer, "Accept-Ranges: bytes") != NULL)
	{
		// display HEAD of http file
		printf("\n~~~~~~~HEAD Request~~~~~~~\n%s\n", buffer);

		// parse header for content length
		char* start = strstr(buffer, "Content-Length: ") + 16;
		char* end = strchr(start, '\n');
		int num = end - start;
		char* bytes = malloc(sizeof(start));
		memcpy(bytes, start, num);
		bytes_in_file = atoi(bytes);
	}
	else
	{
		printf("This file does not accept byte ranges\n");
		return 0;
	}

	bytes_per_thread = bytes_in_file / THREAD_COUNT + 1;
	// create a GET request string (GET <URL> HTTP)
	get_request = malloc(strlen(url) + 100);
	strcpy(get_request, "GET ");
	strcat(get_request, file_path);
	strcat(get_request, " HTTP/1.1\r\nHost: ");
	strcat(get_request, host_name);
	strcat(get_request, "\r\n\r\n");

	// send get request
	send(data_socket, get_request, strlen(get_request), 0);
	
	printf("\n~~~~~~~GET Request~~~~~~~\n%s\n", get_request);

	// create threads to currently accept data from socket
	for (i = 0; i < THREAD_COUNT; i++)
	{
		pthread_create(&threads[i], NULL, (void*)copySection, file);
	}

	for (i = 0; i < THREAD_COUNT; i++)
	{
		pthread_join(threads[i], NULL);
		printf("Thread #%d terminated\n", i);
	}

	endTime = clock();

	// printf("\ntotal bytes downloaded = %d\n", total_received);

	// close downloaded file and the socket
	close(header_socket);
	close(data_socket);
  
  int msec = (endTime - startTime) * 1000 / CLOCKS_PER_SEC;
	fprintf(timing, "RESULTS:\nthreads  : %d\nbytes    : %d\nseconds  : %d\nm-seconds: %d\n\n",
									THREAD_COUNT, bytes_in_file, msec/1000, msec%1000);

  fclose(file);
  fclose(timing);

  return 0;
}


