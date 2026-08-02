#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Misc/Optional.h"

class SWindow;

// 저장 확인 창에서 사용자가 내린 결정.
enum class ERecordingDecision : uint8
{
	// 지정한 폴더로 옮기고 이름을 바꾼다.
	Save,

	// 파일을 삭제한다. 사용자가 명시적으로 누른 경우에만 발생한다.
	Discard,

	// 아무것도 하지 않는다. 원본이 OBS 기본 경로에 그대로 남는다.
	LeaveInPlace,
};

// 저장 확인 창에 필요한 한 건의 녹화 정보.
struct FRecordingDispositionItem
{
	FGuid   SessionId;
	FString OutputPath;			// OBS가 알려준 원본 경로. 이 값만 신뢰한다.
	FString LevelName;
	double  DurationSeconds = 0.0;
	int64   FileSizeBytes = 0;
};

// PIE 녹화 파일의 저장/삭제 결정을 순차 처리하는 대기열.
// Coordinator와 분리되어 있어 녹화 파이프라인을 막지 않는다.
class FRecordingDispositionQueue : public TSharedFromThis<FRecordingDispositionQueue>
{
public:
	FRecordingDispositionQueue();
	~FRecordingDispositionQueue();

	// 정지 성공한 녹화 한 건을 대기열에 넣는다.
	void Enqueue(const FRecordingDispositionItem& Item);

	// 에디터 종료 시 미결 항목을 원본 그대로 두고 경로만 로그에 남긴다.
	void HandleEditorPreExit();

	// 기본 파일명을 만든다. 확장자는 붙이지 않는다.
	static FString BuildDefaultFileName(const FRecordingDispositionItem& Item);

	// 기본 저장 폴더를 정한다. 설정이 비어 있으면 원본 폴더를 쓴다.
	static FString GetDefaultSaveDirectory(const FRecordingDispositionItem& Item);

	// 같은 이름이 있으면 _1, _2 … 를 붙인다. 덮어쓰지 않는다.
	// 자동화 테스트에서 직접 검증하기 위해 공개해 둔다.
	static FString MakeUniqueDestination(const FString& Directory, const FString& BaseName, const FString& Extension);

private:
	// 대기열 선두를 꺼내 처리한다. 창이 이미 떠 있으면 아무것도 하지 않는다.
	void ProcessNext();

	// 파일 크기가 안정될 때까지 폴링한다. 게임 스레드를 붙잡지 않는다.
	bool TickFileReady(float DeltaTime);
	void StopFileReadyPolling();

	// 파일이 만질 수 있는 상태가 됐다. 설정에 따라 창을 띄우거나 바로 처리한다.
	void OnFileReady();

	void ShowDialog();
	void CloseDialog();

	// 결정을 실제 파일 조작으로 옮긴다.
	void ApplyDecision(const FRecordingDispositionItem& Item, ERecordingDecision Decision, const FString& Directory, const FString& BaseName);

	bool TrySave(const FRecordingDispositionItem& Item, const FString& Directory, const FString& BaseName);
	bool TryDiscard(const FRecordingDispositionItem& Item);

	TArray<FRecordingDispositionItem> Pending;

	// 지금 처리 중인 한 건. 창이 떠 있는 동안 유지된다.
	TOptional<FRecordingDispositionItem> CurrentItem;

	TWeakPtr<SWindow> ActiveWindow;

	// "나머지에도 적용" 체크 시 남은 항목에 그대로 쓰는 결정.
	TOptional<ERecordingDecision> StickyDecision;
	FString StickyDirectory;

	FTSTicker::FDelegateHandle FileReadyTickerHandle;
	double FileReadyStartTime = 0.0;
	int64  LastObservedSize = -1;
};
