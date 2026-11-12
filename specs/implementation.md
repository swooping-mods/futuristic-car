# Futuristic Courier: behavior and implementation

Describes the retained payload **0.2.1**. The behavior prose describes the retained payload; current candidate source and metadata are identified below. Gameplay qualification is a separate requirement.


<!-- source-only-sync -->
## Current source candidate

Source version **0.2.1** is saved from workshop revision `ca32bbf79ce0840d5166a232bdb3a9fdcd761254`. The retained installable payload remains **0.2.1** with its original bytes. This source update does not rebuild or requalify that payload. The current source and metadata define the candidate; gameplay, rendered behavior and co-op remain unqualified for rebuilt bytes.

The authored behavior and lifecycle descriptions below are retained from the published payload. They are not new qualification evidence for the source candidate. The integration table below follows the current candidate metadata.
<!-- /source-only-sync -->

## Behavior

Registers Futuristic Courier as vehicle ID 23, a $1,750 garage purchase with its own textured model and fitted collision hull. Vehicle Framework owns native catalog, garage, ownership and the shared hover/flight controller; this provider supplies tuning and presentation. Shift+F9 boards an eligible nearby owned hover car or toggles lift/land. W/S fly, A/D turn, Space/Ctrl climb/descend, Z/C strafe, Shift boosts and X brakes.

## State and lifecycle

Vehicle identity and ownership use native saved records. Each loaded instance owns its presentation tree; shared embedded mesh/texture resources are reused. The framework snapshots/restores body/gravity/constraint state around hover control and runs local flight only for the admitted owner. The provider retains its model, engine presentation and optional trailer tooling.

## Integration and configuration

Runtime dependencies: [vehicle-framework](https://github.com/swooping-mods/vehicle-framework).

The [module metadata](../source/mods/futuristic-car/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

| Service | ABI | Table size |
|---|---:|---|
| Requires `vehicle-framework.vehicles` | 1 | at least 48 bytes |

## Implementation sources

- [mod.c](../source/mods/futuristic-car/mod.c)
- [hover_profile.h](../source/mods/futuristic-car/hover_profile.h)
- [presentation.h](../source/mods/futuristic-car/presentation.h)
- [engine_core.h](../source/mods/futuristic-car/engine_core.h)

## Verification boundary

Keep Vehicle Framework and matching saved ID definitions available when reloading owned vehicles. Flight/collision, arbitrary slopes/obstacles, remote crews and cross-build behavior need matching game evidence; this is not an arbitrary vehicle importer.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
