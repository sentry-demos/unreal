#!/usr/bin/env python3
"""
Fix for Sentry Issue UNREAL-V: Wire up BP_SentryTower_ProjectileWrath in W_LevelUp widget

This script adds the missing Wrath projectile class reference to the W_LevelUp widget
blueprint, fixing the null pointer crash when the Wrath attack modifier is selected.

To run this script in Unreal Editor:
1. Open the SentryTower project in Unreal Engine 5.8
2. Go to Window > Developer Tools > Output Log
3. In the Python console at the bottom, run: import fix_wrath_projectile
   OR
4. Run this from the command line:
   UnrealEditor.exe "/path/to/SentryTower.uproject" -run=pythonscript -script="/path/to/fix_wrath_projectile.py"
"""

import unreal

def fix_wrath_projectile_in_widget():
    """
    Wire up the BP_SentryTower_ProjectileWrath class in the W_LevelUp widget.
    Returns True if successful, False otherwise.
    """
    
    unreal.log("Starting fix for Wrath projectile in W_LevelUp widget...")
    
    # Paths to the assets
    widget_path = "/Game/UI/W_LevelUp"
    wrath_projectile_path = "/Game/Blueprints/Player/BP_SentryTower_ProjectileWrath"
    
    # Load the widget blueprint class
    unreal.log(f"Loading widget blueprint from: {widget_path}")
    widget_asset = unreal.EditorAssetLibrary.load_asset(widget_path)
    
    if not widget_asset:
        unreal.log_error(f"Failed to load widget asset: {widget_path}")
        return False
    
    # Load the Wrath projectile blueprint class  
    unreal.log(f"Loading Wrath projectile from: {wrath_projectile_path}")
    wrath_projectile_asset = unreal.EditorAssetLibrary.load_asset(wrath_projectile_path)
    
    if not wrath_projectile_asset:
        unreal.log_error(f"Failed to load Wrath projectile asset: {wrath_projectile_path}")
        return False
    
    # Get the generated class from the blueprint
    wrath_projectile_class = wrath_projectile_asset.generated_class()
    
    if not wrath_projectile_class:
        unreal.log_error("Failed to get generated class from Wrath projectile blueprint")
        return False
    
    unreal.log(f"Successfully loaded Wrath projectile class: {wrath_projectile_class.get_name()}")
    
    # Get the widget blueprint's generated class
    widget_class = widget_asset.generated_class()
    
    if not widget_class:
        unreal.log_error("Failed to get generated class from widget blueprint")
        return False
    
    # Get the default object (CDO) from the widget class
    widget_cdo = unreal.get_default_object(widget_class)
    
    # Common variable names for the Wrath projectile class in the widget
    possible_variable_names = [
        "WrathProjectileClass",
        "ProjectileWrathClass",
        "WrathProjectile",
        "ProjectileWrath",
        "Wrath_Projectile_Class",
        "ProjectileTypeWrath"
    ]
    
    # Try to find and set the variable
    found = False
    for var_name in possible_variable_names:
        if hasattr(widget_cdo, var_name):
            unreal.log(f"Found variable: {var_name}")
            setattr(widget_cdo, var_name, wrath_projectile_class)
            found = True
            break
    
    if not found:
        unreal.log_warning(f"Could not find Wrath projectile variable in widget blueprint")
        unreal.log_warning("You may need to add the variable manually in the Blueprint Editor")
        unreal.log_warning("Available variables:")
        for attr in dir(widget_cdo):
            if not attr.startswith('_'):
                unreal.log_warning(f"  - {attr}")
        return False
    
    # Save the modified asset
    unreal.log("Saving modified widget asset...")
    success = unreal.EditorAssetLibrary.save_loaded_asset(widget_asset, False)
    
    if success:
        unreal.log("✓ Successfully wired up Wrath projectile in W_LevelUp widget!")
        unreal.log(f"✓ Variable {var_name} now references {wrath_projectile_class.get_name()}")
        return True
    else:
        unreal.log_error("Failed to save widget asset")
        return False

def main():
    """Main entry point for the script."""
    result = fix_wrath_projectile_in_widget()
    
    if result:
        print("\n" + "="*60)
        print("SUCCESS: W_LevelUp widget has been fixed!")
        print("The Wrath button will now correctly set the projectile type.")
        print("="*60 + "\n")
    else:
        print("\n" + "="*60)
        print("MANUAL FIX REQUIRED:")
        print("1. Open Content/UI/W_LevelUp in the Blueprint Editor")
        print("2. Find the Wrath button's OnClicked event")
        print("3. In the event graph, find the 'Set Projectile Type' node")
        print("4. Set the 'Projectile Type' input to:")
        print("   BP_SentryTower_ProjectileWrath")
        print("5. Compile and save the blueprint")
        print("="*60 + "\n")
    
    return result

if __name__ == "__main__":
    main()
