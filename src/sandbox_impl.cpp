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
#include "sandbox.h"

#include "camera_component.h"
#include "crash_reporter.h"
#include "engine.h"
#include "light_component.h"
#include "scene_loading.h"
#include "terminal_commands.h"
#include "transform_component.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>

// This file implements what sandbox.h declares. It is the only place where the classes of the
// sandbox file meet the classes of the engine.

// Constants
#if defined(NDEBUG)
constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif

// The calls of the initialization chain, in the order in which they have to be made
enum InitializationCall
{
	InitializeWindowCall,
	CreateInstanceCall,
	PickDeviceCall,
	CreateSwapChainCall,
	InitializeRenderingCall,
	CreatePipelinesCall,
	CreateCommandBuffersCall,
	CreateSyncObjectsCall,
	InitializationCallCount
};

static const char *const INITIALIZATION_CALL_NAMES[InitializationCallCount] = {
    "InitializeWindow", "CreateInstance", "PickDevice", "CreateSwapChain",
    "InitializeRendering", "CreatePipelines", "CreateCommandBuffers", "CreateSyncObjects"};

// What each call does, for the terminal output
static const char *const INITIALIZATION_CALL_DESCRIPTIONS[InitializationCallCount] = {
    "the window and its input callbacks",
    "Vulkan instance, debug messenger, window surface",
    "GPU, logical device with its queues, memory pool",
    "swap chain and its image views",
    "dynamic rendering, depth image, off-screen color image",
    "the engine's pipelines, descriptor set layouts, light buffers",
    "command pool, descriptor pool, default textures, command buffers",
    "semaphores and fences, worker threads, model loader, UI"};

// The frame calls of the render loop, in the order in which they have to be made once per frame
enum FrameCall
{
	BeginFrameCall,
	UpdateSceneCall,
	BeginRenderingCall,
	DrawSceneCall,
	EndRenderingCall,
	EndFrameCall,
	FrameCallCount
};

static const char *const FRAME_CALL_NAMES[FrameCallCount] = {
    "BeginFrame", "UpdateScene", "BeginRendering", "DrawScene", "EndRendering", "EndFrame"};

// What each call does, for the terminal output
static const char *const FRAME_CALL_DESCRIPTIONS[FrameCallCount] = {
    "wait for the GPU, acquire a swap chain image",
    "camera controls, terminal commands, scene data -> uniform buffers",
    "begin the command buffer, clear the color and depth attachments",
    "per object: bind pipeline, descriptor sets, buffers, draw",
    "the engine's UI on top, end the command buffer",
    "submit the command buffer, present the image"};

/**
 * @brief A pipeline as the terminal output shows it: the engine's "pbr" or one created with
 * CreatePipeline(), with the names of the objects that were added to it.
 */
struct PipelineView
{
	std::string              name;
	std::string              shaderFile;
	PipelineSettings         settings;
	bool                     engine = false;        // "pbr": its settings are the engine's, not the student's
	std::vector<std::string> objects;
};

/**
 * @brief The hidden part of the Sandbox: the engine and the objects that were handed out.
 */
struct Sandbox::Impl
{
	Engine engine;

	std::vector<std::unique_ptr<SceneObject>> objects;
	std::vector<std::unique_ptr<Camera>>      cameras;
	std::vector<std::unique_ptr<Light>>       lights;

	bool modelLoaded = false;

	// The initialization chain: which calls are done, and the title of the window (the name the Vulkan
	// instance is created with)
	bool        initializationDone[InitializationCallCount] = {};
	bool        initializationOrderPrinted                  = false;
	std::string title;

	bool RequireInitialization(const char *caller, int callCount);
	bool InitializationCall(int call, const std::function<bool()> &work);

	// The render loop: whether the first IsRunning() has done its one-time work, the frame call that
	// is expected next, and whether rendering was stopped by a frame call out of order or an error
	bool loopStarted      = false;
	int  expectedCall     = BeginFrameCall;
	bool renderingStopped = false;

	bool FrameSequenceError(const char *caller);
	void FrameCall(int call, const std::function<void()> &work);

