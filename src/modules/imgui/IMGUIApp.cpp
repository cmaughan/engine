/**
 * @file
 */

#include "IMGUIApp.h"

#include "io/Filesystem.h"
#include "core/command/Command.h"
#include "core/Color.h"
#include "core/UTF8.h"
#include "core/Common.h"
#include "video/Renderer.h"
#include "video/ScopedViewPort.h"
#include "IMGUI.h"

namespace imgui {

IMGUIApp::IMGUIApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, uint16_t traceport) :
		Super(filesystem, eventBus, timeProvider, traceport), _camera(video::CameraType::FirstPerson, video::CameraMode::Orthogonal) {
}

IMGUIApp::~IMGUIApp() {
}

void IMGUIApp::onMouseWheel(int32_t x, int32_t y) {
	if (y > 0) {
		_mouseWheel = 1;
	} else if (y < 0) {
		_mouseWheel = -1;
	}
	Super::onMouseWheel(x, y);
}

void IMGUIApp::onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) {
	ImGuiIO& io = ImGui::GetIO();
	io.MousePos = ImVec2(x, y);
	Super::onMouseMotion(x, y, relX, relY);
}

void IMGUIApp::onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) {
	if (button == SDL_BUTTON_LEFT) {
		_mousePressed[0] = true;
	} else if (button == SDL_BUTTON_RIGHT) {
		_mousePressed[1] = true;
	} else if (button == SDL_BUTTON_MIDDLE) {
		_mousePressed[2] = true;
	}
	Super::onMouseButtonPress(x, y, button, clicks);
}

bool IMGUIApp::onTextInput(const std::string& text) {
	ImGuiIO& io = ImGui::GetIO();
	io.AddInputCharactersUTF8(text.c_str());
	return true;
}

bool IMGUIApp::onKeyPress(int32_t key, int16_t modifierUnused) {
	ImGuiIO& io = ImGui::GetIO();
	key &= ~SDLK_SCANCODE_MASK;
	core_assert(key >= 0 && key < (int)SDL_arraysize(io.KeysDown));
	io.KeysDown[key] = true;
	const int16_t modifier = SDL_GetModState();
	io.KeyShift = (modifier & KMOD_SHIFT) != 0;
	io.KeyCtrl  = (modifier & KMOD_CTRL) != 0;
	io.KeyAlt   = (modifier & KMOD_ALT) != 0;
	io.KeySuper = (modifier & KMOD_GUI) != 0;
	return Super::onKeyPress(key, modifierUnused);
}

bool IMGUIApp::onKeyRelease(int32_t key) {
	ImGuiIO& io = ImGui::GetIO();
	key &= ~SDLK_SCANCODE_MASK;
	core_assert(key >= 0 && key < (int)SDL_arraysize(io.KeysDown));
	io.KeysDown[key] = false;
	const int16_t modifier = SDL_GetModState();
	io.KeyShift = (modifier & KMOD_SHIFT) != 0;
	io.KeyCtrl  = (modifier & KMOD_CTRL) != 0;
	io.KeyAlt   = (modifier & KMOD_ALT) != 0;
	io.KeySuper = (modifier & KMOD_GUI) != 0;
	return Super::onKeyRelease(key);
}

void IMGUIApp::onWindowResize() {
	Super::onWindowResize();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)_dimension.x, (float)_dimension.y);

	_camera.init(glm::ivec2(0), dimension());
	_camera.update(0L);
	video::ScopedShader scoped(_shader);
	_shader.setProjection(_camera.projectionMatrix());
}

core::AppState IMGUIApp::onConstruct() {
	const core::AppState state = Super::onConstruct();
	return state;
}

static const char* _getClipboardText(void*) {
	const char* text = SDL_GetClipboardText();
	if (!text) {
		return nullptr;
	}
	const int len = strlen(text);
	if (len == 0) {
		SDL_free((void*) text);
		return "";
	}
	static ImVector<char> clipboardBuffer;
	// Optional branch to keep clipboardBuffer.capacity() low:
	if (len <= clipboardBuffer.capacity() && clipboardBuffer.capacity() > 512) {
		ImVector<char> emptyBuffer;
		clipboardBuffer.swap(emptyBuffer);
	}
	clipboardBuffer.resize(len + 1);
	strcpy(&clipboardBuffer[0], text);
	SDL_free((void*) text);
	return (const char*) &clipboardBuffer[0];
}

