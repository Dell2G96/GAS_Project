"""Add reactive guard to the two existing Stone phase assets."""
import unreal

boss_ai = "/SoulsCore/Bot/Stone/BossAI/BT_Stone_Boss_v05"
phase1 = unreal.load_asset(boss_ai + "_Phase1")
phase2 = unreal.load_asset(boss_ai + "_Phase2")
guard = unreal.load_asset("/SoulsCore/Weapons/Stone_Hammer/Abilities/GA_Guard_Stone")
window = unreal.load_asset("/SoulsCore/Game/Abilities/GameplayEffect/Guard/GE_PerfectGuardWindow")
assert phase1 and phase2 and guard and window
guard_defaults = unreal.get_default_object(guard.generated_class())
if not guard_defaults.get_editor_property("perfect_guard_window_effect"):
    guard_defaults.set_editor_property("perfect_guard_window_effect", window.generated_class())
if guard_defaults.get_editor_property("perfect_guard_window") <= 0:
    native_defaults = unreal.get_default_object(unreal.LeeGameplayAbility_Guard)
    guard_defaults.set_editor_property("perfect_guard_window", native_defaults.get_editor_property("perfect_guard_window"))
unreal.BlueprintEditorLibrary.compile_blueprint(guard)
guard_defaults = unreal.get_default_object(guard.generated_class())
assert guard_defaults.get_editor_property("perfect_guard_window_effect")
assert guard_defaults.get_editor_property("perfect_guard_window") > 0
assert unreal.EditorAssetLibrary.save_loaded_asset(guard, only_if_is_dirty=False)

assert unreal.LeeBossTreeLibrary.add_reactive_guard_to_phase(phase1)
assert unreal.LeeBossTreeLibrary.add_reactive_guard_to_phase(phase2)
print("STONE_REACTIVE_GUARD_ASSETS_OK", guard_defaults.get_editor_property("perfect_guard_window"))
