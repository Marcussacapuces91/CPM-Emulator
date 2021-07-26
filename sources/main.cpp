/**
 * Copyright 2021 Marc SIBERT
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <fstream>
#include <exception>

#define LOG		1

#include "computer.h"

Computer computer;

int main(int argc, char** argv) {
	
#ifdef LOG
	std::ofstream out("log.txt");
	auto old_rdbuf = std::clog.rdbuf();
	std::clog.rdbuf(out.rdbuf());
#endif
	
	try {
		computer.init();
		switch (argc) {
			case 1:
				computer.run(0x0000);
				break;
			case 2:
				computer.load(argv[1]);
				computer.run(0x0100);
				break;
			default:
				std::cerr << "Invalid number of arguments!" << std::endl;
		}
	} catch (std::exception& e) {
		std::cerr << "Exception " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
