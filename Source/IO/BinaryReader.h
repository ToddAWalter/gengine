//
// Clark Kromenaker
//
// Wrapper around a binary data stream,
// with helpers for reading bytes as specific types.
//
#pragma once
#include <istream>

#include "Vector2.h"
#include "Vector3.h"

class BinaryReader
{
public:
	BinaryReader(const std::string& filePath);
    BinaryReader(const char* filePath);
    BinaryReader(const char* memory, unsigned int memoryLength);
    ~BinaryReader();
	
	// Should only read if OK is true, and should only use read value if OK is still true after reading!
	bool OK() const
	{
		// Remember, "good" returns true as long as fail/bad/eof bits are all false.
		return mStream->good();
	}
    
    void Seek(int position);
    void Skip(int size);
    
	int GetPosition() const { return (int)mStream->tellg(); }
    
    int Read(char* buffer, int size);
    int Read(unsigned char* buffer, int size);
    
    std::string ReadString(int length);
    
    uint8_t ReadUByte();
    int8_t ReadByte();
    
    uint16_t ReadUShort();
    int16_t ReadShort();
    
    uint32_t ReadUInt();
    int32_t ReadInt();
    
    float ReadFloat();
    double ReadDouble();

    // For convenience - reading in some more commonly encountered complex types.
    Vector2 ReadVector2();
    Vector3 ReadVector3();

private:
	// Stream we are reading from.
	// Needs to be pointer because type of stream (memory, file, etc) changes sometimes.
    std::istream* mStream = nullptr;
};
