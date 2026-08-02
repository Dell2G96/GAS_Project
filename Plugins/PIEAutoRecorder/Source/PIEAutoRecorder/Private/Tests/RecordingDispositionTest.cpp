#include "Misc/AutomationTest.h"

#include "Disposition/RecordingDispositionQueue.h"
#include "PIEAutoRecorderSettings.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace PIEAutoRecorderFileTest
{
	// 테스트용 임시 폴더. 프로젝트 Saved 아래에 만들고 끝나면 지운다.
	FString MakeTempDirectory()
	{
		const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("PIEAutoRecorderTest"), FGuid::NewGuid().ToString());
		IFileManager::Get().MakeDirectory(*Dir, /*Tree=*/true);
		return Dir;
	}

	// 내용이 있는 더미 파일을 만든다.
	bool WriteDummyFile(const FString& Path)
	{
		return FFileHelper::SaveStringToFile(TEXT("dummy recording"), *Path);
	}
}

//========================================================================================
// 1. 이름 충돌 자동 증가 — 덮어쓰기가 절대 일어나지 않아야 한다 (P0-15)
//========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecordingUniqueNameTest, "PIEAutoRecorder.Disposition.UniqueName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 같은 이름이 이미 있을 때 _1, _2 가 붙는지 확인한다.
bool FRecordingUniqueNameTest::RunTest(const FString& Parameters)
{
	using namespace PIEAutoRecorderFileTest;

	const FString Dir = MakeTempDirectory();

	// 비어 있으면 원래 이름 그대로
	const FString First = FRecordingDispositionQueue::MakeUniqueDestination(Dir, TEXT("PIE_Test"), TEXT("mkv"));
	TestEqual(TEXT("충돌 없으면 원래 이름"), FPaths::GetCleanFilename(First), TEXT("PIE_Test.mkv"));

	// 같은 이름을 만들어 두면 _1
	TestTrue(TEXT("더미 파일 생성"), WriteDummyFile(First));
	const FString Second = FRecordingDispositionQueue::MakeUniqueDestination(Dir, TEXT("PIE_Test"), TEXT("mkv"));
	TestEqual(TEXT("첫 충돌은 _1"), FPaths::GetCleanFilename(Second), TEXT("PIE_Test_1.mkv"));

	// _1까지 있으면 _2
	TestTrue(TEXT("더미 파일 생성"), WriteDummyFile(Second));
	const FString Third = FRecordingDispositionQueue::MakeUniqueDestination(Dir, TEXT("PIE_Test"), TEXT("mkv"));
	TestEqual(TEXT("두 번째 충돌은 _2"), FPaths::GetCleanFilename(Third), TEXT("PIE_Test_2.mkv"));

	// 기존 파일이 그대로 남아 있어야 한다 (덮어쓰기 금지)
	TestTrue(TEXT("원본 유지"), IFileManager::Get().FileExists(*First));
	TestTrue(TEXT("_1 유지"), IFileManager::Get().FileExists(*Second));

	// 확장자가 없는 경우도 안전해야 한다
	const FString NoExt = FRecordingDispositionQueue::MakeUniqueDestination(Dir, TEXT("NoExtension"), TEXT(""));
	TestEqual(TEXT("확장자 없음"), FPaths::GetCleanFilename(NoExt), TEXT("NoExtension"));

	IFileManager::Get().DeleteDirectory(*Dir, /*RequireExists=*/false, /*Tree=*/true);
	return true;
}

//========================================================================================
// 2. 기본 파일명 생성 — 한글·공백·금지 문자가 있어도 안전해야 한다 (P1-19)
//========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecordingFileNameTest, "PIEAutoRecorder.Disposition.FileName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 레벨명이 어떤 형태여도 파일 시스템에서 쓸 수 있는 이름이 나와야 한다.
bool FRecordingFileNameTest::RunTest(const FString& Parameters)
{
	// 사용자가 설정에서 형식을 바꿔도 테스트 결과가 흔들리면 안 된다.
	// 테스트 안에서 형식을 고정하고, 끝나면 원래 값으로 되돌린다.
	UPIEAutoRecorderSettings* Settings = GetMutableDefault<UPIEAutoRecorderSettings>();
	const FString SavedFormat = Settings->SaveFilenameFormat;
	Settings->SaveFilenameFormat = TEXT("PIE_{level}_{date}_{time}_{id}");
	ON_SCOPE_EXIT{ Settings->SaveFilenameFormat = SavedFormat; };

	FRecordingDispositionItem Item;
	Item.SessionId = FGuid::NewGuid();
	Item.LevelName = TEXT("L_Boss_Arena");
	Item.DurationSeconds = 100.0;

	// 형식의 각 토큰이 실제로 치환되는지
	const FString Normal = FRecordingDispositionQueue::BuildDefaultFileName(Item);
	TestTrue(TEXT("레벨명 포함"), Normal.Contains(TEXT("L_Boss_Arena")));
	TestTrue(TEXT("접두사 포함"), Normal.StartsWith(TEXT("PIE_")));
	TestFalse(TEXT("치환되지 않은 토큰 없음"), Normal.Contains(TEXT("{")));

	// {id}는 세션 ID 앞 8자리로 치환된다
	TestTrue(TEXT("세션 ID 포함"), Normal.Contains(Item.SessionId.ToString(EGuidFormats::Digits).Left(8)));

	// 금지 문자가 든 레벨명
	{
		FRecordingDispositionItem Bad = Item;
		Bad.LevelName = TEXT("Level:With*Bad?Chars");

		const FString Result = FRecordingDispositionQueue::BuildDefaultFileName(Bad);

		const FString InvalidChars = TEXT("\\/:*?\"<>|");
		for (int32 Index = 0; Index < InvalidChars.Len(); ++Index)
		{
			TestFalse(FString::Printf(TEXT("금지 문자 %s 제거됨"), *InvalidChars.Mid(Index, 1)),
				Result.Contains(InvalidChars.Mid(Index, 1)));
		}
	}

	// 한글과 공백이 든 레벨명 — 크래시 없이 이름이 만들어져야 한다
	{
		FRecordingDispositionItem Korean = Item;
		Korean.LevelName = TEXT("보스 방 테스트");

		const FString Result = FRecordingDispositionQueue::BuildDefaultFileName(Korean);
		TestFalse(TEXT("빈 이름이 아님"), Result.IsEmpty());
	}

	return true;
}

