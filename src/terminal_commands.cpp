/* Copyright (c) 2026 Paulo Hoheisel
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "terminal_commands.h"

#include <iostream>
#include <sstream>
#include <thread>

void TerminalCommands::Start()
{
	if (started)
	{
		return;
	}
	started = true;

	// The thread spends its life blocked in std::getline(). It is detached, because a thread that
	// has to be joined would keep the program alive at exit until one more line is typed.
	std::thread([queue = queue] {
		std::string line;
		while (std::getline(std::cin, line))
		{
			std::lock_guard<std::mutex> lk(queue->mutex);
			queue->lines.push_back(line);
		}
	}).detach();
}

std::vector<std::string> TerminalCommands::TakeLines()
{
	std::vector<std::string> lines;
	{
		std::lock_guard<std::mutex> lk(queue->mutex);
		lines.swap(queue->lines);
	}
	return lines;
}

// Remove the blanks at both ends
static std::string Trim(const std::string &text)
{
	const size_t first = text.find_first_not_of(" \t\r");
	if (first == std::string::npos)
	{
		return std::string();
	}
	const size_t last = text.find_last_not_of(" \t\r");
	return text.substr(first, last - first + 1);
}

bool TerminalCommands::Parse(const std::string &line, TerminalCommand &command)
{
	// Name.Move(x, y, z): the name ends at the last '.' in front of the '('
	const size_t open  = line.find('(');
	const size_t close = line.find(')', open);
	if (open == std::string::npos || close == std::string::npos)
	{
		return false;
	}
	const size_t dot = line.rfind('.', open);
	if (dot == std::string::npos)
	{
		return false;
	}

	// Behind the ')' only a ';' may follow
	const std::string rest = Trim(line.substr(close + 1));
	if (!rest.empty() && rest != ";")
	{
		return false;
	}

	command.name = Trim(line.substr(0, dot));
	if (command.name.empty())
	{
		return false;
	}

	const std::string type = Trim(line.substr(dot + 1, open - dot - 1));
	if (type == "Move")
	{
		command.type = TerminalCommand::Type::Move;
	}
	else if (type == "Rotate")
	{
		command.type = TerminalCommand::Type::Rotate;
	}
	else if (type == "Scale")
	{
		command.type = TerminalCommand::Type::Scale;
	}
	else
	{
		return false;
	}

	// Three numbers, separated by commas
	std::istringstream values(line.substr(open + 1, close - open - 1));
	char               comma1 = 0;
	char               comma2 = 0;
	values >> command.values.x >> comma1 >> command.values.y >> comma2 >> command.values.z;
	if (values.fail() || comma1 != ',' || comma2 != ',')
	{
		return false;
	}

	// Nothing but blanks may follow the third number
	values >> std::ws;
	return values.eof();
}
