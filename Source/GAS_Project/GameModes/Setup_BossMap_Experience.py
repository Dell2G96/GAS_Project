"""
보스 맵 DefaultGameplayExperience 설정 스크립트
실행 조건: LeeWorldSetting.h가 EditAnywhere로 변경된 후 리빌드 완료 시
실행 방법: UE Editor Output Log > Python Console에서 실행
"""

import unreal

# L_BossMap 열기 (이미 열려있으면 skip)
world = unreal.EditorLevelLibrary.get_editor_world()
if 'L_BossMap' not in world.get_path_name():
    print("L_BossMap을 먼저 열어주세요: /SoulsCore/Maps/L_BossMap")
else:
    ws = world.get_world_settings()
    
    # Boss Experience Class 설정
    boss_class = unreal.load_class(None, '/SoulsCore/Experiences/B_LeeSoulsCoreGame_Boss.B_LeeSoulsCoreGame_Boss_C')
    ws.set_editor_property('DefaultGameplayExperience', boss_class)
    
    result = ws.get_editor_property('DefaultGameplayExperience')
    print(f"DefaultGameplayExperience = {result}")
    
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.save_current_level()
    print("L_BossMap 저장 완료!")
    print("\n보스 맵 PIE 테스트:")
    print("  - Net Mode: Listen Server (설정됨)")
    print("  - Players: 4명 (설정됨)")
    print("  - 기대 결과: 3명 접속 → 4번째는 'Boss session is full'")