static void _setClipboardText(void*, const char* text) {
	SDL_SetClipboardText(text);
}

core::AppState IMGUIApp::onInit() {
	const core::AppState state = Super::onInit();
	video::checkError();
	if (state != core::AppState::Running) {
		return state;
	}

	_renderUI = core::Var::get(cfg::ClientRenderUI, "true");

	if (!_shader.setup()) {
		Log::error("Could not load the ui shader");
		return core::AppState::Cleanup;
	}

	_vbo.setMode(video::VertexBufferMode::Stream);
	_bufferIndex = _vbo.create();
	if (_bufferIndex < 0) {
		Log::error("Failed to create ui vertex buffer");
		return core::AppState::Cleanup;
	}
	_indexBufferIndex = _vbo.create(nullptr, 0, video::VertexBufferType::IndexBuffer);
	if (_indexBufferIndex < 0) {
		Log::error("Failed to create ui index buffer");
		return core::AppState::Cleanup;
	}

	_camera.setNearPlane(-1.0f);
	_camera.setFarPlane(1.0f);
	_camera.init(glm::ivec2(0), _dimension);
	_camera.update(0L);

	video::Attribute attributeColor;
	attributeColor.bufferIndex = _bufferIndex;
	attributeColor.index = _shader.getLocationColor();
	attributeColor.size = _shader.getComponentsColor();
	attributeColor.stride = sizeof(ImDrawVert);
	attributeColor.offset = offsetof(ImDrawVert, col);
	attributeColor.type = video::DataType::UnsignedByte;
	attributeColor.normalized = true;
	_vbo.addAttribute(attributeColor);

	video::Attribute attributeTexCoord;
	attributeTexCoord.bufferIndex = _bufferIndex;
	attributeTexCoord.index = _shader.getLocationTexcoord();
	attributeTexCoord.size = _shader.getComponentsTexcoord();
	attributeTexCoord.stride = sizeof(ImDrawVert);
	attributeTexCoord.offset = offsetof(ImDrawVert, uv);
	_vbo.addAttribute(attributeTexCoord);

	video::Attribute attributePosition;
	attributePosition.bufferIndex = _bufferIndex;
	attributePosition.index = _shader.getLocationPos();
	attributePosition.size = _shader.getComponentsPos();
	attributePosition.stride = sizeof(ImDrawVert);
	attributePosition.offset = offsetof(ImDrawVert, pos);
	_vbo.addAttribute(attributePosition);

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)_dimension.x, (float)_dimension.y);
	io.Fonts->AddFontDefault();

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	_texture = video::genTexture();
	video::bindTexture(video::TextureUnit::Upload, video::TextureType::Texture2D, _texture);
	video::setupTexture(video::TextureType::Texture2D, video::TextureWrap::None);
	video::uploadTexture(video::TextureType::Texture2D, video::TextureFormat::RGBA, width, height, pixels, 0);
	io.Fonts->TexID = (void *) (intptr_t) _texture;

	io.KeyMap[ImGuiKey_Tab] = SDLK_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
	io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
	io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
	io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
	io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
	io.KeyMap[ImGuiKey_Delete] = SDLK_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = SDLK_BACKSPACE;
	io.KeyMap[ImGuiKey_Enter] = SDLK_RETURN;
	io.KeyMap[ImGuiKey_Escape] = SDLK_ESCAPE;
	io.KeyMap[ImGuiKey_A] = SDLK_a;
	io.KeyMap[ImGuiKey_C] = SDLK_c;
	io.KeyMap[ImGuiKey_V] = SDLK_v;
	io.KeyMap[ImGuiKey_X] = SDLK_x;
	io.KeyMap[ImGuiKey_Y] = SDLK_y;
	io.KeyMap[ImGuiKey_Z] = SDLK_z;
	io.RenderDrawListsFn = nullptr;
	io.SetClipboardTextFn = _setClipboardText;
	io.GetClipboardTextFn = _getClipboardText;
	io.ClipboardUserData = nullptr;

#ifdef _WIN32
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(_window, &wmInfo);
	io.ImeWindowHandle = wmInfo.info.win.window;
#endif
	SDL_StartTextInput();

	Log::info("Set up imgui");

	return state;
}