	// The terminal output: the pipelines in the order in which they are printed ("pbr" first), and the
	// three views (the chain, every pipeline with its objects, the frame sequence)
	std::vector<PipelineView> pipelineViews;

	void PrintInitializationChain(std::ostream &out, int markedCall, const char *reason) const;
	void ReportInitializationChain(int markedCall, const char *reason);
	void PrintPipelines(std::ostream &out) const;
	void PrintFrameSequence(std::ostream &out, int markedCall, const char *reason) const;
	void PrintOverview(std::ostream &out) const;

	// The commands that are typed into the terminal while the engine renders
	TerminalCommands terminalCommands;

	bool FindEntities(const std::string &name, std::vector<Entity *> &entities, bool &isLight) const;
	void PrintTerminalHelp(const std::string &problem) const;
	void ApplyTerminalCommands();
};

/**
 * @brief Component that applies the terminal commands.
 *
 * The engine updates the components of all entities once per frame on the main thread, and not
 * while a model is loading (Engine::Update). That is the moment at which the scene may be changed.
 */
class TerminalCommandComponent final : public Component
{
  private:
	std::function<void()> applyCommands;

  public:
	/**
	 * @brief Constructor.
	 * @param applyCommandsFunction The function that applies the commands.
	 */
	explicit TerminalCommandComponent(std::function<void()> applyCommandsFunction) :
	    Component("TerminalCommandComponent"),
	    applyCommands(std::move(applyCommandsFunction))
	{}

	/**
	 * @brief Apply the commands that were typed since the last frame.
	 * @param deltaTime The time elapsed since the last frame.
	 */
	void Update(std::chrono::milliseconds deltaTime) override
	{
		applyCommands();
	}
};

// The glTF loader names its entities "<model>_Material_<index>_<materialName>"
static std::string MaterialNameOf(const Entity *entity)
{
	const std::string &entityName = entity->GetName();
	const size_t       tagPos     = entityName.find("_Material_");
	if (tagPos == std::string::npos)
	{
		return std::string();
	}
	const std::string remainder      = entityName.substr(tagPos + std::string("_Material_").size());
	const size_t      nextUnderscore = remainder.find('_');
	if (nextUnderscore == std::string::npos)
	{
		return std::string();
	}
	return remainder.substr(nextUnderscore + 1);
}

// --- SceneObject ---

void SceneObject::SetPosition(const glm::vec3 &position)
{
	for (Entity *entity : entities)
	{
		if (auto *transform = entity->GetComponent<TransformComponent>())
		{
			transform->SetPosition(position);
		}
	}
}

void SceneObject::SetRotation(const glm::vec3 &degrees)
{
	for (Entity *entity : entities)
	{
		if (auto *transform = entity->GetComponent<TransformComponent>())
		{
			transform->SetRotation(glm::radians(degrees));
		}
	}
}

void SceneObject::SetScale(const glm::vec3 &scale)
{
	for (Entity *entity : entities)
	{
		if (auto *transform = entity->GetComponent<TransformComponent>())
		{
			transform->SetScale(scale);
		}
	}
}

void SceneObject::Move(const glm::vec3 &translation)
{
	for (Entity *entity : entities)
	{
		if (auto *transform = entity->GetComponent<TransformComponent>())
		{
			transform->Translate(translation);
		}
	}
}

void SceneObject::Rotate(const glm::vec3 &degrees)
{
	for (Entity *entity : entities)
	{
		if (auto *transform = entity->GetComponent<TransformComponent>())
		{
			transform->Rotate(glm::radians(degrees));
		}
	}
}

void SceneObject::Scale(const glm::vec3 &factors)
{
	for (Entity *entity : entities)
	{
		if (auto *transform = entity->GetComponent<TransformComponent>())
		{
			transform->Scale(factors);
		}
	}
}

