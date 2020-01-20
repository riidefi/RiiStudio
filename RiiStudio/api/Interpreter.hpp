#pragma once

#include <string>
#include <pybind11/embed.h>
namespace py = pybind11;

struct Interpreter
{
    py::scoped_interpreter guard{};

    void exec(const std::string& code);
};