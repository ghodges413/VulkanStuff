/*
 *  Targa.h
 *
 */
#pragma once

enum targaOrdering_t {
    TARGA_ORDER_NONE          = 0,
    TARGA_ORDER_LEFT_TO_RIGHT = 1 << 0,
    TARGA_ORDER_RIGHT_TO_LEFT = 1 << 1,
    TARGA_ORDER_TOP_TO_BOTTOM = 1 << 2,
    TARGA_ORDER_BOTTOM_TO_TOP = 1 << 3,
};

/*
 ===============================
 Targa
 ===============================
 */
class Targa {
public:
	Targa();
	~Targa();
    
    bool Load( const char * filename, const bool verbose = false );

	const unsigned char * DataPtr() const { return m_data_ptr; }

	int GetWidth() const    { return m_width; }
	int GetHeight() const   { return m_height; }

	int GetBitsPerPixel() const { return m_bitsPerPixel; }

private:
    static void DisplayHeader( const unsigned char * data, const int size );
	static void ByteSwap( unsigned char & a, unsigned char & b );
	
private:	
	int m_width;
	int m_height;
    int m_bitsPerPixel;
    
    int m_ordering;
	int m_targaOrdering;
    
    unsigned char * m_data_ptr;
};
