//
//  Input.h
//
#pragma once
#include "Math/Vector.h"
#if 0

/*
 =====================================
 Keyboard
 =====================================
 */
class Keyboard {
public:
	enum KeyboardButtons_t {
		kb_BACKSPACE = 8,
		kb_ENTER = 13,
		kb_SHIFT_OUT = 14,
		kb_SHIFT_IN = 15,
		kb_ESC = 27,
        kb_SPACE = 32,
		kb_DEL = 127,
	};
public:
    Keyboard();
    ~Keyboard();
    
    const bool      IsKeyDown( unsigned char key ) const    { return m_keysDown[ key ]; }
    bool            IsKeyDown( unsigned char key )          { return m_keysDown[ key ]; }
    
    void            SetKeyDown( unsigned char key );
    void            SetKeyUp( unsigned char key );
    
    bool            WasKeyDown( unsigned char key );
    
private:
    static const int s_maxKeys = 256;
    bool    m_keysDown[ s_maxKeys ];
    bool    m_wasKeysDown[ s_maxKeys ];
};

/*
 =====================================
 Mouse
 =====================================
 */
class Mouse {
public:
	enum MouseButton_t {
		mb_left = 0,
		mb_right = 1,
		mb_middle = 2,
		mb_thumb = 3,
		mb_ring = 4,
	};
public:
    Mouse();
	~Mouse() {}

	Vec2 GLPos() const { return Vec2( m_pos.x, m_pos.y ); }
	const float & ScrollPos() const { return m_pos.z; }

	Vec3 GLPosDelta() { Vec3 delta = ( m_pos - m_prevPos ); m_prevPos = m_pos; return delta; }
	
	void SetPos( const Vec3 & pos ) { m_prevPos = m_pos; m_pos = pos; }

	bool IsButtonDown( const MouseButton_t & idx ) { /*assert( idx >= 0 && idx < sMaxButtons );*/ return m_buttonsDown[ idx ]; }
	bool WasButtonDown( const MouseButton_t & idx );

	void SetButtonState( const MouseButton_t & idx, const bool & isDown );

private:
	Vec3 m_pos;	// in gl screen space, not windows screen space
	Vec3 m_prevPos;

	static const int s_maxButtons = 5;
	bool m_buttonsDown[ s_maxButtons ];	// Up to 5 mouse buttons
	bool m_wasButtonsDown[ s_maxButtons ];
};

extern Keyboard g_keyboard;
extern Mouse g_mouse;




void keyboard( unsigned char key, int x, int y );
void keyboardup( unsigned char key, int x, int y );
void special( int key, int x, int y );
void mouse( int button, int state, int x, int y );
void motion( int x, int y );
void motionPassive( int x, int y );
void entry( int state );
#else
#include "Math/Vector.h"

enum keyboard_t {
	key_backspace			= 8,
	key_tab					= 9,
	key_enter				= 13,
	key_shift				= 16,
	key_ctrl				= 17,
	key_alt					= 18,
	key_pausebreak			= 19,
	key_capslock			= 20,
	key_escape				= 27,
	key_page_up				= 33,
	key_spacebar			= 32,
	key_page_down			= 34,
	key_end					= 35,
	key_home				= 36,
	key_left_arrow			= 37,
	key_up_arrow			= 38,
	key_right_arrow			= 39,
	key_down_arrow			= 40,
	key_insert				= 45,
	key_delete				= 46,
	key_0					= 48,
	key_1					= 49,
	key_2					= 50,
	key_3					= 51,
	key_4					= 52,
	key_5					= 53,
	key_6					= 54,
	key_7					= 55,
	key_8					= 56,
	key_9					= 57,
	key_a					= 65,
	key_b					= 66,
	key_c					= 67,
	key_d					= 68,
	key_e					= 69,
	key_f					= 70,
	key_g					= 71,
	key_h					= 72,
	key_i					= 73,
	key_j					= 74,
	key_k					= 75,
	key_l					= 76,
	key_m					= 77,
	key_n					= 78,
	key_o					= 79,
	key_p					= 80,
	key_q					= 81,
	key_r					= 82,
	key_s					= 83,
	key_t					= 84,
	key_u					= 85,
	key_v					= 86,
	key_w					= 87,
	key_x					= 88,
	key_y					= 89,
	key_z					= 90,
	key_left_window_key		= 91,
	key_right_window_key	= 92,
	key_select_key			= 93,
	key_numpad_0			= 96,
	key_numpad_1			= 97,
	key_numpad_2			= 98,
	key_numpad_3			= 99,
	key_numpad_4			= 100,
	key_numpad_5			= 101,
	key_numpad_6			= 102,
	key_numpad_7			= 103,
	key_numpad_8			= 104,
	key_numpad_9			= 105,
	key_multiply			= 106,
	key_add					= 107,
	key_subtract			= 109,
	key_decimal_point		= 110,
	key_divide				= 111,
	key_f1					= 112,
	key_f2					= 113,
	key_f3					= 114,
	key_f4					= 115,
	key_f5					= 116,
	key_f6					= 117,
	key_f7					= 118,
	key_f8					= 119,
	key_f9					= 120,
	key_f10					= 121,
	key_f11					= 122,
	key_f12					= 123,
	key_num_lock			= 144,
	key_scroll_lock			= 145,
	key_semi_colon			= 186,
	key_equal_sign			= 187,
	key_comma				= 188,
	key_dash				= 189,
	key_period				= 190,
	key_forward_slash		= 191,
	key_grave_accent		= 192,
	key_open_bracket		= 219,
	key_back_slash			= 220,
	key_close_bracket		= 221,
	key_single_quote		= 222,