SceneObject *SceneObject::Part(const std::string &materialName)
{
	// A part is named after the object and the material, e.g. "Room.wood"
	const std::string partName = name + "." + materialName;

	// A part that was asked for before
	for (const auto &part : parts)
	{
		if (part->name == partName)
		{
			return part.get();
		}
	}

	auto part  = std::make_unique<SceneObject>();
	part->name = partName;
	for (Entity *entity : entities)
	{
		if (MaterialNameOf(entity) == materialName)
		{
			part->entities.push_back(entity);
		}
	}

	if (part->entities.empty())
	{
		std::cerr << "Part: the object \"" << name << "\" has no material \"" << materialName << "\". Its materials:";
		for (Entity *entity : entities)
		{
			std::cerr << " \"" << MaterialNameOf(entity) << "\"";
		}
		std::cerr << std::endl;
	}

	parts.push_back(std::move(part));
	return parts.back().get();
}

// --- Camera ---

void Camera::SetPosition(const glm::vec3 &position)
{
	if (auto *transform = entity->GetComponent<TransformComponent>())
	{
		transform->SetPosition(position);
	}
}

void Camera::SetRotation(const glm::vec3 &degrees)
{
	if (auto *transform = entity->GetComponent<TransformComponent>())
	{
		transform->SetRotation(glm::radians(degrees));
	}
}

void Camera::SetFieldOfView(float degrees)
{
	if (auto *camera = entity->GetComponent<CameraComponent>())
	{
		camera->SetFieldOfView(degrees);
	}
}

// --- Light ---

void Light::SetPosition(const glm::vec3 &position)
{
	if (auto *transform = entity->GetComponent<TransformComponent>())
	{
		transform->SetPosition(position);
	}
}

void Light::SetRotation(const glm::vec3 &degrees)
{
	if (auto *transform = entity->GetComponent<TransformComponent>())
	{
		transform->SetRotation(glm::radians(degrees));
	}
}

void Light::SetDirection(const glm::vec3 &direction)
{
	if (glm::length(direction) == 0.0f)
	{
		return;
	}
	// A light shines along the -Z axis of its transform (LightComponent::GetLight). The rotation
	// around x (pitch) and around y (yaw) that turns (0, 0, -1) into the direction:
	if (auto *transform = entity->GetComponent<TransformComponent>())
	{
		const glm::vec3 d = glm::normalize(direction);
		transform->SetRotation(glm::vec3(std::asin(d.y), std::atan2(-d.x, -d.z), 0.0f));
	}
}

void Light::SetColor(const glm::vec3 &color)
{
	if (auto *light = entity->GetComponent<LightComponent>())
	{
		light->SetColor(color);
	}
}

void Light::SetIntensity(float intensity)
{
	if (auto *light = entity->GetComponent<LightComponent>())
	{
		light->SetIntensity(intensity);
	}
}

void Light::SetRange(float range)
{
	if (auto *light = entity->GetComponent<LightComponent>())
	{
		light->SetRange(range);
	}
}

void Light::SetConeAngles(float innerDegrees, float outerDegrees)
{
	if (auto *light = entity->GetComponent<LightComponent>())
	{
		light->SetConeAngles(glm::radians(innerDegrees), glm::radians(outerDegrees));
	}
}

// --- Terminal commands ---

// The entities behind a name: all parts of an object, or the entity of a light
bool Sandbox::Impl::FindEntities(const std::string &name, std::vector<Entity *> &entities, bool &isLight) const
{
	for (const auto &object : objects)
	{
		if (object->name == name)
		{
			entities = object->entities;
			isLight  = false;
			return true;
		}
	}
	for (const auto &light : lights)
	{
		if (light->entity->GetName() == name)
		{
			entities = {light->entity};
			isLight  = true;
			return true;
		}
	}
	return false;
}

// The one line that answers a line that could not be applied
void Sandbox::Impl::PrintTerminalHelp(const std::string &problem) const
{
	std::cerr << "Terminal: " << problem << " Commands: Name.Move(x, y, z), Name.Rotate(x, y, z) in degrees, Name.Scale(x, y, z). Names:";
	const char *separator = " ";
	for (const auto &object : objects)
	{
		std::cerr << separator << "\"" << object->name << "\"";
		separator = ", ";
	}
	for (const auto &light : lights)
	{
		std::cerr << separator << "\"" << light->entity->GetName() << "\"";
		separator = ", ";
	}
	std::cerr << std::endl;
}

