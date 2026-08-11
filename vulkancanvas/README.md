# Vulkan-based canvas for ROOT

## Overview

This is an experimental implementation of [`TCanvasImp`](https://root.cern.ch/doc/master/classTCanvasImp.html) using [Vulkan](https://www.vulkan.org/) as the rendering backend.

It demonstrates that painting and interactivity handling can be performed in a modern GPU-accelerated environment, similar to the [raylib-based prototype](../README.md) but targeting the Vulkan API directly.

## Architecture

The implementation mirrors the raylib version's 3-class structure:

| Class | Role | Base class |
|-------|------|------------|
| `TVulkanGuiFactory` | Factory for creating canvas instances | `TGuiFactory` |
| `TVulkanCanvas` | Window management + render loop | `TCanvasImp` |
| `TVulkanPadPainter` | Drawing primitives (lines, boxes, text...) | `TPadPainterBase` |

Key differences from raylib:

- **Vulkan** is not immediate-mode — draw commands are collected into a retained buffer (`VkDrawCommands`) during pad paint, then recorded into a `VkCommandBuffer` each frame.
- **Windowing** uses SDL2 via its Vulkan WSI extension (`SDL_Vulkan_CreateSurface`).
- **Rendering pipeline** requires explicit shader compilation (GLSL → SPIR-V), descriptor sets, and render pass setup.
- **Text rendering** requires a glyph cache / atlas approach with Freetype or a library like [nanovg](https://github.com/memononen/nanovg).

## Dependencies

- **CERN ROOT** (6.28+) — components: Core, RIO, Hist, Gpad, Graf
- **Vulkan SDK** (1.0+) — headers and loader
- **SDL2** — windowing and input (via `SDL_vulkan.h`)
- **Freetype** (optional) — text rendering

## Build Instructions

```bash
mkdir build && cd build
cmake /path/to/vulkancanvas
make -j$(nproc)
```

## How to Use

Run ROOT from the build directory so it picks up `rootlogon.C`:

```bash
cd build
root -l hsimple.C
```

The `rootlogon.C` script registers the `vulkan` GUI factory plugin.

## What Is Missing (TODOs)

- [ ] Full swap chain initialization (format/present mode selection)
- [ ] GLSL shaders compilation (vertex + fragment for colored triangles)
- [ ] Render pass setup with alpha blending
- [ ] Command buffer recording for all primitive types
- [ ] Present queue submission (`vkQueuePresentKHR`)
- [ ] Text rendering pipeline (glyph atlas + Freetype)
- [ ] Multi-canvas support (per-canvas VkDevice)
- [ ] Window resize handling (swap chain recreation)
- [ ] Menu bar / toolbar UI elements
- [ ] Right-click context menu
- [ ] Graphics attribute editors
- [ ] Image save / export

## Comparison with Raylib Version

| Feature | raylib | vulkan |
|---------|--------|--------|
| Immediate mode | Yes | No (retained) |
| Windowing | Built-in | SDL2 |
| Shader setup | Implicit | Explicit (SPIR-V) |
| Text rendering | raylib builtin | Freetype + atlas |
| Complexity | Low | High |
| Performance potential | Good | Excellent |
| Portability | Cross-platform | Vulkan-capable GPUs |

## Contact

If you're interested in contributing to this project, contact the author(s).
