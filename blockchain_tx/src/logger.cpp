#include "logger.h"

void Logger::log(const std::string& l) {
	std::cout << "[LOG] " << l << "\n";
}

void Logger::debug(const std::string& l) {
	std::cout << "[DEBUG] " << l << "\n";
}

void Logger::error(const std::string& l) {
	std::cout << "[ERROR] " << l << "\n";
}

void Logger::reject(const std::string& l) {
	std::cout << "[REJECT] " << l << "\n";
}