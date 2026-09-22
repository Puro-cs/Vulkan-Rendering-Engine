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
#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <glm/glm.hpp>

/**
 * @brief One command that was typed into the terminal, e.g. Room.Move(1, 0, 0).
 */
struct TerminalCommand
{
	enum class Type
	{
		Move,
		Rotate,
		Scale
	};

	std::string name;                                // the name of the object or light
	Type        type   = Type::Move;
	glm::vec3   values = {0.0f, 0.0f, 0.0f};        // x, y, z; degrees for Rotate
};

/**
 * @brief Reads the commands that are typed into the terminal while the engine renders.
 *
 * A reader thread waits for input on std::cin and puts every line into a queue. The main thread
 * takes the lines out of the queue once per frame, so the scene is only changed by the main thread.
 */
class TerminalCommands
{
  public:
	/**
	 * @brief Start the reader thread.
	 */
	void Start();

	/**
	 * @brief Take the lines that were typed since the last call.
	 * @return The lines, oldest first.
	 */
	std::vector<std::string> TakeLines();

	/**
	 * @brief Parse a line of the form Name.Move(x, y, z), Name.Rotate(x, y, z) or Name.Scale(x, y, z).
	 * @param line The line to parse.
	 * @param command The parsed command.
	 * @return True if the line has this form, false otherwise.
	 */
	static bool Parse(const std::string &line, TerminalCommand &command);

  private:
	// The lines that were read and not taken yet. The reader thread is detached and can outlive
	// this object, so the queue belongs to both of them.
	struct Queue
	{
		std::mutex               mutex;
		std::vector<std::string> lines;
	};

	std::shared_ptr<Queue> queue = std::make_shared<Queue>();

	bool started = false;
};
