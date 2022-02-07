#include "common.h"
/** 
 * Sets up server and handles incoming connections
 * @param port Server port
 */
void server(int port)
{  
  int sock = create_socket(port);
  struct sockaddr_in client_address;
  int len = sizeof(client_address);
  int connection, pid, bytes_read;

  while(1){
    connection = accept(sock, (struct sockaddr*) &client_address,&len);
    char buffer[BSIZE];
    Command *cmd = malloc(sizeof(Command));
    State *state = malloc(sizeof(State));
    state->sock_pasv = -1;
	  state->username = NULL; 
	  state->message = NULL;
	  state->connection = -1;
    pid = fork();
    
    memset(buffer,0,BSIZE);

    if(pid<0){
      fprintf(stderr, "Cannot create child process.");
      exit(EXIT_FAILURE);
    }

    if(pid==0){
      close(sock);
      srand(getpid());
      char welcome[BSIZE] = "220 ";
      if(strlen(welcome_message)<BSIZE-4){
        strcat(welcome,welcome_message);
      }else{
        strcat(welcome, "Welcome to nice FTP service.");
      }

      /* Write welcome message */
      strcat(welcome,"\n");
      write(connection, welcome,strlen(welcome));
      state->connection = connection;

      /* Read commands from client */
      while (bytes_read = read(connection,buffer,BSIZE)){
        
        signal(SIGCHLD,my_wait);

        if(!(bytes_read>BSIZE)){
          /* TODO: output this to log */
          buffer[BSIZE-1] = '\0';
          printf("User %s sent command: %s\n",(state->username==0)?"unknown":state->username,buffer);
          parse_command(buffer,cmd);
          //state->connection = connection;
          /* Ignore non-ascii char. Ignores telnet command */
          if(buffer[0]<=127 || buffer[0]>=0){
            response(cmd,state);
          }
          memset(buffer,0,BSIZE);
          memset(cmd,0,sizeof(cmd));
        }else{
          /* Read error */
          perror("server:read");
        }
      }
      printf("Client disconnected.\n");
	    free(cmd);
      if(state->username)
         free(state->username);
	    free(state);
      exit(0);
    }else{
      //printf("closing... :(\n");
      close(connection);
	    free(cmd);
      //free(state->username);
	    free(state);
    }
  }
}

/**
 * Creates socket on specified port and starts listening to this socket
 * @param port Listen on this port
 * @return int File descriptor for new socket
 */
int create_socket(int port)
{
  int sock;
  int reuse = 1;

  /* Server addess */
  /*struct sockaddr_in server_address = (struct sockaddr_in){  
     AF_INET,
     htons(port),
     (struct in_addr){INADDR_ANY}
  };*/
  struct sockaddr_in server_address; 
  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port =  htons(port);
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);  
  


  if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    fprintf(stderr, "Cannot open socket");
    exit(EXIT_FAILURE);
  }

  /* Address can be reused instantly after program exits */
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);
  setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof reuse);

  /* Bind socket to server address */
  if(bind(sock,(struct sockaddr*) &server_address, sizeof(server_address)) < 0){
    fprintf(stderr, "bind error to address, port %d\n", port);
    perror("Cannot bind socket to address");   
    //exit(EXIT_FAILURE);
    return(-1);
  }

  listen(sock,5);
  return sock;
}

/**
 * Accept connection from client
 * @param socket Server listens this
 * @return int File descriptor to accepted connection
 */
int accept_connection(int socket)
{
  int addrlen = 0;
  struct sockaddr_in client_address;
  addrlen = sizeof(client_address);
  return accept(socket,(struct sockaddr*) &client_address,&addrlen);
}

/**
 * Get ip where client connected to
 * @param sock Commander socket connection
 * @param ip Result ip array (length must be 4 or greater)
 * result IP array e.g. {127,0,0,1}
 */
void getip(int sock, int *ip)
{
  socklen_t addr_size = sizeof(struct sockaddr_in);
  struct sockaddr_in addr;
  char host[INET_ADDRSTRLEN];
  getsockname(sock, (struct sockaddr *)&addr, &addr_size);
  //char* host = inet_ntoa(addr.sin_addr);
  if (inet_ntop(AF_INET, &addr.sin_addr, host,
                    INET_ADDRSTRLEN) == NULL)
        printf("Couldn't convert client address to string\n");
  sscanf(host,"%d.%d.%d.%d",&ip[0],&ip[1],&ip[2],&ip[3]);
}


/**
 * General lookup for string arrays
 * It is suitable for smaller arrays, for bigger ones trie is better
 * data structure for instance.
 * @param needle String to lookup
 * @param haystack Strign array
 * @param count Size of haystack
 */
int lookup(char *needle, const char **haystack, int count)
{
  int i;
  for(i=0;i<count; i++){
    if(strcmp(needle,haystack[i])==0)return i;
  }
  return -1;
}

/**
 * Lookup enum value of string
 * @param cmd Command string 
 * @return Enum index if command found otherwise -1
 */

int lookup_cmd(char *cmd){
  const int cmdlist_count = sizeof(cmdlist_str)/sizeof(char *);
  return lookup(cmd, cmdlist_str, cmdlist_count);
}




/** 
 * Writes current state to client
 */
void write_state(State *state)
{
  ssize_t len = write(state->connection, state->message, strlen(state->message));
  if (len <= 0){
    perror("sending data error:");
  }

}

/**
 * Generate random port for passive mode
 * @param state Client state
 */
void gen_port(Port *port)
{
  //srand(time(NULL));
  //srand(getpid());
  port->p1 = 128 + (rand() % 64);
  port->p2 = rand() % 0xff;

}

/**
 * Parses FTP command string into struct
 * @param cmdstring Command string (from ftp client)
 * @param cmd Command struct
 */
void parse_command(char *cmdstring, Command *cmd)
{
  sscanf(cmdstring,"%s %s",cmd->command,cmd->arg);
}

#ifndef __APPLE__
void set_res_limits()
{
	// Set MAX FD's to 100000
	struct rlimit res;
	res.rlim_cur = 1000000;
	res.rlim_max = 1000000;
	if( setrlimit(RLIMIT_NOFILE, &res) == -1 )
	{
		perror("Resource FD limit");
		exit(0);
	}
	printf("FD limit set to 1000000\n");	
	if( setrlimit(RLIMIT_RTPRIO, &res) == -1 )
	{
		perror("Resource Prioiry limit");
		exit(0);
	}
	printf("Prioirty limit set to 100\n");
}
#endif

/**
 * Handles zombies
 * @param signum Signal number
 */
void my_wait(int signum)
{
  int status;
  wait(&status);
}

main()
{
  //set_res_limits();
  server(21);
  return 0;

}
