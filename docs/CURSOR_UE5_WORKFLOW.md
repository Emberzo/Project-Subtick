# Project Subtick — Cursor + Unreal Engine 5 workflow

## Layout

- **Game repo:** `ProjectSubtick/` (this repository) — `Source/`, `Config/`, `Content/`.
- **Engine source:** separate tree (e.g. `~/projects/UE5`) — never commit the engine.

## Day to day

1. **Edit C++** in Cursor under `Source/ProjectSubtick/` (including `AimTrainer/`).
2. **Open the `.uproject`** with your built `UnrealEditor` for maps, PIE, packaging, and “Compile” when the editor prompts after C++ changes.
3. **Terminal rebuild** (when the editor compile loop misbehaves):

```bash
/path/to/UE5/Engine/Build/BatchFiles/Linux/Build.sh ProjectSubtickEditor Linux Development -project="/path/to/ProjectSubtick/ProjectSubtick.uproject"
```

## CS2 import (console)

With PIE or standalone running:

```
subtick.ImportCs2
```

Merges Linux Steam userdata `*.vcfg` / `autoexec.cfg` (see `Cs2ConfigImporter`) into `UAimTrainerSettingsSubsystem` and applies `sensitivity`, `m_yaw`, `m_pitch`, `zoom_sensitivity_ratio_mouse`.

## Drills (in PIE)

- **1** — Static click  
- **2** — Micro flick  
- **3** — Target switch  
- **C** — Toggle calibration segment (rotate ~360°, toggle again; adjusts `FineTuneMultiplier` toward a full in-game turn)  
- **LMB** — Ray trace from camera center

## First-time engine build reminders

After a fresh UE source sync, build **`UnrealEditor`** and **`ShaderCompileWorker`** before expecting the editor to run shader compiles.
