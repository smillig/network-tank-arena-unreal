# Assignment notes:
Problem:
An improvement on gameplay could be weapon upgrades that spawn in the level and can be picked up to give a temporary weapon upgrade for a limitied time. Weapon upgrades could also respawn after a given time.

Baseline:
There is basic move and shoot gameplay.

Plan:
Implement server side spawning of weapon upgrades. Create replicated event when picked up, server is authoritative for if upgrade is touched/picked up. 
Upgrade status is server authoritative, upgrade is applied only to pickup owner. Speed of shot or number of spawned projectiles is also server authoritative.

Evidence:
Spawns of items will be handled on the server and replicated to the client. OnCollision with the pickup will enable ability on the server and replicate to the client. Only owning pawn will have ability. Ability will timeout based on timer on server. Upgrade respawn will also be handled on the server and replicated to clients.

Possible Extras:
Investigate if interpolation, prediction or reconciliation would enhance feature.

## Project / feature summary:
• How to run and test the project.
Download or change to shaun-assignment branch. Open uproject or build and run in 2 player listen server mode in new window PIE.

• What you implemented or changed.
All code can be seen in git diffs or by searching for comments that have the prefix "Change:"
I implemented a rapid fire and spreadshot weapon upgrade. The upgrades are pickups that spawn at an item spawner I also made.
This reqired a few changes in the TankWeaponComponent and the TankPlayerController. I added a struct and some variables to TankWeaponComponent for client and server tracking and some modifications to TankPlayerController so that the rapid fire weapon felt like an actual upgrade.
New classes for the spawner is in WeaponUpgradeSpawnerActor and upgrade is in WeaponUpgradeActor.

• Important client/server, authority and replication decisions.
Server remains authoritative for when upgrades are applied and time out. Controller is extended so firing latch can be bypassed if rapid fire upgrade is active. TryConsume was also moved and subsequent checks were placed into a helper function so multiple projectiles could be spawned on the server per shot if spread shot upgrade is active. Ultimatly SubmitLocalFireRequest is still used to request server to fire so same limitations of original firing apply.
For spawner server is authoritative for spawning and connecting to delegate function of upgrade being collided with/ picked up.
The upgrade itself is also server authoritative. On collision of a tank with the upgrade server authority is checked before the upgrade is destroyed, the upgrade applied to corresponding tank and finally delegate is broadcast for spawner to begin timer.
To prevent the item from being picked up on the same frame a bool is used to mark the item consumed so whomever triggers first will get the pickup and upgrade.

• The network conditions used for testing.
I used the builtin Network Emulation system to set everyone to a "bad" connection. This resulted in the pickups still working but having intermittent client issues as packet loss still resulted in shots not being fired.

• At least one networking/synchronization problem you encountered and how you diagnosed it.
Bullets from clients can still get lost in the case of a client request packet getting lost. This was confirmed using the network emulation system and setting it to "bad." Server requests for this are set as reliable so I'm not sure why they are still lost. I assume the request is lost and no resending is invoked. I know TCP has resend and ack packets but I don't know enough about UDP to know if there's a proper mechanisim to truly be "reliable."

• The networking improvement or optimization you made, including why you chose it.
Firing became hold to fire when the rapid fire upgrade is active. This perhaps wasn't a strict improvement but it felt a lot better gameplay wise. I think the bConsumed bool is a great optimization as it's a classic issue in netwoked games to be able to dupe items and while this might not fully prevent duplication it is a good step against it.

• Known limitations, trade-offs or remaining problems
The same issues with the original firing are still present. Projectiles are prone to jitter, some can not get spawned based on packet loss.

## Bonus Content

- A cursory attempt at interpolation, prediction and reconciliation

I felt that I didn't have a firm enough grasp on the difference between interpolation, prediction and reconciliation. So I quickly did some questioning with some AI to help clerify. While I can't take credit for the code it did help me gain a bit more of an understanding of these concpets. 

Interpolation:

This happens on the client for other clients/ game replication actors that update at a lower rate than the framerate. Since in this game the other client tank is updating at 20 hz and the framerate is 60 we have 3 frames between updates that are lost and can potentially look a bit choppy. So we lerp over the 0.05 seconds from one update to the next.

Prediction and Reconciliation

These go hand in hand since if we predict on the client we need to server to confirm and correct. The client knows what it's attempting so we take that intent and put it in a buffer and simulate it. We also send this intent to the server. The server checks these and if it's out of spec then corrects. This is helpful since usually a server has the most reliable and high bandwidth connection so the updates to the clients can remain lower but frequent updates are sent from the client and assumed correct unless they need correcting so rather than everything sending at full 60 hz we do save some bandwidth at the expense of the server doing extra work, as well as the client sending a bit more.

I think this gave me a much better synopsis of these three concepts. If you have any corrections or additions I'm eager to hear feedback.

# Network Tank Arena

Student starter for the Network Programming lab. Requires **Unreal Engine 5.7.4**
and the Windows C++ toolchain required by Unreal. Open `NetworkTankArena.uproject`
with UE 5.7.4 and build the project if prompted. The startup level is `TestMap`.

W/S move forward/backward, A/D turn the chassis, and the mouse aims the turret
independently. Left mouse sends one discrete fire request per fresh press;
the server fires replicated projectiles at a persistent damage-test cube.
Enhanced Input is configured through **IA_Fire**,
the fire mapping in **IMC_Tank**, and **BP_TankPlayerController's Fire Action**.
The existing asset setup and earlier local firing behavior were manually verified in UE 5.7.4.
Release the focus-acquiring click before firing; that specific capture behavior
has not been verified. See [Authoritative network combat](docs/NETWORK_COMBAT.md)
for the current contract, automation and manual checks. Network combat and the
target readout still require manual UE 5.7.4 verification. Tank health is not implemented.

To run two players locally:

1. Open `TestMap` and select **Play → Advanced Settings**.
2. Set **Number of Players = 2** and **Net Mode = Play As Listen Server**.
3. Use **New Editor Window (PIE)** and enable **Run Under One Process**.
4. Select **Default Player Start**, then Play. Focus each player window in turn
   to control that player's tank. Shift+F1 releases the mouse from PIE.
5. Each window should see both tanks move and aim. Stop PIE and repeat.

Movement is server-authoritative without prediction; the client waits for server
updates, so some input delay and stepped motion are expected.

Thursday Lab 2 starts with [Network conditions](docs/LAB_2_NETWORK_CONDITIONS.md):
use its PIE setup and impairment profiles to observe this movement baseline.
Friday Lab 3 adds reliable fire requests, server collision/damage, replicated
projectiles and target totals. Projectile presentation deliberately has no prediction
or custom interpolation.

For a packaged **Development** build, start two copies. In the first copy, open
the console with tilde and enter `HostLocalhost`. In the second, enter
`JoinLocalhost`. This uses `127.0.0.1:7777`; start the host before joining.
These commands start from standalone play and require `TestMap` in the package.
No account or online service is required.