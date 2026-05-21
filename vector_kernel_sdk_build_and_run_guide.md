# VectorKernelSDK Build and Run Guide

## Overview

This guide explains, in detail, how to open, restore, build, and run the **VectorKernelSDK** solution in Visual Studio on Windows. It also explains how to choose and change the **start-up project**, which is important because the solution contains multiple example applications and static libraries.

The instructions below assume:

- You are using **Windows**.
- You are using **Visual Studio 2022**.
- You have the SDK extracted locally, for example to:

```text
C:\Users\vecto\Downloads\VectorKernelSDK_Part1\VectorKernelSDK_Part1
```

- The solution file is:

```text
VectorKernelSDK.sln
```

---

## 1. What is in this solution

The solution is made up of two general kinds of projects:

### Static library projects
These are reusable code libraries that are built first and then linked into the example applications.

Typical library projects in your tree are:

- `AppCoreLib`
- `EntityCoreLib`
- `RenderCoreLib`

These normally do **not** run by themselves. They build into `.lib` files that the example applications use.

### Example application projects
These are the runnable example programs.

From your current folder listing, examples include:

- `Example0001_RosettaDragon`
- `Example0002_DragonSegments`
- `Example0003_DragonCurveGenerator`
- `Example0004_GridInfinite`
- `Example0005_EntityBookOverflow`
- `Example0007_GridInfiniteDragon`
- `Example0009_Crosshairs`
- `Example0010_SnapGridOnly`
- `Example0011_SnapGridInfiniteDragon_SnapGridOnly`
- `Example0012_EntityHighlightHover_HudDragon`
- `Example0013_PropertiesDialogSelectionStates`
- `Example0014_LayersDialogHotkey`
- `Example0015_HersheyCommandHosted`
- `Example0016_ZoomCommandsHudJigs`

These are the projects you typically choose as the **start-up project** when you want to launch and test a specific example.

---

## 2. Prerequisites

Before building, make sure the following are installed.

### Visual Studio
Install **Visual Studio 2022** with at least the following workload:

- **Desktop development with C++**

It is also helpful to have these individual components installed:

- MSVC v143 build tools
- Latest Windows 10 or Windows 11 SDK
- CMake tools for Windows (optional, but commonly installed)
- Git for Windows support (optional)

### vcpkg
Your solution contains a `vcpkg.json` file, which usually means dependencies are managed through **vcpkg manifest mode**.

That means Visual Studio may automatically restore required packages when the solution is opened, provided vcpkg integration is set up correctly.

If vcpkg is not already integrated on your machine, you may need to do that once.

---

## 3. Folder layout check

After extracting the zip, verify that the solution root looks approximately like this:

```text
VectorKernelSDK_Part1\
    AppCoreLib\
    Applications\
    EntityCoreLib\
    Examples\
    RenderCoreLib\
    vcpkg.json
    VectorKernelSDK.sln
```

Inside `Examples`, you should see the example folders.

This matters because if the zip is extracted with an extra top-level folder or into the wrong place, Visual Studio may still open the solution, but relative paths can become confusing when troubleshooting build output.

---

## 4. Opening the solution in Visual Studio

### Method A: From File Explorer
1. Open File Explorer.
2. Navigate to the SDK root folder.
3. Double-click `VectorKernelSDK.sln`.

### Method B: From inside Visual Studio
1. Open **Visual Studio 2022**.
2. Choose **File > Open > Project/Solution**.
3. Browse to:

```text
C:\Users\vecto\Downloads\VectorKernelSDK_Part1\VectorKernelSDK_Part1\VectorKernelSDK.sln
```

4. Select `VectorKernelSDK.sln`.
5. Click **Open**.

When the solution opens, Visual Studio will usually begin:

- loading all projects,
- evaluating configurations,
- checking package restore status,
- and possibly restoring vcpkg dependencies.

Give it a moment to settle before building.

---

## 5. Allow package restore to complete

If this solution uses vcpkg manifest mode, Visual Studio may show messages such as:

- restoring packages,
- configuring dependencies,
- or waiting for external package resolution.

Let that finish before assuming the build is broken.

If package restore fails, common causes are:

- vcpkg not installed,
- vcpkg integration not enabled,
- missing internet access during restore,
- an unsupported toolchain setup,
- or Visual Studio missing required C++ components.

---

## 6. Choose the correct Solution Configuration and Platform

At the top of Visual Studio, you will usually see two dropdowns:

