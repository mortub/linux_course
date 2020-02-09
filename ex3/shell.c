#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<sys/wait.h> 
#include <limits.h>

#define INPUT 0
#define OUTPUT 1
#define MAX_DATA 10240

//calling fork() without file in command
void callFork(char** arr, int flag_emp)
{
	pid_t pid;
	int status;
 	int i;
  
	if( ( pid = fork() ) == -1 ) 
	{
		perror( "fork() failed." );
		exit( 1 );
	}

	//the chils proccess
	if( pid == 0)
	{	
 
		if( execvp(arr[0],arr)==-1)
    {
      perror("can't execute \n");	
    }	
    		
	}
	if(flag_emp == 1){
   return;
 }
	//the parent process
	wait(&status);
  if (WIFSIGNALED(status)) 
  {
    printf("exit: %d\n", WTERMSIG(status));
  }
            
}

//fork() done and calling out exec with a file
void callForkWithFilename(char** arr,char token[], int flag, int flag_emp)
{
	int fd;
	pid_t pid;
	int status;
   //making sure for a case of without &
  char* tok= strtok(token,"\n");
  
	if( ( pid = fork() ) == -1 ) 
	{
		perror( "fork() failed." );
		exit( 1 );
	}
	
	//the chils proccess
	if( pid == 0)
	{	

		if(flag == INPUT)
		{
			fd = open(tok,O_RDONLY, 0777 );
			if( fd == -1 ) {
				perror(tok);
				exit(1);			
			}
			close(INPUT);
			dup2( fd, INPUT);
			close(fd);			

		}
		if(flag == OUTPUT)
		{			
			fd = open(tok, O_WRONLY | O_CREAT, 0777 );
			if( fd == -1 ) {
				perror(tok);
				exit(1);
			
			}	
			close(OUTPUT);			
			dup2( fd, STDOUT_FILENO);
			close(fd);
			
		}

		if( execvp(arr[0],arr)==-1)
    {
      perror("can't execute \n");	
    }
					
				
	}
	if(flag_emp == 1){
   	close(fd);
    return;
 }
	//the parent process
 	close(fd);
	wait(&status);
  if (WIFSIGNALED(status)) 
  {
    printf("exit: %d\n", WTERMSIG(status));
  }
                  
}

//copying the tokens into arr_copy_tokens
void copyingTokens(char** arr_copy_tokens,char data[],int len)
{
  int flag_emp =0;
	int i=0;
  char *token;	
  char* search_emp;
             
   //getting first token (command)
   token = strtok(data," ");
   
   //taking care of special
   if(strcmp(token,"cd")==0)
   {
     token = strtok(NULL,"\n");
     arr_copy_tokens[0]= (char*) malloc(strlen(token)*sizeof(char*)+1);
	   if(arr_copy_tokens[0] == NULL)
		  {
  			perror("failed malloc\n");
  		}
		  strcpy(arr_copy_tokens[0],token);
	  	arr_copy_tokens[0][strlen(token)]='\0';
      //changing directory
      if(chdir(arr_copy_tokens[0]) == -1)
      {
        perror("not a directory\n");
      }
      
      return;
   }

   while( token != NULL ) {  
    
		  arr_copy_tokens[i]= (char*) malloc(strlen(token)*sizeof(char*)+1);
		  if(arr_copy_tokens[i] == NULL)
		  {
  			perror("failed malloc\n");
  		}
		  strcpy(arr_copy_tokens[i],token);
	  	arr_copy_tokens[i][strlen(token)]='\0';
                                             
      search_emp = strstr(arr_copy_tokens[i],"&");
     
      //if we reached &, flag it, then put there NULL
      if(search_emp)
      {
      
        token = strtok(arr_copy_tokens[i],"\n");
        flag_emp =1;
        arr_copy_tokens[i]=NULL;
         
        callFork(arr_copy_tokens,flag_emp);
        return;
        
      }                                                      
   	  token = strtok(NULL," ");    
	  	i++;    		
  }

      token = strtok(arr_copy_tokens[i-1],"\n");
      strcpy(arr_copy_tokens[i-1],token);
	  	arr_copy_tokens[i-1][strlen(token)]='\0';
      //putting NULL at the end
			arr_copy_tokens[i]= (char*) malloc(0);
			if(arr_copy_tokens[i] == NULL)
			{
				perror("failed malloc\n");
			}
			arr_copy_tokens[i]=NULL;  
     callFork(arr_copy_tokens,flag_emp);
 
  
 
  

}

