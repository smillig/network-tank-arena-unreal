# Network Tank Arena

Student starter for the Network Programming lab. Requires **Unreal Engine 5.7.4**
and the Windows C++ toolchain required by Unreal. Open `NetworkTankArena.uproject`
with UE 5.7.4 and build the project if prompted. The startup level is `TestMap`.

W/S move forward/backward, A/D turn the chassis, and the mouse aims the turret
independently. Weapons are not included in this lab baseline.

To run two players locally:

1. Open `TestMap` and select **Play → Advanced Settings**.
2. Set **Number of Players = 2** and **Net Mode = Play As Listen Server**.
3. Use **New Editor Window (PIE)** and enable **Run Under One Process**.
4. Select **Default Player Start**, then Play. Focus each player window in turn
   to control that player's tank. Shift+F1 releases the mouse from PIE.
5. Each window should see both tanks move and aim. Stop PIE and repeat.

Movement is server-authoritative without prediction; the client waits for server
updates, so some input delay and stepped motion are expected.

For a packaged **Development** build, start two copies. In the first copy, open
the console with tilde and enter `HostLocalhost`. In the second, enter
`JoinLocalhost`. This uses `127.0.0.1:7777`; start the host before joining.
These commands start from standalone play and require `TestMap` in the package.
No account or online service is required.

# Assignment

Choose a focus
Choose one main networking direction that is large enough to be meaningful but small enough to investigate properly
during the course. You may combine related ideas when that makes sense, but do not treat the examples below as a
checklist.
- Responsiveness and tank movement: Interpolation, smoothing, prediction, reconciliation, correction behaviour,
chassis movement or turret rotation under latency.
- Combat and validation: Authoritative projectiles or hitscan, server-side fire validation, hit confirmation, projectile
spawning, or lag compensation.
- Replication efficiency: Update frequency for chassis/turret state, projectile replication, reducing unnecessary
traffic, relevancy/interest management, or lower-frequency state.
- Game and player state: Join-in-progress, match phases, score/state consistency, respawn state,
reconnect/disconnect behaviour, or other persistent multiplayer state.
- Networked gameplay: A new objective, pickup/power-up, team mechanic, weapon mechanic, or other feature
where authority and synchronization matter.
- Advanced synchronization: Networked physics, moving/interactive world objects, destructible elements, or
another technically challenging synchronized system.
- Diagnostics and profiling: Network statistics, profiling, debug overlays, reproducible test scenarios, or a
measurable networking bottleneck.
- Student-proposed direction: A different networking problem or feature is welcome if the scope is suitable. Check
the idea with the teacher early.