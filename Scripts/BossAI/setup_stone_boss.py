"""Unreal Editor Python: create versioned BT/ST assets and wire the Stone controller.
Run with -run=pythonscript -script=<absolute path> after compiling the editor target.
"""
import json
from pathlib import Path
import unreal

asset = unreal.EditorAssetLibrary
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/SoulsCore/Bot/Stone", "/SoulsCore/Weapons/Stone_Hammer", "/SoulsCore/Game/Enemys"], True)
base = "/SoulsCore/Bot/Stone/BossAI"
tree = unreal.LeeBossTreeLibrary.build_boss_trees(base, "BT_Stone_Boss")
if not tree:
    raise RuntimeError("Boss BT/ST generation failed; inspect StateTree compiler log")

roar_path = "/SoulsCore/Weapons/Stone_Hammer/Abilities/GA_Stone_Boss_Roar"
if asset.does_asset_exist(roar_path):
    roar = unreal.load_asset(roar_path)
else:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.LeeGameplayAbility_BossRoar)
    roar = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "GA_Stone_Boss_Roar", roar_path.rsplit("/", 1)[0], unreal.Blueprint, factory)
if not roar:
    raise RuntimeError("Roar blueprint creation failed")
unreal.BlueprintEditorLibrary.compile_blueprint(roar)
assert asset.save_loaded_asset(roar)

ability_set = unreal.load_asset("/SoulsCore/Game/Enemys/AbilitySet_Stone_Boss_Souls")
entries = ability_set.get_editor_property("granted_gameplay_abilities")
if not any(entry.get_editor_property("ability") == roar.generated_class() for entry in entries):
    assert unreal.LeeEnemySetupLibrary.grant_ability_to_set(ability_set, roar.generated_class(), 1, False)
assert asset.save_loaded_asset(ability_set)
controller = unreal.load_asset("/SoulsCore/Bot/Stone/B_AI_Stone_Boss_Souls")
component_path = base + "/B_StoneBossBrainComponent"
if asset.does_asset_exist(component_path):
    component = unreal.load_asset(component_path)
else:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.LeeBossBrainComponent)
    component = unreal.AssetToolsHelpers.get_asset_tools().create_asset("B_StoneBossBrainComponent", base, unreal.Blueprint, factory)
assert component
set_path = "/SoulsCore/Experiences/LAS_StoneBossAI"
if asset.does_asset_exist(set_path):
    action_set = unreal.load_asset(set_path)
else:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.LeeExperienceActionSet)
    action_set = unreal.AssetToolsHelpers.get_asset_tools().create_asset("LAS_StoneBossAI", "/SoulsCore/Experiences", unreal.LeeExperienceActionSet, factory)
experience_bp = unreal.load_asset("/SoulsCore/Experiences/B_LeeSoulsCoreGame_Boss")
experience = unreal.get_default_object(experience_bp.generated_class())
assert unreal.LeeBossTreeLibrary.configure_boss_experience(controller, component, tree, experience, action_set)
assert asset.save_loaded_asset(experience_bp)
report = {"tree": tree.get_path_name(), "controller": controller.get_path_name(),
          "component": component.get_path_name(), "experience": experience_bp.get_path_name(),
          "action_set": action_set.get_path_name(), "roar": roar.get_path_name(),
          "selection": "StateTree weighted tags -> GA random AttackDataList", "pie_tested": False}
output = Path(unreal.Paths.project_saved_dir()) / "StoneBossSetup.json"
output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
for attached_set in experience.get_editor_property("action_sets"):
    unreal.log(attached_set.get_path_name() + " " + unreal.LeeBossTreeLibrary.describe_component_actions(attached_set))
unreal.log("STONE_BOSS_SETUP_OK " + json.dumps(report))