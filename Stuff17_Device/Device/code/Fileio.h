/*
 *  Fileio.h
 *
 */

#pragma once

void RelativePathToFullPath( const char * relativePathName, char * fullPath );
bool GetFileData( const char * fileName, unsigned char ** data, unsigned int & size );
bool SaveFileData( const char * fileName, const void * data, unsigned int size );

int GetFileTimeStampWrite( const char * fileName );