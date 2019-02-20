/*
 *  Fileio.h
 *
 */

#pragma once

void RelativePathToFullPath( const char * relativePathName, char * fullPath );
bool GetFileData( const char * fileName, unsigned char ** data, unsigned int & size );