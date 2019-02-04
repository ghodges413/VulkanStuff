//
//  main.cpp
//
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 720;

/*
====================================================
InitWindow
====================================================
*/
void InitWindow( GLFWwindow * glfwWindow ) {
	glfwInit();

	glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );

	glfwWindow = glfwCreateWindow( WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr );
}

/*
====================================================
main
====================================================
*/
int main( int argc, char * argv[] ) {
	GLFWwindow * glfwWindow = NULL;
	InitWindow( glfwWindow );

	// Cleanup GLFW
	glfwDestroyWindow( glfwWindow );
	glfwTerminate();

	return EXIT_SUCCESS;
}
