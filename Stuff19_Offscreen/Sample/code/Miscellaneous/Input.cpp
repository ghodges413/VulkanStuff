//
//  Input.cpp
//
#include "Miscellaneous/Input.h"
#if 0
Keyboard g_keyboard;
Mouse g_mouse;

/*
 ==========================================================================
 
 Keyboard

 ==========================================================================
 */

/*
 =====================================
 Keyboard::Keyboard
 =====================================
 */
Keyboard::Keyboard() {
    for ( int i = 0; i < s_maxKeys; ++i ) {
        m_keysDown[ i ] = false;
        m_wasKeysDown[ i ] = false;
    }
}

/*
 =====================================
 Keyboard::~Keyboard
 =====================================
 */
Keyboard::~Keyboard() {
    for ( int i = 0; i < s_maxKeys; ++i ) {
        m_keysDown[ i ] = false;
        m_wasKeysDown[ i ] = false;
    }
}

/*
 =====================================
 Keyboard::SetKeyDown
 =====================================
 */
void Keyboard::SetKeyDown( unsigned char key ) {
    m_keysDown[ key ] = true;
    m_wasKeysDown[ key ] = true;
}

/*
 =====================================
 Keyboard::SetKeyUp
 =====================================
 */
void Keyboard::SetKeyUp( unsigned char key ) {
    m_keysDown[ key ] = false;
}

/*
 =====================================
 Keyboard::WasKeyDown
 =====================================
 */
bool Keyboard::WasKeyDown( unsigned char key ) {
    if ( m_wasKeysDown[ key ] && !m_keysDown[ key ] ) {
        m_wasKeysDown[ key ] = false;
        return true;
    }
    return false;
}

/*
 ==========================================================================
 
 Mouse

 ==========================================================================
 */

/*
 =====================================
 Mouse::Mouse
 =====================================
 */
Mouse::Mouse() :
m_pos( 0.0f ) {
	for ( int i = 0; i < 5; ++i ) {
		m_buttonsDown[ i ] = false;
	}
}

/*
 =====================================
 Mouse::SetButtonState
 =====================================
 */
void Mouse::SetButtonState( const MouseButton_t & idx, const bool & isDown ) {
	assert( idx >= 0 && idx < s_maxButtons );

	m_wasButtonsDown[ idx ] = m_buttonsDown[ idx ];
	m_buttonsDown[ idx ] = isDown;
}

/*
 =====================================
 Mouse::WasButtonDown
 =====================================
 */
bool Mouse::WasButtonDown( const MouseButton_t & idx ) {
    if ( m_wasButtonsDown[ idx ] && !m_buttonsDown[ idx ] ) {
        m_wasButtonsDown[ idx ] = false;
        return true;
    }
    return false;
}


//#include "Graphics/Graphics.h"
extern int g_screenHeight;

/*
 ================================
 keyboard
 ================================
 */
void keyboard( unsigned char key, int x, int y ) {
    g_keyboard.SetKeyDown( key );
        
//     int mod = glutGetModifiers();
// //     switch ( mod ) {
// //         case GLUT_ACTIVE_CTRL:	{ printf( "Ctrl Held\n" ); } break;
// //         case GLUT_ACTIVE_SHIFT:	{ printf( "Shift Held\n" ); } break;
// //         case GLUT_ACTIVE_ALT:	{ printf( "Alt Held\n" ); } break;
// //     }
// 	if ( mod | GLUT_ACTIVE_SHIFT ) {
// 		g_keyboard.SetKeyDown( Keyboard::kb_SHIFT_IN );
// 	} else {
// 		g_keyboard.SetKeyUp( Keyboard::kb_SHIFT_IN );
// 	}
}

/*
 ================================
 keyboardup
 ================================
 */
void keyboardup( unsigned char key, int x, int y ) {
	g_keyboard.SetKeyUp( key );
}

/*
 ================================
 special
 ================================
 */
bool ignoreRepeats = false;
void special( int key, int x, int y ) {
}

/*
 ================================
 mouse
 ================================
 */
void mouse( int button, int state, int x, int y ) {
/*
// Mouse Buttons
#define  GLUT_LEFT_BUTTON                   0x0000
#define  GLUT_MIDDLE_BUTTON                 0x0001
#define  GLUT_RIGHT_BUTTON                  0x0002

// Button States
#define  GLUT_DOWN                          0x0000
#define  GLUT_UP                            0x0001

// Cursor/Window States
#define  GLUT_LEFT                          0x0000
#define  GLUT_ENTERED                       0x0001
*/
	// Convert from windows coords (origin at top right)
	// To gl coords (origin at lower left)
	y = g_screenHeight - y;

	// Get Left/Right mouse button inputs
	if ( button == GLUT_RIGHT_BUTTON ) {
		if (state == GLUT_DOWN ) {
			g_mouse.SetButtonState( Mouse::mb_right, true );
		} else {
			g_mouse.SetButtonState( Mouse::mb_right, false );
		}
	} else if ( button == GLUT_LEFT_BUTTON ) {
		if ( state == GLUT_DOWN ) {
			g_mouse.SetButtonState( Mouse::mb_left, true );
		} else {
			g_mouse.SetButtonState( Mouse::mb_left, false );
		}
	} else if ( button == GLUT_MIDDLE_BUTTON ) {
		if ( state == GLUT_DOWN ) {
			g_mouse.SetButtonState( Mouse::mb_middle, true );
		} else {
			g_mouse.SetButtonState( Mouse::mb_middle, false );
		}
	}

	// Wheel reports as button 3(scroll up) and button 4(scroll down)
	int mouseZ = (int)g_mouse.ScrollPos();
	if ( 3 == button && GLUT_DOWN == state ) {
		// Mouse wheel scrolled up
		++mouseZ;
	} else if ( 4 == button && GLUT_DOWN == state ) {
		// Mouse wheel scrolled down
		--mouseZ;
	}

	// Update the mouse position
	g_mouse.SetPos( Vec3( x, y, mouseZ ) );
}

