# Example0004_GridInfinite

These are the step by steps necessary to bring online the infinite grid.

## Goal

Start from the new `Example001_OpenGLApp` class-oriented window/app structure and bring in the infinite background grid with panning and zooming.

## Step 1 - Make a direct copy of the base app project

Duplicate `Example001_OpenGLApp` into a new project folder named `Example0004_GridInfinite`.

That gives you these starting pieces immediately:

- `Application.{h,cpp}`
- `WinMain.cpp`
- `Win32AppWindow.{h,cpp}`
- `WindowTypes.h`
- the same OpenGL / WGL bootstrap and project references as the base demo

## Step 2 - Keep the new window helper code

Do **not** go back to manual `WNDCLASSEX` and manual centering logic inside `WinMain`.

Use the new window abstraction already present in the base app:

- `Win32AppWindow::RegisterWindowClass(...)`
- `Win32AppWindow::CreateCenteredWindow(...)`
- `Win32AppWindow::GetClientSize(...)`

This keeps all of the raw window sizing and centering logic in one place.

## Step 3 - Replace the fixed-view grid logic with camera-driven grid logic

The base OpenGL app builds a one-shot orthographic grid based on client aspect ratio.

For the infinite grid, switch the app to a camera-driven model using:

- `PanZoomController`
- `RenderCore::GridCache`
- `RenderCore::UpdateBackgroundGrid(...)`

That means `Application` now owns:

- current client size
- pan / zoom state
- grid cache state
- grid spacing and major line cadence

## Step 4 - Add the new members to `Application`

Bring these concepts into `Application.h`:

- `ClientSize m_clientSize`
- `PanZoomController m_panZoom`
- `RenderCore::GridCache m_gridCache`
- `float m_gridSpacingWorld`
- `int m_gridMajorEvery`

This is the minimum state needed to rebuild the visible part of the infinite grid only when required.

## Step 5 - Configure the camera during initialization

Inside `Application::Initialize()` set up the pan/zoom controller:

- center-origin ortho mode
- min/max zoom
- wheel zoom factor
- drag speed

Then call `ResetView()`.

That establishes the initial camera and forces the first grid rebuild.

## Step 6 - Rebuild the grid from the visible world extents

Add `RebuildGridIfNeeded()` to `Application.cpp` and call:

`RenderCore::UpdateBackgroundGrid(*m_book, m_panZoom, width, height, m_nextId, m_gridCache, m_gridSpacingWorld, m_gridMajorEvery)`

That routine:

- converts the viewport corners into world space
- expands the bounds slightly
- computes which grid cells are visible
- removes old `EntityTag::Grid` entities
- adds only the current visible vertical and horizontal lines
- adds the X/Y axes
- avoids rebuilding when the cached cell range has not changed

## Step 7 - Upload rebuilt line entities to the GPU

After a successful rebuild, call `UploadLinesToGpu()`.

That function walks the `EntityBook`, extracts visible `LineEntity` objects, and fills the VBO used by the existing line shader.

This keeps the render path simple.

## Step 8 - Change render to use the camera MVP

The fixed app used a hand-built orthographic matrix.

For the infinite grid, render with:

`const glm::mat4 mvp = m_panZoom.GetMVP();`

and pass that to `uMvp`.

This is the key change that makes panning and zooming affect the world view instead of rebuilding a static projection around the client aspect ratio.

## Step 9 - Add input handlers to `Application`

Add these methods:

- `OnResize(...)`
- `OnRButtonDown(...)`
- `OnRButtonUp()`
- `OnMouseMove(...)`
- `OnMouseWheel(...)`
- `ResetView()`

Purpose of each:

- `OnResize` updates viewport dimensions inside `PanZoomController`
- right mouse down starts drag-pan
- right mouse up ends drag-pan
- mouse move pans while right button is held
- mouse wheel zooms about the cursor position
- `ResetView` restores pan = 0 and zoom = 1

## Step 10 - Route Win32 messages into the app class

Inside `WndProc` wire the messages into those methods:

- `WM_SIZE` -> `OnResize`
- `WM_RBUTTONDOWN` -> `OnRButtonDown`
- `WM_RBUTTONUP` -> `OnRButtonUp`
- `WM_MOUSEMOVE` -> `OnMouseMove`
- `WM_MOUSEWHEEL` -> `OnMouseWheel`
- `WM_KEYDOWN` with `R` -> `ResetView`

This keeps camera behavior out of the message pump and inside the application layer.

## Step 11 - Keep the new class-oriented render loop

The render loop remains the same basic structure as the new base app:

1. pump messages
2. tick the frame timer
3. call `Application::Update(...)`
4. call `RenderOneFrame(hwnd)`
5. swap buffers

The important difference is that `RenderOneFrame` now asks `Win32AppWindow` for the current `ClientSize` and passes that into `Application::Render(...)`.

## Step 12 - Update the project files

Create a new Visual Studio project file named `Example0004_GridInfinite.vcxproj` and keep the same references to:

- `AppCoreLib`
- `EntityCoreLib`
- `RenderCoreLib`

Also include:

- `Win32AppWindow.cpp`
- `Win32AppWindow.h`
- `WindowTypes.h`

Those files are still part of the new demo because the new demo is intentionally built on top of the class-oriented window code.

## Controls

- **Right mouse drag** = pan
- **Mouse wheel** = zoom about cursor
- **R** = reset view

## Summary of what was actually brought online

The infinite grid feature comes online when these three parts are connected together:

1. **camera state** via `PanZoomController`
2. **grid generation** via `RenderCore::UpdateBackgroundGrid(...)`
3. **window/input plumbing** via the new `Win32AppWindow`-based `WinMain`

That is the clean path for bringing the old infinite-grid idea into the newer class-oriented OpenGL app base.
