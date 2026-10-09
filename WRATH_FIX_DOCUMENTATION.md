# Fix for Sentry Issue UNREAL-V: Wrath Button Crash

This document describes the fix for the null pointer dereference crash that occurs when selecting the Wrath attack modifier.

## Root Cause

The crash occurred due to two issues:
1. **C++ Code Issue**: `ASentryTowerTurret::Shoot()` did not check if `SpawnActor()` returned null before dereferencing the projectile pointer
2. **Blueprint Issue**: The W_LevelUp widget's Wrath button did not have the `BP_SentryTower_ProjectileWrath` class wired up, causing `SetProjectileType()` to receive null

## Fixes Applied

### 1. C++ Fix (Required - Prevents Crash)

File: `Source/SentryTower/Player/SentryTowerTurret.cpp`

Added a null check after spawning the projectile:

```cpp
void ASentryTowerTurret::Shoot(AActor* TargetActor, const FVector& TargetLocation)
{
    // ... existing code ...
    
    auto Projectile = 
        Cast<ASentryTowerProjectile>(GetWorld()->SpawnActor(ProjectileType, &SpawnLocation, &SpawnRotation));

    // NEW: Check if projectile was successfully spawned
    if (!Projectile)
    {
        return;
    }

    Projectile->TargetToFollow = TargetActor;
    Projectile->TargetStationary = TargetLocation;
    Projectile->Init();
    
    // ... existing code ...
}
```

This prevents the crash by returning early if the projectile fails to spawn.

### 2. Blueprint Fix (Recommended - Makes Wrath Button Functional)

The W_LevelUp widget needs to reference the Wrath projectile class. There are two ways to apply this fix:

#### Option A: Run the Python Script (Automated)

1. Open the SentryTower project in Unreal Engine 5.8
2. Go to Window > Developer Tools > Python
3. Run the script: `Content/Python/fix_wrath_projectile.py`

OR from command line:
```bash
UnrealEditor.exe "SentryTower.uproject" -run=pythonscript -script="Content/Python/fix_wrath_projectile.py"
```

#### Option B: Manual Fix in Blueprint Editor

1. Open `Content/UI/W_LevelUp` in the Blueprint Editor
2. In the event graph, find the Wrath button's OnClicked event handler
3. Locate the node that calls `Set Projectile Type`
4. Set the `Projectile Type` input parameter to: `BP_SentryTower_ProjectileWrath`
5. Compile and save the blueprint

## Testing

To verify the fix:

1. Launch SentryTower (Switch Development build) on the Main map
2. Kill enemies until the level-up menu opens
3. Click the **Wrath** attack modifier button
4. Wait for the turret to face an enemy and fire
5. Verify:
   - Game does NOT crash (C++ fix working)
   - Wrath projectiles are spawned (blueprint fix working)

## Technical Details

**Crash Location**: `SentryTowerTurret.cpp:72`
```
Projectile->TargetToFollow = TargetActor;
```

**Crash Type**: Null pointer dereference (accessing memory at 0x2d8 = null + field offset)

**Call Stack**:
```
ASentryTowerTurret::Shoot()
  └─ GetWorld()->SpawnActor(ProjectileType, ...) returns null
     └─ ProjectileType is null (not set by W_LevelUp widget)
```

**Breadcrumb Trail**: Every crash was preceded by the breadcrumb "Player selected attack modifier Wrath"

## Files Modified

- `Source/SentryTower/Player/SentryTowerTurret.cpp` - Added null check
- `Content/Python/fix_wrath_projectile.py` - Added automated fix script (new file)
- `Content/UI/W_LevelUp.uasset` - Needs manual/scripted update to wire Wrath projectile

## Related Issues

- Sentry Issue: UNREAL-V
- Issue URL: https://demo.sentry.io/issues/7410432589/