// Called once per frame on the main thread, see TerminalCommandComponent
void Sandbox::Impl::ApplyTerminalCommands()
{
	for (const std::string &line : terminalCommands.TakeLines())
	{
		// An empty line is not a command
		if (line.find_first_not_of(" \t\r") == std::string::npos)
		{
			continue;
		}

		TerminalCommand command;
		if (!TerminalCommands::Parse(line, command))
		{
			PrintTerminalHelp("\"" + line + "\" is not a command.");
			continue;
		}

		std::vector<Entity *> entities;
		bool                  isLight = false;
		if (!FindEntities(command.name, entities, isLight))
		{
			PrintTerminalHelp("there is no object or light \"" + command.name + "\".");
			continue;
		}
		if (isLight && command.type == TerminalCommand::Type::Scale)
		{
			PrintTerminalHelp("a light has no size, \"" + command.name + "\" cannot be scaled.");
			continue;
		}

		for (Entity *entity : entities)
		{
			if (auto *transform = entity->GetComponent<TransformComponent>())
			{
				switch (command.type)
				{
					case TerminalCommand::Type::Move:
						transform->Translate(command.values);
						break;
					case TerminalCommand::Type::Rotate:
						transform->Rotate(glm::radians(command.values));
						break;
					case TerminalCommand::Type::Scale:
						transform->Scale(command.values);
						break;
				}
			}
		}
	}
}

// --- Terminal output ---

// One list of calls, in the form of the roadmap:
//   [ok] CreateSwapChain       swap chain and its image views
//   [!!] InitializeRendering   <- missing
//   [  ] CreatePipelines       the engine's pipelines, descriptor set layouts, light buffers
// The first `doneCount` calls are done, `markedCall` is the call the error is about (-1: none).
static void PrintCallList(std::ostream &out, const char *const *names, const char *const *descriptions, int count, int doneCount, int markedCall, const char *reason)
{
	size_t nameWidth = 0;
	for (int call = 0; call < count; ++call)
	{
		nameWidth = std::max(nameWidth, std::strlen(names[call]));
	}
	for (int call = 0; call < count; ++call)
	{
		const char *mark = (call == markedCall) ? "[!!]" : (call < doneCount ? "[ok]" : "[  ]");
		out << "  " << mark << " " << std::left << std::setw(static_cast<int>(nameWidth) + 2) << names[call];
		if (call == markedCall)
		{
			out << "<- " << reason;
		}
		else
		{
			out << descriptions[call];
		}
		out << std::endl;
	}
}

// One pipeline, in the form of the roadmap: the stages from the vertex input to the attachments, which
// of them the student sets ("yours") and which the engine fixes, and the objects the pipeline draws
static void PrintPipeline(std::ostream &out, const PipelineView &pipeline, const std::vector<std::string> &objects)
{
	// The build compiles "shaders/x.slang" to "shaders/x.spv", the file the engine loads
	const std::string spvFile = std::filesystem::path(pipeline.shaderFile).replace_extension(".spv").generic_string();
	out << "Pipeline \"" << pipeline.name << "\"    " << pipeline.shaderFile << " -> " << spvFile << std::endl;
	out << std::endl;

	// The four settings of the student; the engine's own pipeline has none
	const char *yours    = pipeline.engine ? "fixed" : "yours";
	const char *cullMode = "none";
	switch (pipeline.settings.cullMode)
	{
		case CullMode::None:
			cullMode = "none";
			break;
		case CullMode::Front:
			cullMode = "front";
			break;
		case CullMode::Back:
			cullMode = "back";
			break;
	}
	const char *depthTest   = !pipeline.settings.depthTest ? "off" : (pipeline.settings.blending ? "on, no depth writes" : "on, writes depth");
	const char *blending    = pipeline.settings.blending ? "on, drawn in the transparent pass" : (pipeline.engine ? "off (on for blended materials)" : "off");
	const char *attachments = pipeline.settings.blending ? "swap chain image + depth image" : "off-screen color image + depth image";

	const auto stage = [&out](const char *name, const std::string &description, const char *who) {
		out << "  " << std::left << std::setw(19) << name << std::setw(43) << description << who << std::endl;
	};
	const auto connector = [&out]() { out << "        |" << std::endl; };
	stage("Input assembly", "triangle list, engine vertex layout", "fixed");
	connector();
	stage("Vertex shader", "VSMain", yours);
	connector();
	stage("Rasterization", std::string("cull mode: ") + cullMode, yours);
	connector();
	stage("Fragment shader", "PSMain", yours);
	connector();
	stage("Depth test", depthTest, yours);
	connector();
	stage("Color blending", blending, yours);
	connector();
	stage("Attachments", attachments, "fixed");
	out << std::endl;

	out << "  " << std::left << std::setw(19) << "Objects";
	if (objects.empty())
	{
		out << "none";
	}
	const char *separator = "";
	for (const std::string &object : objects)
	{
		out << separator << object;
		separator = ", ";
	}
	out << std::endl;
}

