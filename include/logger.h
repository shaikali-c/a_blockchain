#pragma once
#include "pch.h"

namespace Logger {
	void log(const std::string&);
	void debug(const std::string&);
	void error(const std::string&);
	void reject(const std::string&);
};