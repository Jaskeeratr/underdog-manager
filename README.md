# Underdog Manager

Underdog Manager is a C++-first basketball club-management game targeting Unreal Engine 5.8 and Windows. The player inherits the weakest club in a fictional 12-team league and tries to rebuild it over a 22-game season.

## Current implementation

This repository currently contains the Gate 1 foundation:

- UE 5.8 project and runtime module boundaries
- deterministic, versioned random number generator
- stable player, team, schedule, match, and box-score types
- generated 12-team/180-player fictional league
- validated 22-game double round-robin schedule
- deterministic possession-based match simulator with overtime
- assists, steals, blocks, fouls, free throws, and explicit rebound events
- score and box-score reconciliation
- atomic six-game round advancement and ordered standings
- deterministic post-game fatigue, morale, form, injury, and recovery effects
- game-instance league orchestration
- asynchronous save snapshots and schema validation
- code-created ten-player highlight placeholder with skip support
- Unreal automation tests for determinism, schedule integrity, match integrity, and soak simulation

The project deliberately does not include marketplace content. Assets must be selected, licensed, and recorded before the 3D feasibility gate can be approved.

## Requirements

- Unreal Engine 5.8
- Visual Studio 2022 with Desktop development with C++ and Game development with C++
- Windows 10 or 11 SDK
- Git LFS before adding `.uasset`, `.umap`, animation, audio, or image content

The project is compiled and verified with Unreal Engine 5.8.0, MSVC 19.44, and Windows SDK 10.0.26100.0.

## Build

1. Install Unreal Engine 5.8 and the Visual Studio components above.
2. Right-click `UnderdogManager.uproject` and generate Visual Studio project files.
3. Build the `UnderdogManagerEditor` target for Win64 Development Editor.
4. Open the project and allow Unreal to compile the runtime and test modules.

## Tests

In the Unreal Editor, open **Tools > Test Automation**, search for `Underdog`, and run the suite.

Current verification: all five `Underdog` automation tests pass under UE 5.8 with `-NullRHI`.

For command-line verification:

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "$PWD\UnderdogManager.uproject" `
  -ExecCmds="Automation RunTests Underdog; Quit" `
  -unattended -nop4 -NullRHI -log
```

## Gate 1 completion criteria

Gate 1 is complete only after:

- the UE 5.8 editor target builds without warnings treated as errors;
- all `Underdog` automation tests pass;
- a placeholder court map stages made-shot, missed-shot, and turnover sequences;
- all sequences can be skipped safely;
- selected marketplace characters and animations share a compatible skeleton or retarget successfully;
- a packaged Windows build reproduces the spike.

If the asset spike fails, the approved fallback is a 2D tactical match presentation.
