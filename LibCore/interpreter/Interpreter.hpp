#pragma once

#include <string>
#include <pybind11/embed.h>
namespace py = pybind11;

struct Interpreter
{
    py::scoped_interpreter guard{};

    void exec(const std::string& code);
};
struct LineObject
{
	std::string body;
	int current_character = 0;

	std::string getBody() { return body; }
	void setBody(std::string b) { body = b; }
	int getCurrentCharacter() { return current_character; }
	void setCurrentCharacter(int c) { current_character = c; }
};
