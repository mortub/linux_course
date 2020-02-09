#include "p2p.h"

//defining a struct for files to be notified after copying
typedef struct File 
{
    char* filename;
    int toBeNotified;
}
file;

//the names of the files in the current dir
 char** files;
 //number of files to be allocated
int countFiles=0;
//the client's port number
int myPort=0;
//the sock of the client 
int sock=0;
//the number of files to share given from the server
int numOfFilesFromServer= 0;
//the files the peer wants, specified in leech, get from argv
file** filesToGet;
int numOfFilesToGet=0;
//a flag saying if the programm needs to call sento with shutdown or not
int isShutdown = 0;
int server_fd =0;
//the address and address length of the peer
struct sockaddr_in address; 
int addrlen = sizeof(address); 
//so that in shutdown we will not try to connect to a closed port
int trackPortInShutdown = 0;
 
/******************************************************************************************************/

//FUNCTIONS
void switchCase(int type,char* filename,int new_socket);
void sendMSG_NOTIFY(char * filename);
void setConnection();
msg_notify_t buildMSG_NOTIFY(char* filename);
void getMSG_ACK();
void getFilesInDir();
void allocateFilesArray();
void freeFiles();
void sendMSG_DIRREQ();
void shutdownFunc();
void checkFilesExistInDir(char* argv[],int argc);
int checkFileExistsInDir(char argv[]);
void allocateFilesToGet(char* argv[],int argc);
void freeFilesToGet();
int compareToFilesToGet(char name[]);
msg_filereq_t buildMSG_FILEREQ(char name[]);
msg_filesrv_t buildMSG_FILESRV(char name[]);
long int findSize(char file_name[]);
void checkHostName(int hostname);
void checkHostEntry(struct hostent * hostentry);
void checkIPbuffer(char *IPbuffer);
void connectAsServer();
void sendMSG_SHUTDOWN(msg_dirent_t msg);
void sendMSG_FILEREQ(msg_dirent_t msg);
void getMSG_FILEREQ(int new_socket);
void getMSG_FILESRV(char* filename,int new_socket);
void copyFileFromPeer(char* filename,int new_socket,int size);
void acceptAndConnect();
void setToBeNotifiedInFilesToGet(char* filename);
void notifyFiles();
void run(int argc, char* argv[]);

/******************************************************************************************************/
//notifying to server about copied files after leech
void notifyFiles()
{
   int i;
  for(i=0; i<numOfFilesToGet;i++)
  {
    if(filesToGet[i]->toBeNotified == 1)
    {
       switchCase(MSG_NOTIFY,filesToGet[i]->filename,0);
       sleep(1);
    }
  }
   switchCase(MSG_DUMP,NULL,0);
}

/******************************************************************************************************/
//setting toBeNotified=1 so later client will notify to server he can share that file
void setToBeNotifiedInFilesToGet(char* filename)
{
  int i;
  for(i=0; i<numOfFilesToGet;i++)
  {
    if(strcmp(filename, filesToGet[i]->filename) == 0)
    {
      filesToGet[i]->toBeNotified =1;
    }
  }
}

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
  

    int msgType;
    int valread = recv( new_socket , &msgType, sizeof(msgType),0);
    if(valread <0)
    {
      perror("Server - recv message type\n");
    }
    switchCase(msgType,NULL,new_socket); 
}
/******************************************************************************************************/

