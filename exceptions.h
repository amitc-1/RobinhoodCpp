#pragma once

#include <exception>
//#include <string>

class RobinhoodException : public std::runtime_error
{
	public:
		// constructor only which passes message to base class
		RobinhoodException(std::string msg)
			: std::runtime_error(msg) {}
};