void Sandbox::Impl::PrintInitializationChain(std::ostream &out, int markedCall, const char *reason) const
{
	// The done flags are a prefix of the chain: a call needs the calls in front of it
	int doneCount = 0;
	while (doneCount < InitializationCallCount && initializationDone[doneCount])
	{
		++doneCount;
	}
	PrintCallList(out, INITIALIZATION_CALL_NAMES, INITIALIZATION_CALL_DESCRIPTIONS, InitializationCallCount, doneCount, markedCall, reason);
}

// The view of the chain after an initialization error, printed with the first error only
void Sandbox::Impl::ReportInitializationChain(int markedCall, const char *reason)
{
	if (initializationOrderPrinted)
	{
		return;
	}
	initializationOrderPrinted = true;
	std::cerr << std::endl;
	PrintInitializationChain(std::cerr, markedCall, reason);
	std::cerr << std::endl;
}

void Sandbox::Impl::PrintPipelines(std::ostream &out) const
{
	for (const PipelineView &pipeline : pipelineViews)
	{
		std::vector<std::string> objectNames = pipeline.objects;
		if (pipeline.engine)
		{
			// An object that was added to no pipeline is drawn with "pbr"
			for (const auto &object : objects)
			{
				bool added = false;
				for (const PipelineView &other : pipelineViews)
				{
					added = added || std::find(other.objects.begin(), other.objects.end(), object->name) != other.objects.end();
				}
				if (!added)
				{
					objectNames.push_back(object->name);
				}
			}
		}
		PrintPipeline(out, pipeline, objectNames);
		out << std::endl;
	}
}

void Sandbox::Impl::PrintFrameSequence(std::ostream &out, int markedCall, const char *reason) const
{
	// The calls in front of the expected one are done in this frame
	PrintCallList(out, FRAME_CALL_NAMES, FRAME_CALL_DESCRIPTIONS, FrameCallCount, expectedCall, markedCall, reason);
}

// The start-up print: the chain, every pipeline with its objects, the frame sequence
void Sandbox::Impl::PrintOverview(std::ostream &out) const
{
	out << std::endl;
	out << "Initialization chain (eight calls, in this order):" << std::endl;
	out << std::endl;
	PrintInitializationChain(out, -1, "");
	out << std::endl;
	PrintPipelines(out);
	out << "Frame sequence (six calls per frame, in this order):" << std::endl;
	out << std::endl;
	PrintFrameSequence(out, -1, "");
	out << std::endl;
}

// --- The initialization chain ---

// Checks that the first `callCount` calls of the chain are done and reports the first one that is not.
// A call of the chain requires the calls in front of it; Run() and the scene calls require all of them.
bool Sandbox::Impl::RequireInitialization(const char *caller, int callCount)
{
	for (int call = 0; call < callCount; ++call)
	{
		if (initializationDone[call])
		{
			continue;
		}
		std::cerr << "Initialization error: " << caller << "() was called, but " << INITIALIZATION_CALL_NAMES[call] << "() has not been done." << std::endl;
		ReportInitializationChain(call, "missing");
		return false;
	}
	return true;
}

