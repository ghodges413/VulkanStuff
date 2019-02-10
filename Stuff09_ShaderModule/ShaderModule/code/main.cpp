//
//  main.cpp
//
#include "application.h"

Application * g_application = NULL;

/*
====================================================
main
====================================================
*/
int main( int argc, char * argv[] ) {
	g_application = new Application;

	g_application->MainLoop();

	delete g_application;

	return EXIT_SUCCESS;
}
