//
//	Fileio.cpp
//
#include "Miscellaneous/Fileio.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#if 0
#ifdef WINDOWS
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <sys/file.h>
#include <sys/mman.h>
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

static char gApplicationDirectory[ FILENAME_MAX ];	// needs to be CWD
static bool gWasInitialized = false;

/*
 =================================
 Initialize
 Grabs the application's directory
 =================================
 */
static void Initialize() {
	if ( gWasInitialized ) {
		return;
	}
	gWasInitialized = true;
	
	bool result = GetCurrentDir( gApplicationDirectory, sizeof( gApplicationDirectory ) );
	assert( result );
	if ( result ) {
		printf( "ApplicationDirectory: %s\n", gApplicationDirectory );
	} else {
		printf( "ERROR: Unable to get current working directory!\n");
	}
}

bool RelativePathToFullPath( const char * relativePath, char fullPath[ 2048 ] ) {
	Initialize();

	const int rv = sprintf( fullPath, "%s/../%s", gApplicationDirectory, relativePath );
	return rv > 0;
}

/*
 =================================
 GetFileData
 Opens the file and stores it in data
 =================================
 */
bool GetFileData( const char * fileName, unsigned char ** data, unsigned int & size ) {
	Initialize();

	char newFileName[ 2048 ];
	RelativePathToFullPath( fileName, newFileName );
	
	// open file for reading
	printf( "opening file: %s\n", newFileName );
	FILE * file = fopen( newFileName, "rb" );
	
	// handle any errors
	if ( file == NULL ) {
		printf("ERROR: open file failed: %s\n", newFileName );
		return false;
	}
	
	// get file size
	fseek( file, 0, SEEK_END );
	fflush( file );
	size = ftell( file );
	fflush( file );
	rewind( file );
	fflush( file );
	
	// output file size
	printf( "file size: %u\n", size );
	
	// create the data buffer
	*data = (unsigned char*)malloc( ( size + 1 ) * sizeof( unsigned char ) );
    
	// handle any errors
	if ( *data == NULL ) {
		printf( "ERROR: Could not allocate memory!\n" );
		fclose( file );
		return false;
	}

	// zero out the memory
	memset( *data, 0, ( size + 1 ) * sizeof( unsigned char ) );
	
	// read the data
	unsigned int bytesRead = fread( *data, sizeof( unsigned char ), size, file );
    fflush( file );
	printf( "total bytes read: %u\n", bytesRead );
    
    assert( bytesRead == size );
	
	// handle any errors
	if ( bytesRead != size ) {
		printf( "ERROR: reading file went wrong\n" );
		fclose( file );
		if ( *data != NULL ) {
			free( *data );
		}
		return false;
	}
	
	fclose( file );
	printf( "Read file was success\n");
	return true;	
}
#endif








#if 1//def WINDOWS
#include <direct.h>
#include <windows.h>
#define GetCurrentDir _getcwd
#else
#include <sys/file.h>
#include <sys/mman.h>
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

static char g_ApplicationDirectory[ FILENAME_MAX ];	// needs to be CWD
static bool g_WasInitialized = false;

/*
=================================
InitializeFileSystem
=================================
*/
void InitializeFileSystem() {
	if ( g_WasInitialized ) {
		return;
	}
	g_WasInitialized = true;

	const bool result = GetCurrentDir( g_ApplicationDirectory, sizeof( g_ApplicationDirectory ) );
	assert( result );
	if ( result ) {
		printf( "ApplicationDirectory: %s\n", g_ApplicationDirectory );
	} else {
		printf( "ERROR: Unable to get current working directory!\n");
	}
}

/*
=================================
RelativePathToFullPath
=================================
*/
void RelativePathToFullPath( const char * relativePathName, char * fullPath ) {
	InitializeFileSystem();

	sprintf( fullPath, "%s/%s", g_ApplicationDirectory, relativePathName );
}

/*
====================================================
GetFileData
Opens the file and stores it in data
====================================================
*/
bool GetFileData( const char * fileNameLocal, unsigned char ** data, unsigned int & size ) {
	InitializeFileSystem();

	char fileName[ 2048 ];
	sprintf( fileName, "%s/%s", g_ApplicationDirectory, fileNameLocal );

	FILE * file = NULL;
	
	// open file for reading
	printf( "opening file: %s\n", fileName );
	file = fopen( fileName, "rb" );
	
	// handle any errors
	if ( file == NULL ) {
//		printf("ERROR: open file failed: %s\n", fileName );
		return false;
	}
	
	// get file size
	fseek( file, 0, SEEK_END );
	fflush( file );
	size = ftell( file );
	fflush( file );
	rewind( file );
	fflush( file );
	
	// output file size
	printf( "file size: %u\n", size );
	
	// create the data buffer
	*data = (unsigned char*)malloc( ( size + 1 ) * sizeof( unsigned char ) );
    
	// handle any errors
	if ( *data == NULL ) {
		printf( "ERROR: Could not allocate memory!\n" );
		fclose( file );
		return false;
	}

	// zero out the memory
	memset( *data, 0, ( size + 1 ) * sizeof( unsigned char ) );
	
	// read the data
	unsigned int bytesRead = (unsigned int)fread( *data, sizeof( unsigned char ), size, file );
    fflush( file );
	printf( "total bytes read: %u\n", bytesRead );
    
    assert( bytesRead == size );
	
	// handle any errors
	if ( bytesRead != size ) {
		printf( "ERROR: reading file went wrong\n" );
		fclose( file );
		if ( *data != NULL ) {
			free( *data );
		}
		return false;
	}
	
	fclose( file );
	printf( "Read file was success\n");
	return true;
}