// One call of the chain: the order check, then the work of the engine
bool Sandbox::Impl::InitializationCall(int call, const std::function<bool()> &work)
{
	if (initializationDone[call])
	{
		std::cerr << "Initialization error: " << INITIALIZATION_CALL_NAMES[call] << "() was called twice." << std::endl;
		ReportInitializationChain(call, "called twice");
		return false;
	}
	if (!RequireInitialization(INITIALIZATION_CALL_NAMES[call], call))
	{
		return false;
	}
	try
	{
		if (!work())
		{
			std::cerr << INITIALIZATION_CALL_NAMES[call] << "() failed." << std::endl;
			ReportInitializationChain(call, "failed");
			return false;
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << INITIALIZATION_CALL_NAMES[call] << "() failed: " << e.what() << std::endl;
		ReportInitializationChain(call, "failed");
		return false;
	}
	initializationDone[call] = true;
	return true;
}

// --- Sandbox ---

Sandbox::Sandbox() :
    impl(std::make_unique<Impl>())
{
	// Enable minidump generation for Release-only crashes (e.g., stack cookie failures / fast-fail).
	// Writes dumps under the current working directory (the build/run directory).
	CrashReporter::GetInstance().Initialize("crashes", "SimpleEngine", "1.0.0");
}

Sandbox::~Sandbox()
{
	CrashReporter::GetInstance().Cleanup();
}

bool Sandbox::InitializeWindow(const std::string &title, int width, int height)
{
	impl->title = title;
	return impl->InitializationCall(InitializeWindowCall, [&] { return impl->engine.InitializeWindow(title, width, height); });
}

bool Sandbox::CreateInstance()
{
	return impl->InitializationCall(CreateInstanceCall, [this] { return impl->engine.CreateInstance(impl->title, ENABLE_VALIDATION_LAYERS); });
}

bool Sandbox::PickDevice()
{
	return impl->InitializationCall(PickDeviceCall, [this] { return impl->engine.PickDevice(ENABLE_VALIDATION_LAYERS); });
}

bool Sandbox::CreateSwapChain()
{
	return impl->InitializationCall(CreateSwapChainCall, [this] { return impl->engine.CreateSwapChain(); });
}

bool Sandbox::InitializeRendering()
{
	return impl->InitializationCall(InitializeRenderingCall, [this] { return impl->engine.InitializeRendering(); });
}

bool Sandbox::CreatePipelines()
{
	if (!impl->InitializationCall(CreatePipelinesCall, [this] { return impl->engine.CreatePipelines(); }))
	{
		return false;
	}
	// For the terminal output: the engine's pipeline, the default of every object, with the shader
	// and the settings of the opaque PBR pipeline (Renderer::createPBRPipeline)
	impl->pipelineViews.push_back({"pbr", "shaders/pbr.slang", PipelineSettings{CullMode::Back, true, false}, true, {}});
	return true;
}

bool Sandbox::CreateCommandBuffers()
{
	return impl->InitializationCall(CreateCommandBuffersCall, [this] { return impl->engine.CreateCommandBuffers(); });
}

bool Sandbox::CreateSyncObjects()
{
	return impl->InitializationCall(CreateSyncObjectsCall, [this] { return impl->engine.CreateSyncObjects(); });
}

// --- The render loop ---

// Reports a frame call that was made out of order and stops rendering: IsRunning() is false from now on
bool Sandbox::Impl::FrameSequenceError(const char *caller)
{
	std::cerr << "Frame sequence error: " << caller << "() was called, " << FRAME_CALL_NAMES[expectedCall] << "() was expected. Rendering stopped." << std::endl;
	std::cerr << std::endl;
	PrintFrameSequence(std::cerr, expectedCall, "missing");
	std::cerr << std::endl;
	renderingStopped = true;
	return false;
}

// One frame call: the order check, then the work of the engine. After the last call of a frame the
// first one is expected again.
void Sandbox::Impl::FrameCall(int call, const std::function<void()> &work)
{
	if (renderingStopped)
	{
		return;
	}
	if (call != expectedCall)
	{
		FrameSequenceError(FRAME_CALL_NAMES[call]);
		return;
	}
	try
	{
		work();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
		std::cerr << std::endl;
		PrintFrameSequence(std::cerr, call, "failed");
		std::cerr << std::endl;
		renderingStopped = true;
		return;
	}
	expectedCall = (call + 1) % FrameCallCount;
}

bool Sandbox::IsRunning()
{
	if (impl->renderingStopped)
	{
		return false;
	}

	// The first call: what has to be there before the first frame
	if (!impl->loopStarted)
	{
		if (!impl->RequireInitialization("IsRunning", InitializationCallCount))
		{
			return false;
		}
		if (!impl->engine.GetActiveCamera())
		{
			std::cerr << "IsRunning(): there is no active camera. Create one with CreateCamera() and pass it to SetActiveCamera() in SetupScene()." << std::endl;
			return false;
		}
		impl->loopStarted = true;

		// The engine shows its loading overlay until a load cycle has ended. A scene without a
		// loaded model never starts one, so end it here.
		if (!impl->modelLoaded)
		{
			impl->engine.GetRenderer()->SetLoading(false);
		}

		// Terminal commands such as Room.Move(1, 0, 0): a reader thread collects the lines that are
		// typed, and the component applies them once per frame on the main thread.
		Entity *terminalEntity = impl->engine.CreateEntity("TerminalCommands");
		terminalEntity->AddComponent<TerminalCommandComponent>([this] { impl->ApplyTerminalCommands(); });
		impl->terminalCommands.Start();

		// The terminal output: the chain, every pipeline with its objects and the frame sequence,
		// once, after the start-up log and before the first frame
		impl->PrintOverview(std::cout);
	}

	// The previous frame has to be complete
	if (impl->expectedCall != BeginFrameCall)
	{
		return impl->FrameSequenceError("IsRunning");
	}

	try
	{
		return impl->engine.IsRunning();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
		impl->renderingStopped = true;
		return false;
	}
}

void Sandbox::BeginFrame()
{
	impl->FrameCall(BeginFrameCall, [this] { impl->engine.BeginFrame(); });
}

void Sandbox::UpdateScene()
{
	impl->FrameCall(UpdateSceneCall, [this] { impl->engine.UpdateScene(); });
}

void Sandbox::BeginRendering()
{
	impl->FrameCall(BeginRenderingCall, [this] { impl->engine.BeginRendering(); });
}

void Sandbox::DrawScene()
{
	impl->FrameCall(DrawSceneCall, [this] { impl->engine.DrawScene(); });
}

void Sandbox::EndRendering()
{
	impl->FrameCall(EndRenderingCall, [this] { impl->engine.EndRendering(); });
}

void Sandbox::EndFrame()
{
	impl->FrameCall(EndFrameCall, [this] { impl->engine.EndFrame(); });
}

Camera *Sandbox::CreateCamera(const std::string &name)
{
	// Create a camera entity
	Entity *cameraEntity = impl->engine.CreateEntity(name);

	// Add a transform component to the camera
	cameraEntity->AddComponent<TransformComponent>();

	// Add a camera component to the camera entity
	cameraEntity->AddComponent<CameraComponent>();
	// Camera aspect ratio will be set by the engine during initialization or resize events.

	auto camera    = std::make_unique<Camera>();
	camera->entity = cameraEntity;
	impl->cameras.push_back(std::move(camera));
	return impl->cameras.back().get();
}

void Sandbox::SetActiveCamera(Camera *camera)
{
	if (!camera)
	{
		return;
	}
	// Set the camera as the active camera
	impl->engine.SetActiveCamera(camera->entity->GetComponent<CameraComponent>());
}

Light *Sandbox::CreateLight(const std::string &name, LightType type)
{
	// Create a light entity
	Entity *lightEntity = impl->engine.CreateEntity(name);

	// Add a transform component to the light. A light shines along the -Z axis of its transform.
	lightEntity->AddComponent<TransformComponent>();

	// Add a light component to the light entity
	auto *lightComponent = lightEntity->AddComponent<LightComponent>();
	switch (type)
	{
		case LightType::Directional:
			lightComponent->SetType(ExtractedLight::Type::Directional);
			break;
		case LightType::Point:
			lightComponent->SetType(ExtractedLight::Type::Point);
			break;
		case LightType::Spot:
			lightComponent->SetType(ExtractedLight::Type::Spot);
			break;
	}

	auto light    = std::make_unique<Light>();
	light->entity = lightEntity;
	impl->lights.push_back(std::move(light));
	return impl->lights.back().get();
}

SceneObject *Sandbox::LoadModel(const std::string &name, const std::string &file)
{
	auto object  = std::make_unique<SceneObject>();
	object->name = name;

	if (impl->RequireInitialization("LoadModel", InitializationCallCount))
	{
		Renderer *renderer = impl->engine.GetRenderer();
		// The loader replaces the glTF lights of the renderer. Keep the lights of the models that
		// were loaded before, so that they can be put back together with the new ones.
		const std::vector<ExtractedLight> lightsBefore = renderer->GetStaticLights();
		renderer->SetStaticLights({});
		const size_t entityCountBefore = impl->engine.GetEntities().size();

		// The loading flags of the tutorial's main.cpp. The model is loaded right here instead of
		// on a background thread, so the entities exist when the call returns.
		renderer->SetLoading(true);
		renderer->SetLoadingPhase(Renderer::LoadingPhase::Textures);
		impl->modelLoaded = true;
		if (!LoadGLTFModel(&impl->engine, file, glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f)))
		{
			std::cerr << "LoadModel: \"" << file << "\" could not be loaded; the object \"" << name << "\" is empty" << std::endl;
		}

		// The parts of the object are the mesh entities that the loader created
		const auto &entities = impl->engine.GetEntities();
		for (size_t i = entityCountBefore; i < entities.size(); ++i)
		{
			if (entities[i]->GetComponent<MeshComponent>())
			{
				object->entities.push_back(entities[i].get());
			}
		}

		// A second model appends its glTF lights instead of replacing the list
		std::vector<ExtractedLight> lights = lightsBefore;
		const std::vector<ExtractedLight> &loadedLights = renderer->GetStaticLights();
		lights.insert(lights.end(), loadedLights.begin(), loadedLights.end());
		renderer->SetStaticLights(lights);
	}

	impl->objects.push_back(std::move(object));
	return impl->objects.back().get();
}