	key_left_shift			= 340,
	key_left_control		= 341,
	key_left_alt			= 342,
	key_left_super			= 343,
	key_right_shift			= 344,
	key_right_control		= 345,
	key_right_alt			= 346,
	key_right_super			= 347,
};

enum keyState_t {
	KeyState_RELEASE = 0,
	KeyState_PRESS,
	KeyState_REPEAT,
};

enum mouseButton_t {
// 	#define GLFW_MOUSE_BUTTON_1         0
// #define GLFW_MOUSE_BUTTON_2         1
// #define GLFW_MOUSE_BUTTON_3         2
// #define GLFW_MOUSE_BUTTON_4         3
// #define GLFW_MOUSE_BUTTON_5         4
// #define GLFW_MOUSE_BUTTON_6         5
// #define GLFW_MOUSE_BUTTON_7         6
// #define GLFW_MOUSE_BUTTON_8         7
// #define GLFW_MOUSE_BUTTON_LAST      GLFW_MOUSE_BUTTON_8
// #define GLFW_MOUSE_BUTTON_LEFT      GLFW_MOUSE_BUTTON_1
// #define GLFW_MOUSE_BUTTON_RIGHT     GLFW_MOUSE_BUTTON_2
// #define GLFW_MOUSE_BUTTON_MIDDLE    GLFW_MOUSE_BUTTON_3
	MouseButton_Left = 0,
	MouseButton_Right = 1,
	MouseButton_Middle = 2,
};

enum gamePadMap_t {
	BM_A = 0,
	BM_B = 1,
	BM_X = 2,
	BM_Y = 3,
	BM_LEFT_BUMPER = 4,
	BM_RIGHT_BUMPER = 5,
	BM_SELECT = 6,
	BM_START = 7,	// pause
	BM_LEFT_THUMBSTICK = 8,
	BM_RIGHT_THUMBSTICK = 9,
	BM_DPAD_UP = 10,
	BM_DPAD_RIGHT = 11,
	BM_DPAD_DOWN = 12,
	BM_DPAD_LEFT = 13,
};

struct GLFWwindow;

/*
====================================================
Input
====================================================
*/
class Input {
public:
	Input( GLFWwindow * window );

	keyState_t GetKeyState( keyboard_t key );
	keyState_t GetMouseButtonState( mouseButton_t button );
	bool GetJoyStickAxes();
	bool GetJoyStickButtons();

	static void OnMouseMoved( GLFWwindow * window, double x, double y );
	static void OnMouseWheelScrolled( GLFWwindow * window, double x, double y );
	static void OnKeyboard( GLFWwindow * window, int key, int scancode, int action, int modifiers );

	GLFWwindow * m_glfwWindow;

	Vec2 m_leftThumbStick;
	Vec2 m_rightThumbStick;
	float m_triggerLeft;
	float m_triggerRight;

	int m_numButtons;
	unsigned char m_buttonStates[ 32 ];
};

extern Input * g_Input;
#endif