- **Solution Configurations**
- **Solution Platforms**

Typical values are:

- Configuration: `Debug` or `Release`
- Platform: `x64`

For this SDK, you will usually want:

- **Debug** for development and troubleshooting
- **x64** as the platform

So a safe default is:

- **Configuration:** `Debug`
- **Platform:** `x64`

If `x64` is not visible:
1. Click the platform dropdown.
2. Choose **Configuration Manager**.
3. Under **Active solution platform**, select `x64` if present.
4. If it is missing, create or copy it from another platform.

In most modern Windows desktop builds, you should avoid `Win32` unless the project was explicitly designed for 32-bit builds.

---

## 7. How to build the entire solution

To build everything:

### Option A: Build from the menu
1. Click **Build** on the top menu.
2. Click **Build Solution**.

Shortcut:

```text
Ctrl + Shift + B
```

### What this does
Visual Studio will:

1. Build the library projects first.
2. Build each example project that is included in the active solution configuration.
3. Link the applications against the static libraries.

### What success looks like
You want to see something like:

- `Build: X succeeded, 0 failed`

The **Output** window is usually the best place to watch this.

Open it with:

- **View > Output**

Then in the Output window, choose:

- **Show output from: Build**

---

## 8. How to rebuild everything from scratch

If you suspect stale object files, old libraries, or mismatched binaries, do a full rebuild.

### Clean and rebuild from Visual Studio
1. Click **Build**.
2. Click **Clean Solution**.
3. Wait for that to finish.
4. Then click **Build > Rebuild Solution**.

### When to use Rebuild
Use **Rebuild Solution** when:

- project references changed,
- header files changed in shared libraries,
- linker errors seem inconsistent,
- an example still behaves like old code,
- or you moved/copy-pasted project folders and want a truly fresh build.

---

## 9. How to build only one example project

Sometimes you do not want to build every example. You may only want to build a single example, such as:

- `Example0016_ZoomCommandsHudJigs`

### To build one project only
1. In **Solution Explorer**, find the desired example project.
2. Right-click the project.
3. Click **Build**.

This builds that project and any project dependencies it requires.

That is often faster than building the whole solution.

---

## 10. How to set the start-up project

This is one of the most important parts when working with a multi-project solution.

The **start-up project** is the project Visual Studio will launch when you press:

- **F5** for Start Debugging
- **Ctrl + F5** for Start Without Debugging

If the wrong project is set as start-up, pressing F5 may do nothing useful, or it may try to run a static library, which cannot be launched.

### The right kind of project to choose
Choose an **example application project**, not a library project.

Good candidates include:

- `Example0015_HersheyCommandHosted`
- `Example0016_ZoomCommandsHudJigs`
- or whichever example you are actively testing

Do **not** choose these as start-up projects:

- `AppCoreLib`
- `EntityCoreLib`
- `RenderCoreLib`

Those are libraries, not executable applications.

### Method 1: Set a single project as start-up
1. In **Solution Explorer**, locate the example project you want to run.
2. Right-click that project.
3. Click **Set as Startup Project**.

After that, the project name will typically appear in **bold** in Solution Explorer.

That bold project is now the one that launches when you press F5.

### Example
If you want to run the zoom command example:

1. In Solution Explorer, right-click `Example0016_ZoomCommandsHudJigs`.
2. Click **Set as Startup Project**.
3. Press **F5**.

Visual Studio will then launch that example instead of another one.

---

## 11. How to verify which project is currently the start-up project

There are a few easy ways.

### In Solution Explorer
The start-up project is normally shown in **bold** text.

### In the toolbar
The run/debug target often reflects the currently selected launchable project.

### By pressing F5
If the wrong app launches, the wrong start-up project is probably selected.

---

## 12. How to set multiple start-up projects

Usually you only want one start-up project. But Visual Studio also allows multiple start-up projects.

You probably do **not** need this for the SDK examples, but here is how it works.

1. Right-click the **solution** in Solution Explorer.
2. Click **Properties**.
3. Under **Common Properties**, click **Startup Project**.
4. Choose **Multiple startup projects**.
5. Set actions such as **Start** or **None** beside each project.

For this SDK, the normal choice is still:

- **Single startup project**

because you usually want to launch just one example window at a time.

---

## 13. How to run the selected example

Once the correct start-up project is set:

### Start with debugger attached
Press:

```text
F5
```

This is best when:

- debugging crashes,
- using breakpoints,
- inspecting variables,
- tracing message flow,
- or stepping through command handling.