SceneObject *Sandbox::CreateSphere(const std::string &name, float radius)
{
	auto object  = std::make_unique<SceneObject>();
	object->name = name;

	if (impl->RequireInitialization("CreateSphere", InitializationCallCount))
	{
		Entity *sphereEntity = impl->engine.CreateEntity(name);
		sphereEntity->AddComponent<TransformComponent>();
		auto *mesh = sphereEntity->AddComponent<MeshComponent>();
		mesh->CreateSphere(radius);
		object->entities.push_back(sphereEntity);

		// The renderer creates the GPU buffers of a mesh only when it is asked to. Without this
		// request the sphere would never be drawn.
		impl->engine.GetRenderer()->EnqueueEntityPreallocationBatch({sphereEntity});
	}

	impl->objects.push_back(std::move(object));
	return impl->objects.back().get();
}

bool Sandbox::CreatePipeline(const std::string &name, const std::string &shaderFile, const PipelineSettings &settings)
{
	// The named pipelines copy the layout of the engine's PBR pipeline
	if (!impl->RequireInitialization("CreatePipeline", CreatePipelinesCall + 1))
	{
		return false;
	}
	if (!impl->engine.GetRenderer()->CreatePipeline(name, shaderFile, settings))
	{
		return false;
	}
	// For the terminal output
	impl->pipelineViews.push_back({name, shaderFile, settings, false, {}});
	return true;
}

bool Sandbox::AddToPipeline(const std::string &name, SceneObject *object)
{
	if (!object || !impl->RequireInitialization("AddToPipeline", InitializationCallCount))
	{
		return false;
	}
	auto *renderer = impl->engine.GetRenderer();
	for (Entity *entity : object->entities)
	{
		// An unknown name is reported by the renderer; one report is enough
		if (!renderer->AddToPipeline(name, entity))
		{
			return false;
		}
	}
	// For the terminal output: the object under its pipeline, once
	for (PipelineView &pipeline : impl->pipelineViews)
	{
		if (pipeline.name == name && std::find(pipeline.objects.begin(), pipeline.objects.end(), object->name) == pipeline.objects.end())
		{
			pipeline.objects.push_back(object->name);
		}
	}
	return true;
}
