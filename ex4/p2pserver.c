
#include "p2p.h"
//the socket of the server, known to all
int server_fd =0;
//the address and address length of the server
struct sockaddr_in address; 
int addrlen = sizeof(address); 
//counting ports to give to clients
int countPort = 1;
//all files that can be shared
file_ent_t **filesToShare;
//number of files that can be shared from server
int countFiles=0;
/******************************************************************************************************/

//FUNCTIONS
void setConnection();
void checkHostName(int hostname);
void checkHostEntry(struct hostent * hostentry);
void checkIPbuffer(char *IPbuffer);
void switchCase(int type, int socket,int portNumber);
void handleMSG_NOTIFY(int socket);
void clearBuffer(char buffer[]);
void sendMSG_ACK(int socket,int portNumber);
void handleMSG_DIRREQ(int socket);
void insertFileToShare( char name[P_NAME_LEN + 1],in_addr_t addr,in_port_t port);
void allocateFilesToShare();
void handleMSG_SHUTDOWN(int socket);
void freeFilesToShare();
msg_dirent_t buildMSG_DIRENT(file_ent_t file[]);
void acceptAndConnect();

/******************************************************************************************************/

//accepting a new client, and receiving messages from them
void acceptAndConnect()
{
  int new_socket = 0;
  if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
					(socklen_t*)&addrlen))<0) 
	{ 
		perror("accept"); 
		exit(EXIT_FAILURE); 
	} 
  
  while(1)
  {
    int msgType;
    int valread = recv( new_socket , &msgType, sizeof(msgType),0);
    if(valread <0)
    {
      perror("Server - recv message type\n");
    }
    switchCase(msgType,new_socket,0); 
  }
}
/******************************************************************************************************/
//creating a new MSG_DIRENT message
msg_dirent_t buildMSG_DIRENT(file_ent_t file[])
{
  msg_dirent_t msg;
  
  msg.m_type = MSG_DIRENT;
  strcpy(msg.m_name,file->fe_name);
  msg.m_addr = file->fe_addr;
  msg.m_port = file->fe_port;
 
  return msg;
}
/******************************************************************************************************/
//sending MSG_DIRENT message
void sendMSG_DIRENT(int socket)
{
  msg_dirent_t msg;
  
  msg.m_type = MSG_DIRENT;
  //first sending the type to the client
  if(send(socket , &msg.m_type ,sizeof(msg.m_type), 0 )<0)
  {
    perror("Server - send MSG_DIRENT type\n");
  }
  sleep(1);
  int i;
  for(i=0; i<countFiles;i++)
  {
    msg = buildMSG_DIRENT(filesToShare[i]);
    if(send(socket , &msg ,sizeof(msg), 0 )<0)
    {
      perror("Server - send MSG_DIRENT message\n");
    }
    sleep(1);
  }
  
}
/******************************************************************************************************/
//free allocation of filesToShare
void freeFilesToShare()
{
  int i;
  for(i=0;i<countFiles;i++)
  {

    free(filesToShare[i]);
  }
  
  //free(filesToShare);
  
}
/******************************************************************************************************/
//do shutdown
void handleMSG_SHUTDOWN(int socket)
{
  printf("Server shutting down\n");
 
  shutdown(server_fd,2);
  if(countFiles == 0)
  {
    exit(0);
  }

  freeFilesToShare();
  exit(0);
}
/******************************************************************************************************/
//allocate filestoshare
void allocateFilesToShare()
{
  filesToShare = (file_ent_t **)malloc(sizeof(file_ent_t *));
  if(filesToShare == NULL)
  {
    perror("cannot allocate\n");
    exit(1);
  }
}
/******************************************************************************************************/
//sending MSG_DIRHDR message 
void sendMSG_DIRHDR(int socket)
{
    msg_dirhdr_t msg;
    msg.m_type = MSG_DIRHDR;
    msg.m_count = countFiles;
    
   
   if(send(socket , &msg.m_type ,sizeof(msg.m_type), 0 )<0)
   {
     perror("Server - send MSG_DIRHDR type\n");
   }
   sleep(1);
   if(send(socket , &msg ,sizeof(msg_dirhdr_t), 0 )<0)
   {
     perror("Server - send MSG_DIRHDR message\n");
   }
   printf("Server - sending message of MSG_DIRHDR\n");
   
   //sending a MSG_DIRENT 
   switchCase(MSG_DIRENT,socket,0);

}
/******************************************************************************************************/
//inserting a file from client to files to share
void insertFileToShare( char name[P_NAME_LEN + 1],in_addr_t addr,in_port_t port)
{

  int portNumber = 0;
  char buffer[P_BUFF_SIZE] = {'\0'};
  
  filesToShare[countFiles]=(file_ent_t *)malloc(sizeof(file_ent_t ));
  if(filesToShare[countFiles] ==NULL)
  {
     perror("cannot allocate\n");
     exit(1);
  }
  
  strcpy(filesToShare[countFiles]->fe_name,name);
  filesToShare[countFiles]->fe_addr = addr;
  filesToShare[countFiles]->fe_port = port;
  countFiles++;
 
}
/******************************************************************************************************/
//calling MSG_DIRHDR after receiving MSG_DIRREQ
void handleMSG_DIRREQ(int socket)
{
  printf("Server - dirreq: receiving MSG_DIRREQ\n");
  
  switchCase(MSG_DIRHDR,socket,0);
  
}
/******************************************************************************************************/
//clearing buffer
void clearBuffer(char buffer[])
{
   int i;
  for(i=0;i<P_BUFF_SIZE;i++)
  {  
    buffer[i]='\0';
  }
}
/******************************************************************************************************/
//sending MSG_ACK to the client who sent MSG_NOTIFY
void sendMSG_ACK(int socket,int portNumber)
{
  msg_ack_t msg;
  msg.m_type = MSG_ACK;
  //if the port was 0, assign it a new port number
  if(portNumber == 0)
  {
     msg.m_port = P_SRV_PORT + countPort;
     printf("Server - notify: assigned port %d\n",(msg.m_port));
     countPort ++;
  }
  else{
    msg.m_port = portNumber;
  }
 
  if(send(socket , &msg.m_type ,sizeof(msg.m_type), 0 )<0)
  {
    perror("Server - send MSG_ACK type\n");
  }
   sleep(1);
  printf("Server - notify: sending MSG_ACK\n");
  if(send(socket , &msg ,sizeof(msg_ack_t), 0 ) <0)
  {
    perror("Server - send MSG_ACK message\n");
  }
  
	
}
/******************************************************************************************************/
//get the MSG_NOTIFY ans the file that needs to be shared 
void handleMSG_NOTIFY(int socket)
{  

  int valread;
  int portNumber = 0;
  char buffer[P_BUFF_SIZE] = {'\0'};
  
  msg_notify_t msg;
  printf("Server - notify:receiving MSG_NOTIFY\n");
  valread = recv( socket ,&msg, sizeof(msg_notify_t),0);
  if(valread <0)
  {
     perror("Server - recv message type\n");
  }
  if(msg.m_type != MSG_NOTIFY)
  {
    printf("the message type that was given:%d\n",msg.m_type);
  } 
  portNumber = (int)msg. m_port;
  if(portNumber == 0)
  {
     msg.m_port = P_SRV_PORT + countPort;
  }
  insertFileToShare(msg.m_name,msg.m_addr,msg. m_port);
 
  switchCase(MSG_ACK,socket,portNumber);

}
/******************************************************************************************************/
void switchCase(int type, int socket,int portNumber)
{
  switch(type)
  {  
    case MSG_NOTIFY:
      handleMSG_NOTIFY(socket);
      break;
    case MSG_ACK:
      sendMSG_ACK(socket,portNumber);
      break;
    case MSG_DIRREQ:
      handleMSG_DIRREQ(socket);
      break;
    case MSG_DIRHDR:
      sendMSG_DIRHDR(socket);
      break;
    case MSG_SHUTDOWN:
      handleMSG_SHUTDOWN(socket);
      break;
    case MSG_DIRENT:
      sendMSG_DIRENT(socket);
      break;
    case MSG_DUMP:
      shutdown(socket,2);
      acceptAndConnect();
      break;
    default:
       break;
  }
  
  return;
}

