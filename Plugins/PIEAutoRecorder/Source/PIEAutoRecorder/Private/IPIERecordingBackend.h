#pragma once

#include "CoreMinimal.h"
#include "OBS/OBSWebSocketProtocol.h"

// 요청 결과 콜백. 마지막 인자 SessionId로 "어느 PIE 세션의 응답인가"를 항상 알 수 있다.
DECLARE_DELEGATE_ThreeParams(FOnRecordStatusResult, bool /*bSuccess*/, const FOBSRecordStatus& /*Status*/, FGuid /*SessionId*/);
DECLARE_DELEGATE_ThreeParams(FOnSimpleResult, bool /*bSuccess*/, const FString& /*Error*/, FGuid /*SessionId*/);
DECLARE_DELEGATE_ThreeParams(FOnStopResult, bool /*bSuccess*/, const FString& /*OutputPath*/, FGuid /*SessionId*/);

// 녹화 상태 변화 이벤트. 요청 응답을 보조하는 용도로만 쓴다.
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRecordStateChanged, const FOBSRecordStateChanged& /*Event*/);

// 외부 녹화 프로그램 추상화. 장면·오디오·인코더 같은 프로그램 고유 기능은 넣지 않는다.
// 이 경계가 있어야 Coordinator를 OBS 없이 단위 테스트할 수 있다.
class IPIERecordingBackend
{
public:
	virtual ~IPIERecordingBackend() = default;

	virtual void Connect() = 0;
	virtual void Disconnect() = 0;
	virtual bool IsReady() const = 0;

	// 모든 요청이 SessionId를 받는 것이 설계의 핵심이다. 콜백이 늦게 와도 어느 세션의 응답인지 알 수 있다.
	virtual void QueryRecordStatus(FGuid SessionId, FOnRecordStatusResult Callback) = 0;
	virtual void StartRecording(FGuid SessionId, FOnSimpleResult Callback) = 0;
	virtual void StopRecording(FGuid SessionId, FOnStopResult Callback) = 0;

	virtual FOnRecordStateChanged& OnRecordStateChanged() = 0;
};
