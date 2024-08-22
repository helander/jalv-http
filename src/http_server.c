#include <stdio.h>  // console input/output, perror
#include <stdlib.h> // exit
#include <string.h> // string manipulation
#include <netdb.h>  // getnameinfo

#include <sys/socket.h> // socket APIs
#include <netinet/in.h> // sockaddr_in
#include <unistd.h>     // open, close

#include <signal.h> // signal handling
#include <time.h>   // time

#include <math.h>   // math




#define SIZE 1024  // buffer size

#define BACKLOG 10 // number of pending connections queue will hold

#include <pthread.h>
#include "port.h"
#include "jalv_internal.h"



static char *
booleanImage(bool b)
{
  if (b) return "true";
  return "false";
}



extern void *http_server_run(void* inst);

pthread_t t_http_server;

int http_server_start(Jalv *jalv) {
        if (jalv->opts.http_port == 0) {
        fprintf (stderr, "%s\n", " HTTP Server no port");
                return (1);
        }

        int k = pthread_create (&t_http_server, NULL, http_server_run, jalv);
        if (k != 0) {
        fprintf (stderr, "%d : %s\n", k, "pthread_create : HTTPServer thread");
                return (1);
        }
	return (0);
}

void http_server_stop(Jalv *jalv) {
   pthread_join (t_http_server, NULL);
}



void handleSignal(int signal);


int serverSocket;
int clientSocket;

char *request;









static void NoCommandResponse() {
      const char response[] = "HTTP/1.1 404 No such command\r\nAccess-Control-Allow-Origin: *\r\n\r\n";
      send(clientSocket, response, strlen(response), 0);
}


void *http_server_run(void* inst)
{
  Jalv *jalv = (Jalv *) inst;

  signal(SIGINT, handleSignal);

  struct sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;                     // IPv4
  serverAddress.sin_port = htons(jalv->opts.http_port);                   // port number in network byte order (host-to-network short)
  serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); // localhost (host to network long)

  serverSocket = socket(AF_INET, SOCK_STREAM, 0);

  setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

  if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
  {
    printf("Error: The server is not bound to the address.\n");
    return NULL;
  }

  if (listen(serverSocket, BACKLOG) < 0)
  {
    printf("Error: The server is not listening.\n");
    return NULL;
  }

  char hostBuffer[NI_MAXHOST], serviceBuffer[NI_MAXSERV];
  int error = getnameinfo((struct sockaddr *)&serverAddress, sizeof(serverAddress), hostBuffer,
                          sizeof(hostBuffer), serviceBuffer, sizeof(serviceBuffer), 0);

  if (error != 0)
  {
    printf("Error: %s\n", gai_strerror(error));
    return NULL;
  }

  printf("\nServer is listening on http://%s:%s/\n\n", hostBuffer, serviceBuffer);
  fflush(stdout);

  while (1)
  {
    request = (char *)malloc(SIZE * sizeof(char));
    char method[10], route[100];

    clientSocket = accept(serverSocket, NULL, NULL);
    read(clientSocket, request, SIZE);

    sscanf(request, "%s %s", method, route);

    free(request);
    //printf("\nMethod %s  Path %s\n",method,route);
    //fflush(stdout);


    if (strcmp(method, "GET") != 0)
    {
      const char response[] = "HTTP/1.1 400 Bad Request\r\nAccess-Control-Allow-Origin: *\r\n\r\n";
      send(clientSocket, response, strlen(response), 0);
    }
    else
    {

      char *token;


      token = strtok(route, "/");
      if (token == NULL) {
         NoCommandResponse();
      } else if (strcmp(token,"control") == 0) {
           char *sKey =NULL;
           token = strtok(NULL, "/");
           if (token != NULL) { sKey = token;  token = strtok(NULL, "/");}
           if (sKey != NULL) {
              char *sValue =NULL;
              //token = strtok(NULL, "/");
              if (token != NULL) { sValue = token;  token = strtok(NULL, "/");}
	      int index = atoi(sKey);
	      if (index < 0 || index > jalv->num_ports-1) {
                char buffer[100];
                sprintf(buffer,"HTTP/1.1 404 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\n");
                send(clientSocket, buffer, strlen(buffer), 0);
	      } else {
	       struct Port* const port = &jalv->ports[index];
               if (sValue != NULL) {
			port->control = atof(sValue);
               }
                char buffer[500],value[30];
                buffer[0] = 0;
                strcat(buffer,"HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\n");
		sprintf(value,"%f",port->control);strcat(buffer,value);
                send(clientSocket, buffer, strlen(buffer), 0);
               
              }
           } else {
		 char buffer[5000];
                 buffer[0] = 0;
                 strcat(buffer,"HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\n");

		strcat(buffer,"[");
  		for (size_t i = 0; i < jalv->controls.n_controls; ++i) {
			if (i != 0) strcat(buffer,",");
    			ControlID* const control = jalv->controls.controls[i];
    			struct Port* const port = &jalv->ports[control->index];
			strcat(buffer,"{");
			char record[100];
    			sprintf(record,"\"index\": %d,",control->index);strcat(buffer,record);
    			sprintf(record,"\"symbol\": \"%s\",",lilv_node_as_string(control->symbol));strcat(buffer,record);
			if (control->label != NULL) {
	    			sprintf(record,"\"label\": \"%s\",",lilv_node_as_string(control->label));strcat(buffer,record);
			}
			if (control->group != NULL) {
	    			sprintf(record,"\"group\": \"%s\",",lilv_node_as_string(control->group));strcat(buffer,record);
			}
    			sprintf(record,"\"writable\": %s,",booleanImage(control->is_writable));strcat(buffer,record);
    			sprintf(record,"\"readable\": %s,",booleanImage(control->is_readable));strcat(buffer,record);
    			sprintf(record,"\"toggle\": %s,",booleanImage(control->is_toggle));strcat(buffer,record);
    			sprintf(record,"\"integer\": %s,",booleanImage(control->is_integer));strcat(buffer,record);
    			sprintf(record,"\"enumeration\": %s,",booleanImage(control->is_enumeration));strcat(buffer,record);
    			sprintf(record,"\"logarithmic\": %s,",booleanImage(control->is_logarithmic));strcat(buffer,record);
			if (lilv_node_is_float(control->min) || lilv_node_is_int(control->min)) {
	    			sprintf(record,"\"min\": %f,",lilv_node_as_float(control->min));strcat(buffer,record);
			}
			if (lilv_node_is_float(control->max) || lilv_node_is_int(control->max)) {
	    			sprintf(record,"\"max\": %f,",lilv_node_as_float(control->max));strcat(buffer,record);
			}
			if (lilv_node_is_float(control->def) || lilv_node_is_int(control->def)) {
	    			sprintf(record,"\"default\": %f,",lilv_node_as_float(control->def));strcat(buffer,record);
			}
    			sprintf(record,"\"value\": %f",port->control);strcat(buffer,record);
			strcat(buffer,"}");
  		}
		strcat(buffer,"]");


                 send(clientSocket, buffer, strlen(buffer), 0);
           }
      } else {
         NoCommandResponse();
      } 

    }
    close(clientSocket);
    printf("\n");
  }
  return NULL;
}


void handleSignal(int signal)
{
  if (signal == SIGINT)
  {
    printf("\nShutting down server...\n");

    close(clientSocket);
    close(serverSocket);

    if (request != NULL)
      free(request);

    exit(0);
  }
}
