#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GAS_Project/Notifies/ANS_WeaponTrace.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "ReferenceSkeleton.h"
#include "Animation/Skeleton.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLeeWeaponTraceSourceTest,
	"GAS_Project.Combat.WeaponTrace.MeshSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLeeWeaponTraceSourceTest::RunTest(const FString& Parameters)
{
	auto* Notify = NewObject<UANS_WeaponTrace>();
	TestTrue(TEXT("기존 노티파이 기본값은 장착 무기"), Notify->TraceSource == ELeeMeleeTraceSource::EquippedWeapon);
	TestEqual(TEXT("기존 무기 시작 소켓 유지"), Notify->WeaponBaseSocket, FName(TEXT("WeaponBase")));
	TestEqual(TEXT("기존 무기 끝 소켓 유지"), Notify->WeaponTipSocket, FName(TEXT("WeaponTip")));
	TestNull(TEXT("메시가 없으면 안전하게 무시"), Notify->ResolveTraceMesh(nullptr));

	auto* Asset = NewObject<USkeletalMesh>();
	Asset->SetSkeleton(NewObject<USkeleton>());
	{
		FReferenceSkeletonModifier Modifier(Asset->GetRefSkeleton(), nullptr);
		Modifier.Add(FMeshBoneInfo(TEXT("TestRoot"), TEXT("TestRoot"), INDEX_NONE), FTransform::Identity);
	}
	for (const FName Name : {FName(TEXT("TestHand")), FName(TEXT("TestFoot"))})
	{
		auto* Socket = NewObject<USkeletalMeshSocket>(Asset);
		Socket->SocketName = Name;
		Socket->BoneName = TEXT("TestRoot");
		Asset->GetMeshOnlySocketList().Add(Socket);
	}
	auto* Mesh = NewObject<USkeletalMeshComponent>();
	Mesh->SetSkeletalMeshAsset(Asset);
	Notify->TraceSource = ELeeMeleeTraceSource::CharacterMesh;
	TestNull(TEXT("신체 이름 미지정은 메시 원점으로 폴백하지 않음"), Notify->ResolveTraceMesh(Mesh));
	Notify->BodyStartSocket = TEXT("TestHand");
	Notify->BodyEndSocket = TEXT("TestFoot");
	TestTrue(TEXT("장비 없이 캐릭터 메시의 두 지점 사용"), Notify->ResolveTraceMesh(Mesh) == Mesh);
	Notify->BodyEndSocket = TEXT("TestHand");
	TestTrue(TEXT("같은 지점 두 개도 구체 판정용으로 유효"), Notify->ResolveTraceMesh(Mesh) == Mesh);
	Notify->BodyStartSocket = TEXT("TestRoot");
	Notify->BodyEndSocket = TEXT("TestRoot");
	TestTrue(TEXT("별도 소켓 없이 실제 본 이름도 사용 가능"), Notify->ResolveTraceMesh(Mesh) == Mesh);
	Notify->BodyEndSocket = TEXT("Missing");
	TestNull(TEXT("잘못된 이름은 다른 무기나 메시로 폴백하지 않음"), Notify->ResolveTraceMesh(Mesh));
	TestFalse(TEXT("캐시된 메시에서도 잘못된 이름 검사"), Notify->HasTraceSockets(Mesh));

	FName Start, End;
	Notify->TraceSource = ELeeMeleeTraceSource::EquippedWeapon;
	Notify->GetTraceSocketNames(Start, End);
	TestEqual(TEXT("무기 모드 시작 이름에 신체 설정이 섞이지 않음"), Start, Notify->WeaponBaseSocket);
	TestEqual(TEXT("무기 모드 끝 이름에 신체 설정이 섞이지 않음"), End, Notify->WeaponTipSocket);
	return true;
}
#endif
