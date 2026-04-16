// Copyright Project Subtick. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWorld;

/** Spawns a minimal playable arena (geometry + sky + lights + player start) entirely from code. */
namespace AimTrainerProceduralWorld
{
/** Idempotent per world: safe to call every match start. */
PROJECTSUBTICK_API void EnsureArena(UWorld* World);
}
