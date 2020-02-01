#pragma once

#include <pybind11/embed.h>
namespace py = pybind11;
#include <vector>
#include <cstdio>

#include "Console.hpp"

struct ConsoleHandle
{
	static ConsoleHandle *sCH;

	
	std::string getPrompt() { return mConsole.prompt; }
	void setPrompt(std::string p) { mConsole.prompt = p; }

	Console& mConsole;

	LineObject getHistoryLast() { return mConsole.history.empty() ? LineObject() : mConsole.history[mConsole.history.size()-1]; }
	void setHistoryLast(LineObject h) { if (!mConsole.history.empty()) mConsole.history[mConsole.history.size() - 1] = h; }

	ConsoleHandle(Console& h) : mConsole(h)
	{}

	void scrollback_append(std::string text, std::string type)
	{
		if (type == "INPUT")
		{
		}
		else
		{
			printf("{OUT} %s\n", text.c_str());
			mConsole.AddLog(((type == "ERROR" ? "[error] " : "") + text).c_str());
			// printf("{%s} %s\n", type.c_str(), text.c_str());
		}

	}
};
inline
ConsoleHandle* PyGetCH()
{
	return ConsoleHandle::sCH;
}
inline
void PySetCH(py::object interactive, py::object _stdout, py::object _stdin)
{
}


/*

Console:
	- scrollback_append(text, type)
		- type: INPUT, OUTPUT, ERROR

*/
