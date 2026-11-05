"""Read-only diagnostic for an active PIE world."""
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
print("GAME_WORLD", world.get_path_name() if world else None)
if world:
    controllers = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.AIController)
    with unreal.ScopedSlowTask(len(controllers), "보스 AI 상태 조회") as task:
        for controller in controllers:
            task.enter_progress_frame(1)
            pawn = controller.get_controlled_pawn()
            if "Stone" not in controller.get_name() and (not pawn or "Stone" not in pawn.get_name()):
                continue
            print("BOSS", controller.get_name(), "PAWN", pawn.get_name() if pawn else None)
            components = controller.get_components_by_class(unreal.ActorComponent)
            with unreal.ScopedSlowTask(len(components), "보스 컴포넌트 조회") as subtask:
                for component in components:
                    subtask.enter_progress_frame(1)
                    name = component.get_class().get_name()
                    if any(word in name for word in ("BossBrain", "BehaviorTree", "StateTree", "TargetSelection", "Blackboard")):
                        print("COMPONENT", name, component.get_name())
                        if isinstance(component, unreal.LeeBossBrainComponent):
                            tree = component.get_editor_property("boss_behavior_tree")
                            print("BOSS_TREE", tree.get_path_name() if tree else None)
                        if isinstance(component, unreal.BrainComponent):
                            print("BRAIN_RUNNING", component.is_running(), "PAUSED", component.is_paused())
                        if isinstance(component, unreal.BlackboardComponent):
                            print("BB", "Target", component.get_value_as_object("TargetActor"), "Phase", component.get_value_as_int("Phase"), "Dead", component.get_value_as_bool("bIsDead"), "Groggy", component.get_value_as_bool("bIsGroggy"))
            if pawn:
                print("LOCATION", pawn.get_actor_location(), "VELOCITY", pawn.get_velocity())
                print("MOVE_STATUS", controller.get_move_status())
                montage = pawn.get_current_montage()
                print("MONTAGE", montage.get_name() if montage else None)
                movement = pawn.get_component_by_class(unreal.CharacterMovementComponent)
                if movement:
                    print("MOVEMENT", movement.get_editor_property("movement_mode"), movement.get_editor_property("max_walk_speed"))
    nav_class = getattr(unreal, "RecastNavMesh", None)
    if nav_class:
        nav = unreal.GameplayStatics.get_all_actors_of_class(world, nav_class)
        print("RECAST_NAVMESH_COUNT", len(nav))
