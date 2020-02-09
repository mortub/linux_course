#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <string.h>


#define OUT    0 
#define IN    1 
#define MAXLENGTH 100

//printing the outcome
void print (char* file,off_t* numLines,off_t* numWords,off_t*numChars )
{
	printf("%ld ", (*numLines));
	printf("%ld ", (*numWords));	
	printf("%ld ", (*numChars));
	printf("%s\n", file);
	
} 
void wcFromStdin(off_t* lines,off_t* words,off_t* chars)
{
	int i=0;
	char str[MAXLENGTH];
	// read from standard input 
		int state=OUT;
		fgets (str,MAXLENGTH, stdin);
		for(i=0;i<strlen(str);i++)
		{ 
			int result;
			result = isspace(str[i]);
			// if it is a space character, check if there comes a char that isn't space
			if(result != 0)
			{
				state = OUT; 
			}
			if(state == OUT)
			{
				// if we came to a non-space char after a space, it's a word
				if((result = isspace(str[i])) == 0)
				{
				state = IN;  
				++(*words);
				}
			
			}		
		
		}
		(*lines)=1;
		(*chars)=strlen(str);
		
		print ("stdin",lines,words,chars );
}

//checking if the string given is a directory
int check_if_directory(char* file,off_t* numLines,off_t* numWords,off_t*numChars  )
{
	//checking if the input is a directory
	DIR* pDir;
	pDir = opendir (file);
	//it means we have a directory and that we opened it
        if (pDir != NULL) 
		{
			printf("WC: %s: Is a directory\n", file);
			print(file,numLines,numWords,numChars);  
			return 1;          
        }
		return 0;
}


//doing the command wc
void wc(char* file,off_t* numLines,off_t* numWords,off_t*numChars )
{	
	int state = OUT; 	
	int number_of_read;
	//defining a buffer to insert information from file
	char buffer;
	 if((check_if_directory(file,numLines,numWords,numChars))== 1)
		 return;
	//opening from file to read 
	int file_input=open(file, O_RDONLY);
	//if it could not be opened, write a message and exit the program
	if(file_input < 0)
	{
		perror(file);
		exit(1);
	}
	
	//putting the cursor at the beginning of the file
	*numLines =lseek(file_input, 0, SEEK_SET);
	//finding the number of lines
	while((number_of_read=read(file_input, &buffer, 1))>0)
	{
		if(number_of_read == -1)
		{
			perror(file);
			exit(1);
		}
		
		if(buffer == '\n')
		{
			++(*numLines);
		}
		int result;
		result = isspace(buffer);
		//if it is a space character, check if there comes a char that isn't space
		if(result != 0)
		{
			state = OUT; 
		}
		if(state == OUT)
		{
			//if we came to a non-space char after a space, it's a word
			if((result = isspace(buffer)) == 0)
			{
				state = IN;  
				++(*numWords);
			}
			
		}		
		
	}
	
	//finding the number of characters
	(*numChars)= lseek(file_input, 0, SEEK_END);
	print(file,numLines,numWords,numChars); 
	close(file_input);
	
}

int main (int argc, char* argv[])
{
	int i;
	off_t lines=0,words=0,chars=0,totalLines=0,totalWords=0,totalChars=0;
	char str[100];
	//in case the user didn't give any paramenters
	if(argc == 1)
	{
		wcFromStdin(&lines,&words,&chars);
	}
		
	for(i=1;i<argc;i++)
	{
		wc(argv[i],&lines,&words,&chars);
		totalLines+=lines;
		totalWords+=words;
		totalChars+=chars;
	}
	if(argc > 2)
	{
		printf("%ld %ld %ld total",totalLines,totalWords,totalChars );
	}
	
	
	return 0;
}