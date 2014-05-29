/*
	Patrick Sheehan
	CSCE 313H - MP4
	getweb.c - download a single file over HTTP using a URL

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
#include <time.h>

#define BUFFER_SIZE 4096

// given a full URL, strip it down to the host name (and set the host param)
int URLtoHostName(char* url, char* host, char* file)
{
	int num_bytes, i, httpOffset;
	char *temp;

	temp = malloc(sizeof(url));
	
	// trim http or https from url
	if (strstr(url, "http://") != NULL) httpOffset = 7;				//url += 7;
	else if (strstr(url, "https://") != NULL)	httpOffset = 8; //url += 8;
	else httpOffset = 0;

	url += httpOffset;

	if ((temp = strstr(url, "/")) != NULL)
	{	// trim away the URN (?): (all that is after the host name)
		num_bytes = temp - url;

		// full file name in directory
		memcpy(file, url + num_bytes, strlen(url) - num_bytes);

		// host name
		memcpy(host, url, num_bytes);
		host[num_bytes] = '\0';
	}
	url -= httpOffset;

	return 0;
}

int getFileName(char* file, char *url)
{
	char* temp;

	temp = malloc(sizeof(url));
	// set file to be the name of the file at the end of the url
	if ((temp = strrchr(url, '/')) != NULL)
	{
		temp++;
		strcpy(file, temp);
	}
 	else
 	{
 		printf("ERROR: getFileName() failed to recognize a file at the end of the URL\n");
 		exit(1);
 	}
	return 0;
}

int main(int argc, char* argv[])
{	// download a file from the internet using the URL provided as the argument

	int my_socket,
			num_bytes,
			bytes_in_file = 0;

	char * url, 
			 * get_request,
			 * file_name,
			 * file_path,
			 * host_name, 
			 buffer[BUFFER_SIZE];


	struct addrinfo hints, *address;

	struct hostent *host;

	FILE *file, *timing;

	clock_t startTime, endTime, total;


	// check for correct number of arguments
	if (argc != 2)
	{
		printf("Please provide a URL as a command line argument\n");
		exit(0);
	}
	
	// read the url from command line arg
	url = argv[1];
	host_name = malloc(sizeof(url));
	file_name = malloc(sizeof(url));
	URLtoHostName(url, host_name, file_name);

	// create a GET request string (GET <URL> HTTP/1.1)
	get_request = malloc(strlen(url) + 200);
	strcpy(get_request, "GET ");
	strcat(get_request, url);
	strcat(get_request, " HTTP/1.1\r\nHost: ");
	strcat(get_request, host_name);

	// strcat(get_request, "\r\nRange: bytes=0-");

	strcat(get_request, "\r\n\r\n");		

	printf("%s\n", get_request);


	// assign values for the (addrinfo) socket address struct
	memset(&hints, 0, sizeof(hints));
	hints.ai_family 		= AF_INET;
	hints.ai_socktype 	= SOCK_STREAM;
	
	getaddrinfo(host_name, "80", &hints, &address);

	// create a socket
	my_socket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
	if (my_socket < 0)
	{
		printf("An error occurred while trying to create a socket\n");
  	return 0;
	}

	// connect() to the server with my_socket
	if (connect(my_socket, address->ai_addr, address->ai_addrlen) < 0) 
	{
  	printf("An error occurred while trying to connect()\n");
  	return 0;
  }

  startTime = clock();

  send( my_socket, get_request, strlen(get_request), 0 );

  // get the URL's file name and create a blank copy
  getFileName(file_name, url);
  file = fopen(file_name, "a");
  timing = fopen("Timing Log - Single.txt", "a");

	while((num_bytes = recv(my_socket, buffer, sizeof(buffer), 0)))
	{
		bytes_in_file += num_bytes;
		// printf("bytes received = %d\n", num_bytes);
		char *p;
		p = buffer;

   	fwrite (p, num_bytes, 1, file);
	}

	endTime = clock();

	// printf("start = %lu\nend = %lu\n", startTime, endTime);
	// printf("CLOCKS_PER_SEC = %d\n", CLOCKS_PER_SEC);
	// close downloaded file and the socket
	close(my_socket);

	total = (double)(endTime - startTime) / (CLOCKS_PER_SEC / 1000);
	fprintf(timing, "RESULTS:\nbytes    : %d\nCPU ticks   : %lu\nmilliseconds  : %lu\n\n",
									bytes_in_file, (endTime - startTime), total);

  fclose(file);
  fclose(timing);

  return 0;
}