//========================================================================================
// 3. 삭제는 대상 파일 하나만 — 같은 폴더의 다른 파일이 무사해야 한다 (P0-13)
//========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecordingDeleteScopeTest, "PIEAutoRecorder.Disposition.DeleteScope",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// IFileManager::Delete가 정확히 지정한 파일 하나만 지우는지 확인한다.
bool FRecordingDeleteScopeTest::RunTest(const FString& Parameters)
{
	using namespace PIEAutoRecorderFileTest;

	const FString Dir = MakeTempDirectory();

	// 대괄호·공백·한글이 든 경로에서도 정확히 동작해야 한다 (실제 OBS 경로가 이런 형태였다)
	const FString Target = FPaths::Combine(Dir, TEXT("2026-08-01 03-06-57.mkv"));
	const FString Neighbor1 = FPaths::Combine(Dir, TEXT("2026-08-01 03-06-19.mkv"));
	const FString Neighbor2 = FPaths::Combine(Dir, TEXT("중요한 녹화.mkv"));

	TestTrue(TEXT("대상 생성"), WriteDummyFile(Target));
	TestTrue(TEXT("이웃1 생성"), WriteDummyFile(Neighbor1));
	TestTrue(TEXT("이웃2 생성"), WriteDummyFile(Neighbor2));

	const bool bDeleted = IFileManager::Get().Delete(*Target, /*RequireExists=*/true, /*EvenReadOnly=*/false, /*Quiet=*/true);

	TestTrue(TEXT("대상 삭제됨"), bDeleted);
	TestFalse(TEXT("대상이 사라짐"), IFileManager::Get().FileExists(*Target));
	TestTrue(TEXT("이웃1 무사"), IFileManager::Get().FileExists(*Neighbor1));
	TestTrue(TEXT("이웃2 무사"), IFileManager::Get().FileExists(*Neighbor2));

	IFileManager::Get().DeleteDirectory(*Dir, /*RequireExists=*/false, /*Tree=*/true);
	return true;
}

//========================================================================================
// 4. Copy + Delete 폴백 — 드라이브 간 이동이 실패했을 때의 경로
//========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecordingCopyFallbackTest, "PIEAutoRecorder.Disposition.CopyFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Copy 성공 후 원본 삭제로 이어지는 경로가 실제로 동작하는지 확인한다.
bool FRecordingCopyFallbackTest::RunTest(const FString& Parameters)
{
	using namespace PIEAutoRecorderFileTest;

	const FString SourceDir = MakeTempDirectory();
	const FString DestDir = MakeTempDirectory();

	const FString Source = FPaths::Combine(SourceDir, TEXT("source.mkv"));
	const FString Dest = FPaths::Combine(DestDir, TEXT("dest.mkv"));

	TestTrue(TEXT("원본 생성"), WriteDummyFile(Source));

	TestEqual(TEXT("복사 성공"), IFileManager::Get().Copy(*Dest, *Source), (uint32)COPY_OK);
	TestTrue(TEXT("사본 존재"), IFileManager::Get().FileExists(*Dest));
	TestTrue(TEXT("복사 직후 원본도 존재"), IFileManager::Get().FileExists(*Source));

	// 내용이 보존됐는지 확인한다
	FString Contents;
	TestTrue(TEXT("사본 읽기"), FFileHelper::LoadFileToString(Contents, *Dest));
	TestEqual(TEXT("내용 동일"), Contents, TEXT("dummy recording"));

	TestTrue(TEXT("원본 삭제"), IFileManager::Get().Delete(*Source, true, false, true));
	TestFalse(TEXT("원본이 사라짐"), IFileManager::Get().FileExists(*Source));

	IFileManager::Get().DeleteDirectory(*SourceDir, false, true);
	IFileManager::Get().DeleteDirectory(*DestDir, false, true);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