/*
====================================================
WriteFileData
Opens the file and writes the data
====================================================
*/
bool WriteFileData( const char * fileNameLocal, const void * data, const unsigned int size ) {
	InitializeFileSystem();

	char fileName[ 2048 ];
	sprintf( fileName, "%s/%s", g_ApplicationDirectory, fileNameLocal );

	FILE * file = NULL;

	// open file for write
	printf( "Writing file: %s\n", fileName );
	file = fopen( fileName, "wb" );

	// handle any errors
	if ( file == NULL ) {
		printf( "ERROR: open file for write failed: %s\n", fileName );
		return false;
	}

	unsigned int bytesWritten = fwrite( data, 1, size, file );
	fflush( file );
	fclose( file );

	if ( bytesWritten != size ) {
		printf( "ERROR: bytes written does not match size  %i  %i\n", bytesWritten, size );
		return false;
	}

	printf( "Write file was success\n");
	return true;
}


FILE * g_fileStream = NULL;
bool OpenFileWriteStream( const char * fileNameLocal ) {
	InitializeFileSystem();

	char fileName[ 2048 ];
	sprintf( fileName, "%s/%s", g_ApplicationDirectory, fileNameLocal );

	// open file for write
	printf( "Writing file: %s\n", fileName );
	g_fileStream = fopen( fileName, "wb" );

	// handle any errors
	if ( g_fileStream == NULL ) {
		printf( "ERROR: open file for write stream failed: %s\n", fileName );
		return false;
	}

	return true;
}
bool WriteFileStream( const void * data, const unsigned int size ) {
	unsigned int bytesWritten = fwrite( data, 1, size, g_fileStream );
	//fflush( g_fileStream );	// do we really need to flush it every time?

	return ( size == bytesWritten );
}

bool WriteFileStream( const char * data ) {
	unsigned int strLength = strlen( data );
	return WriteFileStream( data, strLength );
}
void CloseFileWriteStream() {
	if ( NULL != g_fileStream ) {
		fflush( g_fileStream );
		fclose( g_fileStream );
		g_fileStream = NULL;
	}
}




/*
====================================================
SaveFileData
====================================================
*/
bool SaveFileData( const char * fileNameLocal, const void * data, unsigned int size ) {
	InitializeFileSystem();

	char fileName[ 2048 ];
	sprintf( fileName, "%s/%s", g_ApplicationDirectory, fileNameLocal );

	FILE * file = NULL;
	
	// open file for writing
	file = fopen( fileName, "wb" );
	
	// handle any errors
	if ( file == NULL ) {
		printf("ERROR: open file for write failed: %s\n", fileName );
		return false;
	}
	
	unsigned int bytesWritten = (unsigned int )fwrite( data, 1, size, file );
    assert( bytesWritten == size );
	
	// handle any errors
	if ( bytesWritten != size ) {
		printf( "ERROR: writing file went wrong %s\n", fileName );
		fclose( file );
		return false;
	}
	
	fclose( file );
	printf( "Write file was success %s\n", fileName );
	return true;
}

/*
====================================================
GetFileTimeStampWrite
====================================================
*/
int GetFileTimeStampWrite( const char * fileNameLocal ) {
	InitializeFileSystem();

	char fileName[ 2048 ];
	sprintf( fileName, "%s/%s", g_ApplicationDirectory, fileNameLocal );

	HANDLE hFile;
    hFile = CreateFileA( fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if ( INVALID_HANDLE_VALUE == hFile ) {
//        printf( "CreateFile failed with %d  %s\n", GetLastError(), fileName );
        return -1;
    }

	FILETIME writeTime;
	GetFileTime( hFile, NULL, NULL, &writeTime );
	CloseHandle( hFile );

	// Windows System Time is the number of 0.1ms ticks from 1601 January 1st.
	// Unix System Time is the number of 1s ticks from 1970 January 1st.
	// To make our lives easier in cross-system support,
	// it would be wise to convert from windows system time
	// to unix system time.

	SYSTEMTIME base_st = {
		1970,   // wYear
		1,      // wMonth
		0,      // wDayOfWeek
		1,      // wDay
		0,      // wHour
		0,      // wMinute
		0,      // wSecond
		0       // wMilliseconds
	};

	FILETIME base_ft;
	SystemTimeToFileTime( &base_st, &base_ft );

	LARGE_INTEGER itime;
	itime.QuadPart = reinterpret_cast< LARGE_INTEGER & >( writeTime ).QuadPart;
	itime.QuadPart -= reinterpret_cast< LARGE_INTEGER & >( base_ft ).QuadPart;
	itime.QuadPart /= 10000000LL;
	return (int)itime.QuadPart;
}