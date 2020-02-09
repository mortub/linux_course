#ifndef	_SCANTREE_H
#define	_SCANTREE_H

/*
 * function type that is called for each filename
 */
typedef	int	Myfunc( const char * );

/* 
 * function for scanning a directory tree
 */
int	   scantree( const char *fullpath, Myfunc *func );

#endif 
