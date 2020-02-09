#include "scantree.h"
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define NAME 1000

//checks if the path is a directory or a file/link
//if directory- returns 0, else returns 1
int is_regular_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
	lstat (path, &path_stat);
    if (S_ISLNK(path_stat.st_mode)) 
	{
		return 1;
	}	
    return S_ISREG(path_stat.st_mode);
}

//TODO: fix problem: go with a loop when reading from directory, ignore . and ..,
//recursive function to open a tree of directories
int scantree( const char *path, Myfunc *func )
{
	DIR *dir;
	struct dirent *dp;
	char buf[NAME],fullPath[(NAME*3) +1];
	
	//checking if the path is a file or a directory
	int res = is_regular_file(path);
	printf("%s\n",path);
	//if it is a file- just call func
	if(res == 1)
	{	
		(func)(path);		
	} 
	//if it is a directory- call func+recursion
	if(res == 0)
	{	
		//opening the directory 	
		if ((dir = opendir(path)) == NULL) {
			perror (path);
			exit (1);
		}

		
		//check if the path is . or .., and if so- continue reading, don't call func and don't do the recursion
		while(dp = readdir(dir))
		{
			//printf("%s\n",fullPath);
			if(strcmp(dp->d_name, ".") == 0||strcmp(dp->d_name, "..") == 0)
			{
				continue;
			}
			
			//concatenate to absolute path
			strcpy(fullPath,path);
			strcat(fullPath,"/");
			strcat(fullPath,dp->d_name);
			//call func on the name we have
			(func)(fullPath);			
			//recursive call to scantree with the next
			scantree(fullPath,func);
					
		}
				
		//closedir(dir);		
	}

	
	return 0;
}

