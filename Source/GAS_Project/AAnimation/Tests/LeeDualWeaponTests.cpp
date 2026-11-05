#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Materials/Material.h"
#include "GAS_Project/AEquipment/LeeEquipmentMeshSelection.h"
#include "GAS_Project/AEquipment/LeeEquipmentManagerComponent.h"
#include "GAS_Project/AEquipment/LeeEquipmentInstance.h"
#include "GAS_Project/AFeedBack/LeeWeaponTelegraphVisuals.h"
#include "GAS_Project/Notifies/ANS_WeaponTrace.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLeeDualWeaponTraceTest, "GAS_Project.Combat.DualWeapon.SelectionAndCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLeeDualWeaponTraceTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
	if (!TestNotNull(TEXT("테스트 월드"), World)) return false;
	APawn* Owner = World->SpawnActor<APawn>();
	auto* Root = NewObject<USceneComponent>(Owner);
	Owner->AddInstanceComponent(Root);
	Owner->SetRootComponent(Root);
	auto* Body = NewObject<USkeletalMeshComponent>(Owner);
	Owner->AddInstanceComponent(Body);
	Body->SetupAttachment(Root);
	auto* Manager = NewObject<ULeeEquipmentManagerComponent>(Owner);
	Owner->AddInstanceComponent(Manager);
	auto* Instance = NewObject<ULeeEquipmentInstance>(Owner);
	Manager->EquipmentList.Entries.AddDefaulted_GetRef().Instance = Instance;
	auto MakeWeapon = [&](FName AttachSocket, const TArray<FName>& Sockets)
	{
		AActor* Actor = World->SpawnActor<AActor>();
		auto* Mesh = NewObject<UStaticMeshComponent>(Actor);
		Actor->AddInstanceComponent(Mesh);
		Actor->SetRootComponent(Mesh);
		Mesh->SetMobility(EComponentMobility::Movable);
		auto* Asset = NewObject<UStaticMesh>();
		for (FName Name : Sockets)
		{
			auto* Socket = NewObject<UStaticMeshSocket>(Asset);
			Socket->SocketName = Name;
			Asset->AddSocket(Socket);
		}
		Mesh->SetStaticMesh(Asset);
		Mesh->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform, AttachSocket);
		Instance->SpawnedActors.Add(Actor);
		return Mesh;
	};
	UMeshComponent* Axe = MakeWeapon(TEXT("LeftEquipment"), {TEXT("WeaponBase"), TEXT("WeaponTip"), TEXT("Glow")});
	UStaticMeshComponent* Shield = MakeWeapon(TEXT("RightEquipment"), {TEXT("ShieldBase"), TEXT("ShieldEdge"), TEXT("Glow")});
	const auto Right = LeeEquipmentMeshSelection::FindMeshes(Owner, TEXT("RightEquipment"));
	TestTrue(TEXT("첫 장비가 도끼여도 오른쪽은 방패 선택"), Right.Num() == 1 && Right[0] == Shield);
	TestEqual(TEXT("잘못된 장비 소켓은 다른 무기로 대체하지 않음"), LeeEquipmentMeshSelection::FindMeshes(Owner, TEXT("Missing")).Num(), 0);

	FLeeWeaponTelegraphTarget LeftTarget;
	LeftTarget.EquipmentAttachSocket = TEXT("LeftEquipment"); LeftTarget.EffectAttachSocket = TEXT("Glow");
	FLeeWeaponTelegraphTarget RightTarget;
	RightTarget.EquipmentAttachSocket = TEXT("RightEquipment"); RightTarget.EffectAttachSocket = TEXT("Glow");
	TArray<FLeeResolvedTelegraphTarget> Targets;
	LeeWeaponTelegraphVisuals::Resolve(Owner, {LeftTarget, RightTarget, LeftTarget}, Targets);
	TestEqual(TEXT("양쪽 예고 대상 선택과 중복 항목 제거"), Targets.Num(), 2);
	LeftTarget.EffectAttachSocket = TEXT("MissingEffectSocket");
	LeeWeaponTelegraphVisuals::Resolve(Owner, {LeftTarget}, Targets);
	TestEqual(TEXT("이펙트 소켓 오타도 도끼 루트로 대체하지 않음"), Targets.Num(), 0);
	LeftTarget.EffectAttachSocket = NAME_None;
	LeeWeaponTelegraphVisuals::Resolve(Owner, {LeftTarget}, Targets);
	TestTrue(TEXT("None은 명시적인 장비 루트 부착"), Targets.Num() == 1 && Targets[0].Socket.IsNone());
	LeeWeaponTelegraphVisuals::Resolve(Owner, {LeftTarget, RightTarget}, Targets);
	auto* PreviousAxe = NewObject<UMaterial>();
	auto* PreviousShield = NewObject<UMaterial>();
	auto* Glow = NewObject<UMaterial>();
	Axe->SetOverlayMaterial(PreviousAxe); Shield->SetOverlayMaterial(PreviousShield);
	TArray<FLeeWeaponTelegraphVisualState> Visuals;
	LeeWeaponTelegraphVisuals::Apply(Targets, Glow, nullptr, Visuals);
	TestTrue(TEXT("양쪽에 오버레이 적용"), Axe->GetOverlayMaterial() == Glow && Shield->GetOverlayMaterial() == Glow);
	LeeWeaponTelegraphVisuals::Clear(Visuals);
	TestTrue(TEXT("각 장비의 서로 다른 원래 오버레이 복원"), Axe->GetOverlayMaterial() == PreviousAxe && Shield->GetOverlayMaterial() == PreviousShield);
	LeeWeaponTelegraphVisuals::Clear(Visuals);
	TestEqual(TEXT("중복 정리는 안전함"), Visuals.Num(), 0);

	auto* Trace = NewObject<UANS_WeaponTrace>();
	FLeeWeaponTraceTarget AxeTrace;
	AxeTrace.EquipmentAttachSocket = TEXT("LeftEquipment");
	FLeeWeaponTraceTarget ShieldTrace;
	ShieldTrace.EquipmentAttachSocket = TEXT("RightEquipment"); ShieldTrace.StartSocket = TEXT("ShieldBase"); ShieldTrace.EndSocket = TEXT("ShieldEdge");
	Trace->WeaponTargets = {AxeTrace, ShieldTrace, AxeTrace};
	TArray<UANS_WeaponTrace::FWeaponTraceSourceData> Sources;
	Trace->ResolveTraceSources(Body, Sources);
	TestEqual(TEXT("무기별 서로 다른 소켓과 중복 대상 제거"), Sources.Num(), 2);
	if (Sources.Num() == 2)
	{
		TestTrue(TEXT("무기별 판정 메시 분리"), Sources[0].TraceMesh.Get() == Axe && Sources[1].TraceMesh.Get() == Shield);
		AActor* Victim = World->SpawnActor<AActor>();
		Sources[0].HitActors.Add(Victim);
		TestFalse(TEXT("도끼 적중 이력이 방패 판정을 차단하지 않음"), Sources[1].HitActors.Contains(Victim));
	}
	ShieldTrace.EquipmentAttachSocket = TEXT("Missing");
	Trace->WeaponTargets = {ShieldTrace};
	Trace->ResolveTraceSources(Body, Sources);
	TestEqual(TEXT("Trace 명시 대상이 없으면 첫 도끼 폴백 금지"), Sources.Num(), 0);

	// Persona처럼 장비 컴포넌트의 소유자와 부착 대상이 서로 다른 경우를 재현한다.
	AActor* PreviewOwner = World->SpawnActor<AActor>();
	auto* PreviewShield = NewObject<UStaticMeshComponent>(PreviewOwner);
	PreviewOwner->AddInstanceComponent(PreviewShield);
	PreviewShield->SetStaticMesh(Shield->GetStaticMesh());
	PreviewShield->AttachToComponent(Body, FAttachmentTransformRules::KeepRelativeTransform, TEXT("PreviewShield"));
	RightTarget.EquipmentAttachSocket = TEXT("PreviewShield");
	LeeWeaponTelegraphVisuals::Resolve(Owner, {RightTarget}, Targets);
	TestTrue(TEXT("미리보기는 소유자가 달라도 부착된 방패 Glow 소켓을 찾음"),
		Targets.Num() == 1 && Targets[0].Mesh.Get() == PreviewShield && Targets[0].Socket == TEXT("Glow"));

	World->DestroyWorld(false);
	return true;
}
#endif
