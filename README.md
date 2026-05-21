This repository is not being maintained and is here for the benefit of the public as none like it I could find to begin with as code relates to vector drawing.

# VectorKernelSDK

## Overview

**VectorKernelSDK** is a modular C++ graphics and interaction framework designed for building high-performance 2D vector-based applications. It demonstrates a modern architecture for rendering, entity management, interaction handling, and command-driven workflows using a collection of progressively more advanced examples.

The SDK is structured as a combination of reusable static libraries and standalone example applications that illustrate specific features such as rendering, snapping, selection, highlighting, dialogs, and command systems.

---

## What This Project Contains

### Core Libraries

These libraries provide the foundation of the SDK and are shared across all examples.

#### AppCoreLib
- Application framework and lifecycle management
- Input handling (mouse, keyboard, interaction states)
- Command system (e.g., HersheyCommand window integration)
- Selection and interaction scaffolding

#### EntityCoreLib
- Entity system (lines, primitives, IDs)
- Entity storage and indexing (EntityBook)
- Selection and hit-testing structures

#### RenderCoreLib
- OpenGL-based rendering pipeline
- GPU upload paths for geometry
- HUD vs scene rendering separation
- Efficient batching of primitives

---

### Example Applications

Each example builds on previous ones and demonstrates a specific feature or subsystem.

#### Early Rendering + Geometry
- **Example0001_RosettaDragon** — Reference dragon curve implementation
- **Example0002_DragonSegments** — Segment-based rendering
- **Example0003_DragonCurveGenerator** — Procedural generation

#### Grid and Scene Foundations
- **Example0004_GridInfinite** — Infinite grid rendering
- **Example0005_EntityBookOverflow** — Stress testing entity storage

#### Advanced Rendering
- **Example0007_GridInfiniteDragon** — High-performance dragon rendering with grid

#### Interaction + HUD
- **Example0009_Crosshairs** — Crosshair rendering and interaction
- **Example0010_SnapGridOnly** — Snap grid system
- **Example0011_SnapGridInfiniteDragon_SnapGridOnly** — Combined snapping + dragon rendering

#### Selection + Highlighting
- **Example0012_EntityHighlightHover_HudDragon** — Hover highlighting using HUD rendering
- **Example0013_PropertiesDialogSelectionStates** — Selection states + properties dialog

#### UI Integration
- **Example0014_LayersDialogHotkey** — Layers dialog via hotkey (Ctrl+L)
- **Example0015_HersheyCommandHosted** — Command window integration (decoupled from Application)

#### Command + Interaction Systems
- **Example0016_ZoomCommandsHudJigs** —
  - Zoom commands (`zoomex`, `zoomw`)
  - HUD-based temporary interaction tools ("HUD Jigs")
  - Window selection concepts

---

## Key Concepts Demonstrated

### 1. Entity-Based Architecture
All geometry is represented as entities stored in a centralized **EntityBook**, enabling:
- Efficient iteration
- Selection systems
- Rendering batching

### 2. Separation of Concerns
The SDK enforces a strong separation between:
- Core logic (AppCoreLib)
- Data model (EntityCoreLib)
- Rendering (RenderCoreLib)

This makes the system scalable and easier to extend.

### 3. HUD vs Scene Rendering
A critical design pattern:
- **Scene rendering** = persistent geometry
- **HUD rendering** = temporary overlays (crosshairs, highlights, selection windows)

This enables smooth interaction without polluting the main render state.

### 4. Command-Driven Interaction
Commands such as:
- `zoomex`
- `zoomw`

are handled through a command system similar to CAD-style workflows, integrated via the **HersheyCommand window**.

### 5. High-Performance Rendering
- GPU vertex buffers (VBOs)
- Batched line rendering
- Minimal per-frame allocations
- Designed to scale to hundreds of thousands of entities

### 6. Selection Systems (In Progress / Evolving)
- Single-pick selection
- Window selection
- Crossing selection
- Planned SelectionController abstraction

---

## What We Built (Current State)

At this stage, the SDK provides:

- A working **multi-library architecture**
- A **fast dragon curve renderer** with proper coloring
- A **HUD rendering system** for overlays
- A **crosshair system** integrated into interaction flow
- A **snap grid system**
- A **selection and highlighting foundation**
- A **command system with a hosted command window**
- Initial implementation of **zoom commands and HUD-based jigs**

It is evolving toward a CAD-like interaction model with:

- robust selection sets
- high-performance rendering at scale
- modular UI/dialog integration
- extensible command handling

---

## Building and Running

See the full build guide in:

```
VectorKernelSDK Build and Run Guide
```

Quick summary:

1. Open `VectorKernelSDK.sln` in Visual Studio 2022
2. Set configuration to **Debug | x64**
3. Right-click an example project
4. Choose **Set as Startup Project**
5. Press **F5**

---

## Recommended Starting Points

If you are new to the SDK:

- Start with **Example0004_GridInfinite** for rendering basics
- Move to **Example0007_GridInfiniteDragon** for performance
- Explore **Example0009_Crosshairs** for interaction
- Use **Example0015_HersheyCommandHosted** for command system
- Use **Example0016_ZoomCommandsHudJigs** for advanced workflows

---

## Intended Direction

This SDK is clearly moving toward a full-featured vector/CAD-style system with:

- Advanced selection models
- Layer management
- Property editing
- Command-driven workflows
- High-performance rendering at large scales

---

## Notes

- Always ensure the correct **startup project** is selected before running
- Prefer **Rebuild Solution** after major structural changes
- Use the provided cleaning script to reset the build environment if needed

---

## License / Usage

(Define as needed)

---

## Author / Maintainer

Project developed and iterated with a strong focus on:
- performance
- clarity of architecture
- incremental example-driven learning

---

## Final Thought

This is not just a set of examples — it is the foundation of a scalable vector graphics engine and interaction framework. Each example represents a deliberate step toward a much
