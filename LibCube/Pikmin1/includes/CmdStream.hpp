#pragma once

#include "essential_functions.hpp"

#include <fstream>
#include <string>
#include <sstream>

namespace libcube { namespace pikmin1 {

class CmdStream
{
private:
	std::ifstream* m_stream;
	const u32 m_fileSize;
public:
	std::string m_line;
	std::string m_token;

	CmdStream() = default;
	~CmdStream() = default;
	CmdStream(std::ifstream* _fstream, u32 _fileSize)
		: m_stream(_fstream), m_fileSize(_fileSize)	{ }

	std::string readToken()
	{
		// Get a line from the ifstream and input it into m_line
		std::getline(*m_stream, m_line);
		// Create a string stream with the line just read
		std::stringstream lineStream(m_line);
		// Use the linestream to isolate a token and input it into m_token
		lineStream >> m_token;
		// Return the token read
		return m_token;
	}

	// Check to see if the stream is at the end of the file
	bool isEOF() const
	{
		// if the stream position is less than the file size, we aren't at the end of the file so return false, else return true
		return (m_stream->tellg() < m_fileSize) ? false : true;
	}

	// Check to see if the stream is at the end of a section (denoted by a "}")
	bool isEOS() const
	{
		return m_token[0] == '}';
	}

	// Simple getter function to retrieve the size of the file given at the constructor
	const u32 getFileSize() const
	{
		return m_fileSize;
	}
};

}

}