//copying the tokens into arr_copy_tokens
void copyingTokensWithFile(char** arr_copy_tokens,char data[],int len)
{
  //flags to see if we have > or <
  int flag_emp =0;
  int io;
	int i=0;
  char *token;	
  char* check_emp;
  
   //getting first token (command)
   token = strtok(data," ");

   while( token != NULL ) {
      
          
		//if we reached < , then the token after is the input file,
		// the arr_copy_tokens now end with NULL
		if(strcmp(token,"<") == 0 ||strcmp(token,">") == 0)
		{	
       if(strcmp(token,"<") == 0)
         io=INPUT;
       else{
         io=OUTPUT;
       }
      //getting the file name
			token = strtok(NULL," ");
      //cheking if ended with &
			check_emp= strtok(NULL," ");
      //didn't end with &
      if(check_emp == NULL)
      {
        flag_emp=0;
      }
      //if ended with &
      else if(check_emp != NULL)
      {
        if(strcmp(check_emp,"&") == 0)
          flag_emp=1;
      }
			//putting NULL at the end
			arr_copy_tokens[i]= (char*) malloc(0);
			if(arr_copy_tokens[i] == NULL)
			{
				perror("failed malloc\n");
			}
			arr_copy_tokens[i]=NULL;
			
			callForkWithFilename(arr_copy_tokens,token,io, flag_emp);	
		}

		  arr_copy_tokens[i]= (char*) malloc(strlen(token)*sizeof(char*)+1);
		  if(arr_copy_tokens[i] == NULL)
		  {
  			perror("failed malloc\n");
  		}
		  strcpy(arr_copy_tokens[i],token);
	  	arr_copy_tokens[i][strlen(token)]='\0';
		    
   	  token = strtok(NULL," ");
	  	i++;		
  }


}
//allocating an array of tokens according to number of tokens
void allocateArrTokens(char** arr_copy_tokens,int len,char data[])
{
   arr_copy_tokens=(char**)malloc(len*sizeof(char**)+1);
   if(arr_copy_tokens == NULL)
   {
	   perror("failed malloc\n");
   }
      
}

//writes a prompt, receives input
void prompt(char data[])
{
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
       printf("%s :", cwd);
  }
	printf("? ");
	fflush( 0 );
	//reading from keyboard
	if(read(0, data, MAX_DATA) < 0)
	{
		perror("could not read\n");
		exit(1);
	} 
	
}

int countArgs(char data[])
{
	int num_arg=0;
	char *token;
	//allocating data to data_s so data doesn't change
	char *data_s = (char*) malloc(sizeof(char*)*strlen(data)+1);
	if(data_s == NULL)
	{
		perror("failed malloc\n");
	}
	strcpy(data_s,data);
	token = strtok(data_s," ");
	while( token != NULL ) {
	 num_arg++;
     token = strtok(NULL," ");
   }
   free(data_s);
   
   return num_arg;
}

//the tokenizer separates data into tokens by " "
void tokenizer(char data[])
{	   

   int len= strlen(data);	
   //putting \0 at the end of string
   data[len+1]='\0';
   //check how many arguments are in data
   len = countArgs(data);
   char* arr_copy_tokens[len+1];
   //allocating an array of tokens according to number of tokens
   allocateArrTokens(arr_copy_tokens,len,data);
   char *search1;
   char *search2;
   //copying the tokens into arr_copy_tokens
   //only if there is > or < in data[]
   search1 = strstr(data,"<");
   search2 = strstr(data,">");
    if( search1||search2 )
    {
       copyingTokensWithFile(arr_copy_tokens,data,len);
    }
    else{
      copyingTokens(arr_copy_tokens,data,len);
    }
 
   
}
void run(int argc, char* argv[])
{
	//user not supposed to write any arguments here, only after prompt
	if(argc >1)
	{
		perror("too many arguments\n");
	}
   char* check_exit;
   char data[MAX_DATA];
   int stdin_copy ;
   int stdout_copy;
   
  while(1)
  {
  
    stdin_copy = dup(0);
    stdout_copy= dup(1);
     //writing a prompt to the user (my prompt is '?'),
	  //getting input from keyboard into data[]
  	prompt(data);	
    check_exit = strstr(data,"exit");
    if( check_exit)
    {
      printf("exiting program\n");
      exit(0);
    }			
  	//separates with token
  	tokenizer(data);
    dup2(stdin_copy, 0);
    dup2(stdout_copy, 1);
    close(stdin_copy);
    close(stdout_copy);
 }
 
   

	
}
int main (int argc, char* argv[])
{

 run(argc,argv);		
 
	return 0;
}