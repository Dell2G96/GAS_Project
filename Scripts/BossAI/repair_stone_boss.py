"""Restore the five branches on the existing v05 BT. Run with PIE stopped."""
import unreal

path = "/SoulsCore/Bot/Stone/BossAI/BT_Stone_Boss_v05"
tree = unreal.load_asset(path)
phase1 = unreal.load_asset(path + "_Phase1")
phase2 = unreal.load_asset(path + "_Phase2")
assert tree and phase1 and phase2

library = unreal.LeeBossTreeLibrary
if library.get_boss_branch_count(tree) == 0:
    assert library.repair_empty_boss_tree(tree, phase1, phase2), "Repair failed"
assert library.get_boss_branch_count(tree) == 5, "Expected five runtime branches"
unreal.log("STONE_BOSS_BRANCHES_OK " + tree.get_path_name())
