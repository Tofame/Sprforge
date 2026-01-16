#pragma once

#include <fmt/core.h>
#include <iostream>

struct Warninger {
	static void sendWarning(const char* functionInfo, std::string information) {
		fmt::print("[WARNING: {}] {}\n", functionInfo, information);
	}

	static void sendErrorMsg(const char* functionInfo, std::string information) {
		fmt::print("[ERROR: {}] {}\n", functionInfo, information);
	}
};