//copying the file shared from peer to a new file in current directory
void copyFileFromPeer(char* filename,int new_socket,int size)
{
  int fd = open( filename, O_RDWR | O_CREAT, 0666 );
  if(fd == -1)
  {
    perror("Peer - cannot open file to read from another peer\n");
    return;
  }
  
  int i,valread;
  char c;
  for(i=0; i<size; i++)
  {
    valread = recv(new_socket,&c,sizeof(char),0);
    if(valread < 0)
    {
      perror("Peer - recv char of file to share from peer\n");
    }
    write(fd,&c,sizeof(char));
  }
  printf("Peer - finished copying file from peer\n");
  close(fd);
  
  //now in the filesToGet we know we need to set toBeNotified = 1; to the filename
  setToBeNotifiedInFilesToGet(filename);
  shutdown(new_socket,2);

}
/******************************************************************************************************/
//get MSG_FILESRV message, to know the size of file requested
void getMSG_FILESRV(char* filename,int new_socket)
{
  msg_filesrv_t msg;
  int valread = recv(new_socket, &msg, sizeof(msg_filesrv_t),0);
  if(valread < 0)
  {
    perror("Peer - recv MSG_FILESRV message\n");
  }
  if(msg.m_file_size < 0)
  {
    printf("Peer - the peer didn't share the file: %s\n",filename);
    shutdown(new_socket,2);
  }

  else{
    copyFileFromPeer(filename,new_socket,msg.m_file_size);
  }
 
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
//make a connection as a server (peer)
void connectAsServer()
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
  
  printf("Peer -  opening socket on %s:%d\n", IPbuffer,myPort ); 

 
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
	address.sin_port = htons(myPort); 

	// Forcefully attaching socket to the port given from the server
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
//sending MSG_DUMP
void  sendMSG_DUMP()
{
  int type = MSG_DUMP;
  if(send(sock , &type ,sizeof(MSG_DUMP), 0 )<0)
  {
    perror("Client - send MSG_DUMP type\n");
  }
}
/******************************************************************************************************/
//returns the size of file to be shared
long int findSize(char file_name[]) 
{ 
    // opening the file in read mode 
    FILE* fp = fopen(file_name, "r"); 
  
    // checking if the file exist or not 
    if (fp == NULL) { 
        printf("File Not Found!\n"); 
        return -1; 
    } 
  
    fseek(fp, 0L, SEEK_END); 
  
    // calculating the size of the file 
    long int res = ftell(fp); 
  
    // closing the file 
    fclose(fp); 
  
    return res; 
} 
/******************************************************************************************************/
//creating the MSG_FILESRV message, if file to be shared doesn't exist, size= -1
msg_filesrv_t buildMSG_FILESRV(char name[])
{
  msg_filesrv_t msg;
  msg.m_type = MSG_FILESRV;
  int answer =checkFileExistsInDir(name);
  //it means the file requested doesn't exist in the current directory
  if(answer == 0)
  {
    msg.m_file_size = -1;
  }
  else{
    msg.m_file_size = (int)findSize(name);
  }
  
  return msg;
}
/******************************************************************************************************/
//gets the MSG_FILEREQ, then sends MSG_FILESRV and gets the file if size > -1
void getMSG_FILEREQ(int new_socket)
{
  msg_filereq_t msg;
  int valread = recv(new_socket, &msg, sizeof(msg_filereq_t),0);
  if( valread < 0)
  {
    perror("Peer - recv MSG_FILEREQ message\n");
  }
  printf("the filename received from peer: %s\n",msg.m_name);
  
  msg_filesrv_t filesrvMsg = buildMSG_FILESRV(msg.m_name);
  
  //send the filesrv to the client who requested it
  if(send(new_socket, &filesrvMsg.m_type,sizeof(filesrvMsg.m_type),0)<0)
  {
    perror("Peer - send MSG_FILESRV type\n");
  }
  printf("Peer - sending MSG_FILESRV\n");
  if(send(new_socket, &filesrvMsg,sizeof(msg_filesrv_t),0)<0)
  {
    perror("Peer - send MSG_FILESRV message\n");
  }
  
  if(filesrvMsg.m_file_size < 0)
  {
    printf("Peer - peer cannot share the file: %s\n",msg.m_name);
    shutdown(new_socket,2);
    //need to do another accept() for future peers
    acceptAndConnect();
    return;
  }
  //sending the file content to the requesting peer
 	char c; 
  int rn;
  int fd = open(msg.m_name, O_RDWR);
  if(fd == -1)
  {
    perror("Peer - cannot open file to read from another peer\n");
    return;
  }
  
  while((rn =read(fd,&c,1)) != 0)
  {
     if(rn == -1)
     {
       //problems reading from file
       perror(msg.m_name);
       break;
     }
     if(send(new_socket, &c,sizeof(char),0)<0)
     {
       perror("Peer - send char from file to share\n");
     }
     sleep(1);
  }
  
  printf("Peer - finished sharing file with peer\n");
  close(fd);
  
  //need to do another accept() for future peers
  acceptAndConnect();
  
}
/******************************************************************************************************/
//creating MSG_FILEREQ message
msg_filereq_t buildMSG_FILEREQ(char name[])
{
   msg_filereq_t msg;
   msg.m_type = MSG_FILEREQ;
   strcpy(msg.m_name, name);
   
   return msg;
}
/******************************************************************************************************/
//compares the name to the files mentioned in leech
int compareToFilesToGet(char name[])
{
  int i;
  for(i=0;i<numOfFilesToGet;i++)
  {
    if(strcmp(name,filesToGet[i]->filename)==0)
    {
      return 1;
    }
  }
  return 0;
}
/******************************************************************************************************/
//free files to get
void freeFilesToGet()
{
  int i;
  for(i=0;i<numOfFilesToGet;i++)
  {
    free(filesToGet[i]->filename);
    free(filesToGet[i]);
  }
 free(filesToGet);
}
/******************************************************************************************************/
//allocating filestoget array
void allocateFilesToGet(char* argv[],int argc)
{
  filesToGet = (file**)malloc(sizeof(file*));
  if(filesToGet == NULL)
  {
     perror("cannot allocate\n");
     exit(1);
  }
  int i,j=0;
  //skipping exe filename and 'leech'
  for(i=2; i<argc;i++)
  {
    filesToGet[j]=(file*)malloc(sizeof(file));
    if(filesToGet[j] == NULL)
    {
       perror("cannot allocate\n");
       exit(1);
    }
    filesToGet[j]->filename=(char*)malloc(sizeof(char));
    if(filesToGet[j]->filename == NULL)
    {
       perror("cannot allocate\n");
       exit(1);
    }
    strcpy(filesToGet[j]->filename,argv[i]);
    filesToGet[j]->toBeNotified = 0;
    j++;
  }
  numOfFilesToGet = argc -2;

}
/******************************************************************************************************/
//sending shutdown to other peers
void sendMSG_SHUTDOWN(msg_dirent_t msg)
{
    if(trackPortInShutdown == msg.m_port)
    {
      return;
    }
    trackPortInShutdown = msg.m_port;

    struct sockaddr_in addressPeer;
    msg_shutdown_t msgShutdown;
    msgShutdown.m_type = MSG_SHUTDOWN;     
    int newSocket=0;
      char buffer[P_BUFF_SIZE] = {'\0'};

  	// Creating socket file descriptor 
  	if ((newSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
	  { 
		  perror("socket failed"); 
		  exit(EXIT_FAILURE); 
	  } 
      addressPeer.sin_family = AF_INET; 
      addressPeer.sin_port = htons(msg.m_port);
      
  	// Convert IPv4 and IPv6 addresses from text to binary form 
	  if(inet_pton(AF_INET, "127.0.0.1", &addressPeer.sin_addr)<=0) 
	  { 
 	  	printf("\nInvalid address/ Address not supported \n"); 
	  	exit(1); 
  	} 
   
    
  	if (connect(newSocket, (struct sockaddr *)&addressPeer, sizeof(addressPeer)) < 0) 
	  { 
		  printf("\nConnection Failed \n"); 
	  	exit(1); 
	  } 
     
     printf("Peer - sending a shutdown message to peer\n");
    if(send(newSocket, &msgShutdown.m_type,sizeof(msgShutdown.m_type),0)<0)
    {
      perror("Peer - send shutdown to another peer\n");
    }
    shutdown(newSocket,2);

}
/******************************************************************************************************/
//sending MSG_FILEREQ to the peer with the file
void sendMSG_FILEREQ(msg_dirent_t msg)
{
   msg_filereq_t fileReq = buildMSG_FILEREQ(msg.m_name);
   struct sockaddr_in addressPeer;        
   int newSocket=0;

  	// Creating socket file descriptor 
  	if ((newSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
	  { 
		  perror("socket failed"); 
		  exit(EXIT_FAILURE); 
	  } 
      addressPeer.sin_family = AF_INET; 
      addressPeer.sin_port = htons(msg.m_port);
      
  	// Convert IPv4 and IPv6 addresses from text to binary form 
	  if(inet_pton(AF_INET, "127.0.0.1", &addressPeer.sin_addr)<=0) 
	  { 
 	  	printf("\nInvalid address/ Address not supported \n"); 
	  	exit(1); 
  	} 
 
  	if (connect(newSocket, (struct sockaddr *)&addressPeer, sizeof(addressPeer)) < 0) 
	  { 
		  printf("\nConnection Failed \n"); 
	  	exit(1); 
	  } 

    if(send(newSocket, &fileReq.m_type,sizeof(fileReq.m_type),0)<0)
    {
      perror("Peer - send MSG_FILEREQ type\n");
    }
    sleep(1);
    printf("Peer - sending MSG_FILEREQ\n");
    if(send(newSocket, &fileReq,sizeof(fileReq),0)<0)
    {
      perror("Peer -  send MSG_FILEREQ message\n");
    }
    
    //receving from the other peer the file srv
    int msgType;
    int valread = recv( newSocket , &msgType, sizeof(msgType),0);
    if(valread <0)
    {
      perror("Peer - recv message type\n");
    }
    
    switchCase(msgType,msg.m_name,newSocket); 

   
}
/******************************************************************************************************/
//gets the filestoshare from server, checking if shutdown or that one in the list isa file the client wants in leech
void getMSG_DIRENT()
{
  msg_dirent_t msg;
  char buffer[P_BUFF_SIZE] = {'\0'};
  int i;
  for(i=0;i<numOfFilesFromServer;i++)
  {
    int valread = recv( sock , &msg, sizeof(msg_dirent_t),0);
    if(valread < 0)
    {
      perror("Client - recv MSG_DIRENT message\n");
    }
    int portNumber = (int)msg.m_port;

    
    //shutting down a client according to the file entity given from server
    if(isShutdown == 1)
     {
      sendMSG_SHUTDOWN(msg);      
     }
    else {
    
      //if one of the files the server can share are from the files I wanted (in leech),
      // send a request to the sharer
      if(compareToFilesToGet(msg.m_name) == 1)
      {        
       sendMSG_FILEREQ(msg);
       sleep(1);       
      }
    }
    
  }
  
  if(isShutdown == 0)
  {
     //send to server MSG_NOTIFY to all copied files
     notifyFiles();     
  }
  


}
/******************************************************************************************************/
//check if the name is a file in this dir
int checkFileExistsInDir(char argv[])
{
  int i;
  for(i=0;i<countFiles;i++)
  {
    if(strcmp(argv,files[i])==0)
    {
      return 1;
    }
   
  }
  return 0;
}
/******************************************************************************************************/
//check for all files in argv if they exist in die (from seed)
void checkFilesExistInDir(char* argv[],int argc)
{
  int i,answer;
  for(i=2;i<argc;i++)
  {
    answer = checkFileExistsInDir(argv[i]);
    //the file exists in this directory, then send a MSG_NOTIFY
    if(answer == 1)
    {
      switchCase(MSG_NOTIFY,argv[i],0);
    }
    
  }
  sleep(1);
  switchCase(MSG_DUMP,NULL,0);
}
/******************************************************************************************************/
//shutdown
void shutdownFunc()
{
  //asking for the list of files to share from the server
  isShutdown = 1;
  switchCase(MSG_DIRREQ,NULL,0);
  sleep(1);
  msg_shutdown_t msg;
  msg.m_type = MSG_SHUTDOWN;
  //sending a shutdown to the server
  if(send(sock , &msg.m_type ,sizeof(msg.m_type), 0 )<0)
  {
    perror("Client - send MSG_SHUTDOWN type\n");
  }
  switchCase(MSG_SHUTDOWN,NULL,0);
}
/******************************************************************************************************/
//gets MSG_DIRHDR from server
void getMSG_DIRHDR()
{
  msg_dirhdr_t msg;
  int valread = recv( sock , &msg, sizeof(msg_dirhdr_t),0);
  if(valread < 0)
  {
    perror("Client - recv MSG_DIRHDR message\n");
  }
  printf("Client - number of shared files from server : %d\n", msg.m_count);
  numOfFilesFromServer = msg.m_count;
  sleep(1);
  //receiving from the server the MSG_DIRENT
  int msgType;
  valread = recv( sock, &msgType, sizeof(msgType),0);
  if(valread < 0)
  {
    perror("Client - recv message type\n");
  }
  switchCase(msgType,NULL,0); 
}
/******************************************************************************************************/
//sending MSG_DIRREQ 
void sendMSG_DIRREQ()
{
  msg_dirreq_t msg;
  msg.m_type = MSG_DIRREQ;
  if(send(sock , &msg.m_type ,msg.m_type, 0 )<0)
  {
    perror("Client - send MSG_DIRREQ type\n");
  }
  
  int msgType;
  int valread = recv( sock , &msgType, sizeof(msgType),0);
  if(valread < 0)
  {
    perror("Client - recv message type\n");
  }
  switchCase(msgType,NULL,0); 
  
}
/******************************************************************************************************/
//free files array
void freeFiles()
{
  int i;
  for(i=0;i<countFiles;i++)
  {
    free(files[i]);
  }
  free(files);
}
/******************************************************************************************************/
//allocating files array
void allocateFilesArray()
{
  files = (char**)malloc(countFiles * sizeof(char*));
  if(files == NULL)
  {
    perror("cannot allocate\n");
    exit(1);
  }
  int i;
  for(i=0;i<countFiles;i++)
  {
    files[i] =(char*)malloc((P_NAME_LEN + 1)*sizeof(char));
    if(files[i] == NULL)
    {
      perror("cannot allocate\n");
      exit(1);
    }
   memset(files[i],'\0', P_NAME_LEN + 1);

  }
  
}
/******************************************************************************************************/
//get the files in this dir
void getFilesInDir()
{
  struct dirent *pDirent;
  DIR *pDir;
  char cwd[PATH_MAX];
  struct stat buf;
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    perror("getcwd() error");
     exit(1);
  }
  pDir = opendir (cwd);
  if (pDir == NULL) {
    printf ("Cannot open directory '%s'\n", cwd);
    exit(1);
  }

 while ((pDirent = readdir(pDir)) != NULL) {
     
      if(pDirent->d_type != DT_DIR) {
         countFiles++;
      }

 }
 int i=0;
 allocateFilesArray();
 rewinddir(pDir);
  while ((pDirent = readdir(pDir)) != NULL) {
     
      if(pDirent->d_type != DT_DIR) {
         strcpy(files[i], pDirent->d_name);
         i++;
      }

 }
   
 closedir (pDir);
 
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
//gets MSG_ACK from server
void getMSG_ACK()
{
  int valread;
  int portNumber = 0;
  msg_ack_t msg;
  valread = recv( sock , &msg,sizeof(msg_ack_t),0);
  if(valread < 0)
  {
    perror("Client - recv MSG_ACK message\n");
  }
  portNumber = (int)msg. m_port;
  myPort=portNumber;


}
/******************************************************************************************************/
//building the struct to be sent, with port == 0 
msg_notify_t buildMSG_NOTIFY(char* filename)
{
   msg_notify_t msg; 
  //setting all message fields
  msg.m_type = MSG_NOTIFY;

  
  strcpy(msg.m_name, filename);
 	if(inet_pton(AF_INET, "127.0.0.1", &msg.m_addr)<=0) 
	{ 
		printf("\nInvalid address/ Address not supported \n"); 
		exit(1); 
	} 
  msg.m_port = myPort;
   
  return msg;
}
/******************************************************************************************************/
//sending MSG_NOTIFY to server
void sendMSG_NOTIFY(char * filename)
{
 
   msg_notify_t msg = buildMSG_NOTIFY(filename);
   
   char buffer[P_BUFF_SIZE];
   
   if(send(sock , &msg.m_type ,msg.m_type, 0 )<0)
   {
     perror("Client - send MSG_NOTIFY type\n");
   } 

   sleep(1);
   if(send(sock , &msg ,sizeof( msg_notify_t), 0 )<0)
   {
     perror("Client - send MSG_NOTIFY message\n");
   }

   sleep(1);
  int msgType,valread;
  valread = recv( sock , &msgType, sizeof(msgType),0);
  if(valread < 0)
  {
    perror("Client - recv message type\n");
  }
  switchCase(msgType,NULL,0);

 
}

/******************************************************************************************************/
void switchCase(int type,char* filename,int new_socket)
{
  switch(type)
  {  
    case MSG_NOTIFY:
      sendMSG_NOTIFY(filename);
      break;
    case MSG_ACK:
      getMSG_ACK();
      break;
    case MSG_DIRREQ:
      sendMSG_DIRREQ();
      break;
    case MSG_DIRHDR:
      getMSG_DIRHDR();
      break;
    case MSG_SHUTDOWN:
      printf("Peer - shutting down\n");
      shutdown(sock,2);
      shutdown(server_fd,2);
      exit(0);
      break;
    case MSG_DIRENT:
      getMSG_DIRENT();
      break;
    case MSG_FILEREQ:
      getMSG_FILEREQ(new_socket);
      break;
    case MSG_DUMP:
      sendMSG_DUMP();
      break;
    case MSG_FILESRV:
      getMSG_FILESRV(filename,new_socket);
    default:
       return;
  }
  
  return;
}

/******************************************************************************************************/
//function to set a connection with the server
void setConnection()
{
	struct sockaddr_in serv_addr;  
	char buffer[P_BUFF_SIZE] = {0}; 
 
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{ 
		printf("\n sock creation error \n"); 
		exit(1);  
	} 

	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(P_SRV_PORT); 
	
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
	{ 
		printf("\nInvalid address/ Address not supported \n"); 
		exit(1); 
	} 

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{ 
		printf("\nConnection Failed \n"); 
		exit(1); 
	} 

 
}
/******************************************************************************************************/
//doing the main
void run(int argc, char* argv[])
{
  getFilesInDir();
	setConnection(); 

  if(argc <2){
    printf("Too few arguments\n");
    exit(1);
  }

  if(strcmp(argv[1],"seed") == 0)
  { 
    if(argc < 3)
    {
      printf("Too few arguments for seed\n");
      exit(1);
    }
    printf("seed\n");
    checkFilesExistInDir(argv,argc);
    connectAsServer();
  }
  
  if(strcmp(argv[1],"leech") == 0)
  { 
    if(argc < 3)
    {
      printf("Too few arguments for leech\n");
      exit(1);
    }
    printf("leech\n");
    allocateFilesToGet(argv, argc);
    switchCase(MSG_DIRREQ,NULL,0);
    connectAsServer();
    
  }
   int i;
  for(i=1;i< argc; i++)
  {
    if(strcmp(argv[i],"shutdown") == 0)
    {
      printf("shutdown\n");
      shutdownFunc();
    }
  }


 if(numOfFilesToGet > 0)
 {
   freeFilesToGet();
 }
 freeFiles();
}
int main(int argc, char* argv[]) 
{ 

  run(argc,argv);
  return 0; 
} 

