# Underdog Manager

Underdog Manager is a C++-first basketball club-management game targeting Unreal Engine 5.8 and Windows. The player inherits the weakest club in a fictional 12-team league and tries to rebuild it over a 22-game season.

## Current implementation

This repository contains a playable C++ management-game vertical slice through the Phase 10 release-candidate systems pass:

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
- native management screens for roster, schedule, standings, scouting, training, tactics, trades,
  playoffs, awards, offseason, saves, league history, contracts, rivalries, Match Center, and post-game recap
- multi-season progression with contracts, free agency, draft, development, chemistry, and AI management
- deterministic highlight-cue generation and text-based match presentation
- Phase 7 placeholder 3D broadcast playback with ten athletes, a ball, camera presets, scoreboard,
  pause, skip, speed, and exit-to-recap controls
- Phase 8 franchise operations with attendance, ticket pricing, game-day finances, fan support,
  four facility tracks, owner objectives, and owner confidence
- Phase 9 organization management with six staff roles, contracts, hiring, AI staffing,
  tactical familiarity, and coaching effects on development, scouting, and medical outcomes
- explainable trade evaluation with roster, salary-matching, package-size, and atomic-execution rules
- high-contrast broadcast-inspired management shell with dedicated Staff & Coaching workflows
- Phase 10 manager career progression with persistent records, season evaluations, reputation,
  employment status, explainable job offers, and club movement
- multi-season franchise settlement with media/commercial income and staff payroll accounting
- save schema version 9 and a deterministic ten-season career/economy soak test

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

Verified baseline: the UE 5.8 editor target builds successfully and all 45 registered `Underdog`
automation tests pass headlessly with `-NullRHI`, including the ten-season Phase 10 career,
roster, staff-payroll, and franchise-cash soak.

For command-line verification:

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "$PWD\UnderdogManager.uproject" `
  -ExecCmds="Automation RunTests Underdog; Quit" `
  -unattended -nop4 -NullRHI -log
```

## Phase 7 status

The current Phase 7 implementation is a code-only technical foundation. It provides deterministic
cue playback and broadcast controls using engine placeholder meshes. Production completion still requires:

- a dedicated arena map and authored court layout;
- licensed athlete, uniform, ball, arena, animation, audio, and crowd assets;
- event-specific staging paths and animation montages;
- packaged-build performance profiling and clean-machine verification;
- visual QA of every highlight template and camera transition.

If the asset spike fails, the approved fallback is a 2D tactical match presentation.
