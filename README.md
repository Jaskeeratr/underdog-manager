# Underdog Manager

Underdog Manager is a C++-first basketball club-management game targeting Unreal Engine 5.8 and Windows. The player inherits the weakest club in a fictional 12-team league and tries to rebuild it over multiple seasons.

## Current implementation

This repository contains a playable C++ management-game vertical slice with zero external asset dependencies:

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
- Unreal automation tests for determinism, schedule integrity, match integrity, and soak simulation
- native management screens for roster, schedule, standings, scouting, training, tactics, trades,
  playoffs, awards, offseason, saves, league history, contracts, rivalries, Match Center, and post-game recap
- multi-season progression with contracts, free agency, draft, development, chemistry, and AI management
- deterministic highlight-cue generation and text-based match presentation
- 2D tactical court viewer with animated player dots, ball trajectories, per-template choreography,
  broadcast-style scoreboard, and playback controls (pause, skip, speed, exit)
- franchise operations with attendance, ticket pricing, game-day finances, fan support,
  four facility tracks, owner objectives, and owner confidence
- organization management with six staff roles, contracts, hiring, AI staffing,
  tactical familiarity, and coaching effects on development, scouting, and medical outcomes
- explainable trade evaluation with roster, salary-matching, package-size, and atomic-execution rules
- high-contrast broadcast-inspired management shell with dedicated Staff & Coaching workflows
- manager career progression with persistent records, season evaluations, reputation,
  employment status, explainable job offers, and club movement
- multi-season franchise settlement with media/commercial income and staff payroll accounting
- save schema version 9 and a deterministic ten-season career/economy soak test

The project requires no marketplace or licensed content. All rendering is code-generated UMG.

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

Verified baseline: the UE 5.8 editor target builds successfully and all 47 registered `Underdog`
automation tests pass headlessly with `-NullRHI`, including the ten-season career,
roster, staff-payroll, and franchise-cash soak.

For command-line verification:

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "$PWD\UnderdogManager.uproject" `
  -ExecCmds="Automation RunTests Underdog; Quit" `
  -unattended -nop4 -NullRHI -log
```

## Matchday experience

When the player simulates a round, the Match Center opens for their next game showing a pre-game
comparison of both teams' lineups, records, tactics, injuries, and rivalry status. Two options:

- **Watch Highlights** launches a full-screen 2D tactical court viewer. A top-down basketball court
  shows 10 color-coded player dots and an orange ball animating through each highlight cue.
  Each highlight template (three-pointer, drive-and-finish, steal-fastbreak, clutch basket, etc.)
  has a unique movement choreography with bezier ball trajectories. A broadcast-style scoreboard
  tracks score, period, clock, and play descriptions in real time. Controls: pause, skip, 1x/2x/4x speed, exit.

- **Instant Result** skips the replay and goes straight to the results screen.

After the broadcast finishes (or on exit), the Post-Game Recap screen shows the highlight feed,
quarter-by-quarter scores, key plays, and headline summary.
