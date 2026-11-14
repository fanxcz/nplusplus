# Paint Template

The `paint` template copies the repository's N++ Paint raster editor into `src/main.npp`. It uses the CPU-side canvas API, a virtual 1600×900 UI space and a mouse-driven tool panel.

## Create and run

```bash
npp new MyPaint --template paint
cd MyPaint
npp check
npp build --release
npp run
```

## Current features

The source implements pencil/brush, eraser, straight line, rectangle and fill tools; an on-screen color palette; adjustable brush size; undo/redo checkpoints; BMP save and load; fullscreen toggle; and a drawing canvas of 1024×720 pixels. The exact UI controls are defined by the checked-in `examples/npp_paint.npp`; do not rely on keyboard shortcut lists copied from older drafts.

The app writes `npp_paint.bmp` relative to its working directory. Choose a writable working folder if you plan to save drawings.

## Packaging

A standalone N++ Paint executable can be produced by building the `paint` template project on Windows x64. Keep the runtime DLLs and editable `.npp` source beside the executable when distributing it.

The canvas currently saves BMP. PNG export, layers, selections, vector paths and arbitrary image formats are not included in the current source.
