"""Reload saved Blueprint component templates without actor placement or level saves."""
import json
from pathlib import Path
import unreal

saved = Path(unreal.Paths.project_saved_dir())
setup = json.loads((saved / "StoneBossSetup.json").read_text(encoding="utf-8"))
tree = unreal.load_asset(setup["tree"])
bp = unreal.load_asset(setup["controller"])
roar = unreal.load_asset(setup["roar"])
assert tree and bp and roar
assert unreal.LeeBossTreeLibrary.get_boss_branch_count(tree) == 5, "Boss BT must have five runtime branches"
subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
lib = unreal.SubobjectDataBlueprintFunctionLibrary
handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
objects = [lib.get_object_for_blueprint(lib.get_data(h), bp) for h in handles]
brains = [o for o in objects if isinstance(o, unreal.LeeBossBrainComponent)]
assert len(brains) == 0, "Direct BossBrain must be removed from controller"
component_bp = unreal.load_asset(setup["component"])
brain = unreal.get_default_object(component_bp.generated_class())
assert brain.get_editor_property("boss_behavior_tree") == tree
experience_bp = unreal.load_asset(setup["experience"])
experience = unreal.get_default_object(experience_bp.generated_class())
action_set = unreal.load_asset(setup["action_set"])
assert action_set in experience.get_editor_property("action_sets")
connections = unreal.LeeBossTreeLibrary.describe_component_actions(action_set)
assert setup["controller"].split(".")[0] + ".B_AI_Stone_Boss_Souls_C" in connections
assert component_bp.generated_class().get_path_name() in connections
assert "server=1 client=0" in connections
ability_set = unreal.load_asset("/SoulsCore/Game/Enemys/AbilitySet_Stone_Boss_Souls")
entries = ability_set.get_editor_property("granted_gameplay_abilities")
assert sum(e.get_editor_property("ability") == roar.generated_class() for e in entries) == 1
assert unreal.get_default_object(roar.generated_class()).get_editor_property("roar_montage") is None
result = {"direct_component_removed": True, "experience_action_set_saved": True,
          "runtime_branch_count": 5,
          "server_only_injection": True, "connection": connections,
          "selection": "Shinobi weighted tag selection -> GA random montage",
          "roar_granted_once": True, "pie_combat_tested": False}
(saved / "StoneBossVerification.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("STONE_BOSS_VERIFY_OK " + json.dumps(result))