core::AppState IMGUIApp::onRunning() {
	core::AppState state = Super::onRunning();

	if (state != core::AppState::Running) {
		return state;
	}

	core_assert(_bufferIndex > -1);
	core_assert(_indexBufferIndex > -1);

	{
		core_trace_scoped(IMGUIAppBeforeUI);
		beforeUI();
	}

	const bool renderUI = _renderUI->boolVal();
	if (!renderUI) {
		return core::AppState::Running;
	}

	ImGuiIO& io = ImGui::GetIO();
	const int renderTargetW = (int) (io.DisplaySize.x * io.DisplayFramebufferScale.x);
	const int renderTargetH = (int) (io.DisplaySize.y * io.DisplayFramebufferScale.y);
	// Avoid rendering when minimized, scale coordinates for
	// retina displays (screen coordinates != framebuffer coordinates)
	if (renderTargetW == 0 || renderTargetH == 0) {
		return core::AppState::Running;
	}

	io.DeltaTime = _deltaFrame / 1000.0f;
	const uint32_t mouseMask = SDL_GetMouseState(nullptr, nullptr);
	// If a mouse press event came, always pass it as "mouse held this frame",
	// so we don't miss click-release events that are shorter than 1 frame.
	io.MouseDown[0] = _mousePressed[0] || (mouseMask & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
	io.MouseDown[1] = _mousePressed[1] || (mouseMask & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
	io.MouseDown[2] = _mousePressed[2] || (mouseMask & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
	_mousePressed[0] = _mousePressed[1] = _mousePressed[2] = false;

	io.MouseWheel = (float)_mouseWheel;
	_mouseWheel = 0;

	ImGui::NewFrame();

	SDL_ShowCursor(io.MouseDrawCursor ? SDL_FALSE : SDL_TRUE);

	core_trace_scoped(IMGUIAppUpdateUI);
	video::ScopedShader scopedShader(_shader);
	_shader.setProjection(_camera.projectionMatrix());
	_shader.setTexture(video::TextureUnit::Zero);

	video::ScopedViewPort scopedViewPort(0, 0, renderTargetW, renderTargetH);
	video::scissor(0, 0, renderTargetW, renderTargetH);

	video::enable(video::State::Blend);
	video::disable(video::State::DepthTest);
	video::enable(video::State::Scissor);
	video::disable(video::State::CullFace);
	video::blendFunc(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha);
	video::blendEquation(video::BlendEquation::Add);

	{
		core_trace_scoped(IMGUIAppOnRenderUI);
		onRenderUI();
	}
	ImGui::Render();

	ImDrawData* drawData = ImGui::GetDrawData();
	drawData->ScaleClipRects(io.DisplayFramebufferScale);

	for (int n = 0; n < drawData->CmdListsCount; ++n) {
		const ImDrawList* cmdList = drawData->CmdLists[n];
		const ImDrawIdx* idxBufferOffset = nullptr;

		core_assert_always(_vbo.update(_bufferIndex, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert)));
		core_assert_always(_vbo.update(_indexBufferIndex, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx)));
		core_assert_always(_vbo.bind());

		for (int i = 0; i < cmdList->CmdBuffer.Size; ++i) {
			const ImDrawCmd* cmd = &cmdList->CmdBuffer[i];
			if (cmd->UserCallback) {
				cmd->UserCallback(cmdList, cmd);
			} else {
				video::bindTexture(video::TextureUnit::Zero, video::TextureType::Texture2D, (video::Id)(intptr_t)cmd->TextureId);
				const ImVec4& cr = cmd->ClipRect;
				video::scissor(cr.x, cr.y, cr.z - cr.x, cr.w - cr.y);
				video::drawElements<ImDrawIdx>(video::Primitive::Triangles, cmd->ElemCount, (void*)idxBufferOffset);
			}
			idxBufferOffset += cmd->ElemCount;
		}
		_vbo.unbind();
	}

	video::scissor(0, 0, renderTargetW, renderTargetH);
	return core::AppState::Running;
}

core::AppState IMGUIApp::onCleanup() {
	ImGui::Shutdown();
	_shader.shutdown();
	_vbo.shutdown();
	_indexBufferIndex = -1;
	_bufferIndex = -1;
	return Super::onCleanup();
}

}