/******************************************************************************************************/
// Returns hostname for the local computer 
void checkHostName(int hostname) 
{ 
    if (hostname == -1) 
    { 
        perror("gethostname"); 
        exit(1); 
    } 
} 
/******************************************************************************************************/
// Returns host information corresponding to host name 
void checkHostEntry(struct hostent * hostentry) 
{ 
    if (hostentry == NULL) 
    { 
        perror("gethostbyname"); 
        exit(1); 
    } 
} 
/******************************************************************************************************/  
// Converts space-delimited IPv4 addresses 
// to dotted-decimal format 
void checkIPbuffer(char *IPbuffer) 
{ 
    if (NULL == IPbuffer) 
    { 
        perror("inet_ntoa"); 
        exit(1); 
    } 
} 

/******************************************************************************************************/
//function to set a connection for the server
void setConnection()
{   
	int opt = 1; 
	char buffer[P_BUFF_SIZE] = {0}; 
	
  char hostbuffer[256]; 
  char *IPbuffer; 
  struct hostent *host_entry; 
  int hostname; 
  
  // To retrieve hostname 
  hostname = gethostname(hostbuffer, sizeof(hostbuffer)); 
  checkHostName(hostname); 
  
  // To retrieve host information 
  host_entry = gethostbyname(hostbuffer); 
  checkHostEntry(host_entry); 
  
  // To convert an Internet network 
  // address into ASCII string 
  IPbuffer = inet_ntoa(*((struct in_addr*) 
                           host_entry->h_addr_list[0])); 
  
  printf("Server - server: opening socket on %s:%d\n", IPbuffer,P_SRV_PORT ); 

 
	// Creating socket file descriptor 
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
	{ 
		perror("socket failed"); 
		exit(EXIT_FAILURE); 
	} 
	
	// Forcefully attaching socket to the port 12345 
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
												&opt, sizeof(opt))) 
	{ 
		perror("setsockopt"); 
		exit(EXIT_FAILURE); 
	} 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons( P_SRV_PORT ); 
	
	// Forcefully attaching socket to the port P_SRV_PORT 
	if (bind(server_fd, (struct sockaddr *)&address, 
								sizeof(address))<0) 
	{ 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	} 
	if (listen(server_fd, 3) < 0) 
	{ 
		perror("listen"); 
		exit(EXIT_FAILURE); 
	} 
	
 acceptAndConnect();
 
  
}
/******************************************************************************************************/
int main(int argc, char const *argv[]) 
{ 

  allocateFilesToShare();
  
  setConnection();

	
 
	return 0; 
} 