/*
 ================================
 motion

 * Updates mouse position when buttons are pressed
 ================================
 */
void motion( int x, int y ) {
	// Convert from windows coords (origin at top right)
	// To gl coords (origin at lower left)
	y = g_screenHeight - y;

	Vec3 pos( x, y, g_mouse.ScrollPos() );
	g_mouse.SetPos( pos );
}

/*
 ================================
 motionPassive

 * Updates mouse position when zero buttons are pressed
 ================================
 */
void motionPassive( int x, int y ) {
	// Convert from windows coords (origin at top right)
	// To gl coords (origin at lower left)
	y = g_screenHeight - y;

	Vec3 pos( x, y, g_mouse.ScrollPos() );
	g_mouse.SetPos( pos );
}

/*
 ================================
 entry
 ================================
 */
void entry( int state ) {
// 	if ( GLUT_ENTERED == state ) {
// 		printf( "Mouse entered window\n" );
// 	} else {
// 		printf( "Mouse left window area\n" );
// 	}
}
#else

#include "Miscellaneous/Input.h"
#include "application.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

Input * g_Input = NULL;

/*
========================================================================================================

Input

========================================================================================================
*/

/*
====================================================
Input::Input
====================================================
*/
Input::Input( GLFWwindow * window ) {
	m_glfwWindow = window;

	glfwSetInputMode( m_glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED );
	glfwSetInputMode( m_glfwWindow, GLFW_STICKY_KEYS, GLFW_TRUE );
	glfwSetCursorPosCallback( m_glfwWindow, Input::OnMouseMoved );
	glfwSetScrollCallback( m_glfwWindow, Input::OnMouseWheelScrolled );
	glfwSetKeyCallback( m_glfwWindow, Input::OnKeyboard );
}

/*
====================================================
Input::GetKeyState
====================================================
*/
keyState_t Input::GetKeyState( keyboard_t key ) {
	int state = glfwGetKey( m_glfwWindow, key );

	if ( GLFW_RELEASE == state ) {
		return keyState_t::KeyState_RELEASE;
	}
	if ( GLFW_PRESS == state ) {
		return keyState_t::KeyState_PRESS;
	}
	if ( GLFW_REPEAT == state ) {
		return keyState_t::KeyState_REPEAT;
	}
	return keyState_t::KeyState_RELEASE;
}

/*
====================================================
Input::GetMouseButtonState
====================================================
*/
keyState_t Input::GetMouseButtonState( mouseButton_t button ) {
	int state = glfwGetMouseButton( m_glfwWindow, button );

	if ( GLFW_RELEASE == state ) {
		return keyState_t::KeyState_RELEASE;
	}
	if ( GLFW_PRESS == state ) {
		return keyState_t::KeyState_PRESS;
	}
	if ( GLFW_REPEAT == state ) {
		return keyState_t::KeyState_REPEAT;
	}
	return keyState_t::KeyState_RELEASE;
}

/*
====================================================
Input::GetJoyStickAxes
====================================================
*/
bool Input::GetJoyStickAxes() {
	// Check for joystick
	int present = glfwJoystickPresent( GLFW_JOYSTICK_1 );
	if ( present <= 0 ) {
		return false;
	}

	int numAxes = 0;
	const float * axes = glfwGetJoystickAxes( GLFW_JOYSTICK_1, &numAxes );
	if ( numAxes > 5 ) {
		m_leftThumbStick.x = axes[ 0 ];
		m_leftThumbStick.y = axes[ 1 ];
		m_rightThumbStick.x = axes[ 2 ];
		m_rightThumbStick.y = axes[ 3 ];
		m_triggerLeft = axes[ 4 ];
		m_triggerRight = axes[ 5 ];
	}

	return numAxes > 5;
}

/*
====================================================
Input::GetJoyStickButtons
====================================================
*/
bool Input::GetJoyStickButtons() {
	const unsigned char * buttons = glfwGetJoystickButtons( GLFW_JOYSTICK_1, &m_numButtons );
	for ( int i = 0; i < m_numButtons && i < 32; i++ ) {
		m_buttonStates[ i ] = buttons[ i ];
	}

	return m_numButtons > 0;
}

/*
====================================================
Input::OnMouseMoved
====================================================
*/
void Input::OnMouseMoved( GLFWwindow * window, double x, double y ) {
	Application * application = (Application *)glfwGetWindowUserPointer( window );
	application->MouseMoved( (float)x, (float)y );
}

/*
====================================================
Input::OnMouseWheelScrolled
====================================================
*/
void Input::OnMouseWheelScrolled( GLFWwindow * window, double x, double y ) {
	Application * application = (Application *)glfwGetWindowUserPointer( window );
	application->MouseScrolled( (float)y );
}

/*
====================================================
Input::OnKeyboard
====================================================
*/
void Input::OnKeyboard( GLFWwindow * window, int key, int scancode, int action, int modifiers ) {
	// key = key that was pressed/released
	// action = press, release, repeat
	// modifiers = bit field for alt,ctrl,etc
	//printf( "key %i  scancode %i,  action %i,  modifiers %i\n", key, scancode, action, modifiers );
	Application * application = (Application *)glfwGetWindowUserPointer( window );
	application->Keyboard( key, scancode, action, modifiers );
}

#endif