### Start without debugger
Press:

```text
Ctrl + F5
```

This is useful when:

- you just want to test the app,
- you do not need breakpoints,
- you want cleaner runtime behavior,
- or you want the console/output window to remain open after exit.

---

## 14. What to do if Build Solution succeeds but F5 will not run

This usually means one of a few things.

### Cause 1: A library is set as the start-up project
Fix:
- Right-click the desired example app.
- Choose **Set as Startup Project**.

### Cause 2: The project type is not an application
Check that the selected project is an example executable and not a static library.

### Cause 3: Configuration mismatch
Make sure:
- the project is included in the active solution configuration,
- the platform is `x64`,
- and the example actually built successfully.

### Cause 4: Startup item is excluded from build
Open **Configuration Manager** and verify the example project is checked under **Build**.

---

## 15. How to use Configuration Manager

Configuration Manager is very useful in large solutions.

Open it from:

- **Build > Configuration Manager**

Here you can verify:

- the active solution configuration,
- the active solution platform,
- whether each project is checked to build,
- and whether each project maps to the correct platform.

### Recommended checks
For the example you want to run:

- **Build** should be checked.
- **Platform** should usually be `x64`.

For the library projects:

- they should also be checked, because the example depends on them.

---

## 16. How project dependencies work in this solution

The example applications depend on the library projects.

That means when you build an example, Visual Studio should first build dependencies such as:

- `AppCoreLib`
- `EntityCoreLib`
- `RenderCoreLib`

If dependencies are wired correctly, you do **not** need to build those manually first.

However, if you ever suspect dependency wiring problems, you can inspect them.

### To inspect dependencies
1. Right-click the **solution**.
2. Choose **Project Dependencies**.
3. Select the example project.
4. Confirm the required libraries are checked as dependencies.

---

## 17. Where the build outputs usually go

Your solution appears to use common Visual Studio output folders such as:

- `x64\Debug\`
- `x64\Release\`
- project-local `x64` folders in some cases

Depending on each `.vcxproj`, output may land either:

- at the solution level,
- in a project folder,
- or in a shared `x64\Debug` path.

If an `.exe` builds successfully but you are unsure where it went:

1. Right-click the example project.
2. Click **Properties**.
3. Go to:
   - **Configuration Properties > General**
   - **Configuration Properties > Debugging**
4. Check values such as:
   - **Output Directory**
   - **Target Name**
   - **Working Directory**
   - **Command**

That will tell you exactly what Visual Studio is producing and launching.

---

## 18. How to inspect project properties for one example

For any example project:

1. Right-click the project.
2. Click **Properties**.

Useful property pages include:

### General
Shows:
- output directories,
- intermediate directories,
- target extension,
- platform toolset,
- and Windows SDK version.

### C/C++
Shows compiler settings such as:
- include directories,
- preprocessor definitions,
- warning levels,
- language standard,
- optimization settings.

### Linker
Shows linker settings such as:
- additional library directories,
- linked `.lib` files,
- subsystem type,
- output file name.

### Debugging
Shows launch settings such as:
- command,
- working directory,
- arguments,
- environment.

This area is extremely helpful when diagnosing why one example runs differently than another.

---

## 19. How to open Solution Explorer if it is missing

If you do not see the project tree:

- Click **View > Solution Explorer**

Shortcut:

```text
Ctrl + Alt + L
```

This is where you will do most of the work for:

- setting the start-up project,
- building a specific example,
- inspecting project properties,
- and browsing source files.

---

## 20. How to tell whether an example is an executable project

A runnable example project usually has:

- source files like `Application.cpp`, `main.cpp`, or similar,
- linker settings for an `.exe`,
- and a project icon/type indicating an application.

A static library project usually builds a `.lib` and cannot be run directly.

If unsure, right-click the project, choose **Properties**, and inspect:

- **Configuration Properties > General > Configuration Type**

Common values:

- **Application (.exe)** = runnable
- **Static Library (.lib)** = not directly runnable

---

## 21. Recommended day-to-day workflow

A practical workflow for this SDK is:

1. Open `VectorKernelSDK.sln`.
2. Wait for package restore to finish.
3. Set configuration to **Debug** and platform to **x64**.
4. In Solution Explorer, right-click the example you want to test.
5. Choose **Set as Startup Project**.
6. Press **Ctrl + Shift + B** to build.
7. Fix any compile or linker errors.
8. Press **F5** to run under the debugger.
9. Repeat as needed.

For example, if you are working on zoom commands:

1. Set `Example0016_ZoomCommandsHudJigs` as the start-up project.
2. Build that project.
3. Run it.
4. Test `zoomex` and `zoomw` behavior.

---

## 22. Recommended workflow after changing shared library code

If you edited shared code in:

- `AppCoreLib`
- `EntityCoreLib`
- `RenderCoreLib`

then the safest workflow is:

1. Save all files.
2. Build the solution or rebuild the active example.
3. If behavior seems stale, do **Rebuild Solution**.
4. Run the desired example again.

This is especially important when changing:

- entity structures,
- rendering paths,
- interaction state,
- command handling,
- selection logic,
- dialog integration,
- or cross-library headers.

---

## 23. How to use the cleaning script before a fresh build

If you want a very clean tree before building:

1. Close Visual Studio.
2. Put the cleaning script at the solution root.
3. Right-click PowerShell and choose **Run as needed**.
4. Navigate to the root folder containing the script.
5. Run the script.
6. Re-open the solution.
7. Let package restore happen again if needed.
8. Build the solution.

This helps when:

- old build outputs are causing confusion,
- timestamps need normalization,
- you are preparing a clean zip,
- or you want to verify the project can build from a near-pristine state.

---

## 24. Common problems and what they usually mean

### Problem: `Cannot open include file`
Usually means:
- missing include directory,
- package restore did not complete,
- or project/library configuration mismatch.

### Problem: unresolved external symbol
Usually means:
- a required `.lib` was not built,
- a dependency is missing,
- a function is declared but not defined,
- or a project reference is not set correctly.

### Problem: the wrong example runs
Usually means:
- the wrong **start-up project** is selected.

### Problem: nothing happens when pressing F5
Usually means:
- a static library is selected as startup,
- build failed earlier,
- or the selected project is not launchable.

### Problem: build succeeds but app behaves like old code
Usually means:
- stale outputs or intermediate files,
- wrong configuration/platform,
- wrong executable being launched,
- or a rebuild is needed.

---

## 25. Exact steps to set `Example0016_ZoomCommandsHudJigs` as the start-up project

Because you specifically mentioned start-up project selection, here is the exact sequence in plain terms.

1. Open `VectorKernelSDK.sln` in Visual Studio.
2. Open **Solution Explorer** if it is not already visible.
3. Expand the `Examples` area if needed.
4. Find `Example0016_ZoomCommandsHudJigs`.
5. Right-click `Example0016_ZoomCommandsHudJigs`.
6. Click **Set as Startup Project**.
7. Confirm the project name becomes **bold**.
8. At the top of Visual Studio, set:
   - **Debug**
   - **x64**
9. Click **Build > Build Solution**.
10. Press **F5** to run it under the debugger.

That is the standard sequence you will use over and over when testing one example at a time.

---

## 26. Exact steps to switch from one example to another

Suppose you were running `Example0015_HersheyCommandHosted` and now want to run `Example0016_ZoomCommandsHudJigs`.

Do this:

1. Stop debugging if the old app is still running.
2. In Solution Explorer, right-click `Example0016_ZoomCommandsHudJigs`.
3. Click **Set as Startup Project**.
4. Build the solution or build just that project.
5. Press **F5**.

That is all you need to switch the launched example.

---

## 27. Suggested note for this SDK

Because this solution contains many examples and several shared libraries, it is a good habit to always check these three things before pressing F5:

1. **Am I on Debug or Release?**
2. **Am I on x64?**
3. **Is the correct example set as the start-up project?**

Those three checks prevent a lot of wasted time.

---

## 28. Minimal quick-start version

If you just want the shortest correct sequence:

1. Open `VectorKernelSDK.sln` in Visual Studio 2022.
2. Wait for restore/package setup to finish.
3. Set configuration to **Debug** and platform to **x64**.
4. Right-click the example you want, such as `Example0016_ZoomCommandsHudJigs`.
5. Choose **Set as Startup Project**.
6. Press **Ctrl + Shift + B**.
7. Press **F5**.

---

## 29. Final recommendation

For active development, I would usually recommend:

- **Startup Project:** the example you are currently testing
- **Configuration:** `Debug`
- **Platform:** `x64`
- **Build command:** `Ctrl + Shift + B`
- **Run command:** `F5`

And whenever behavior seems inconsistent:

- do **Clean Solution**,
- then **Rebuild Solution**,
- then confirm the correct start-up project is still selected.

