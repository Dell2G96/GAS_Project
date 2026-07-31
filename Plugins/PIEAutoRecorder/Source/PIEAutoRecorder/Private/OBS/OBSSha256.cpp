#include "OBSSha256.h"

#include "Misc/Base64.h"

THIRD_PARTY_INCLUDES_START
#include <openssl/sha.h>
THIRD_PARTY_INCLUDES_END

namespace OBSSha256
{
	// 입력 바이트열의 SHA-256 해시(32바이트)를 계산한다.
	void Compute(const uint8* Data, int32 Size, uint8 OutDigest[32])
	{
		// Size가 0이어도 OpenSSL은 빈 입력의 정상 해시를 돌려준다.
		SHA256(Data, static_cast<size_t>(FMath::Max(Size, 0)), OutDigest);
	}

	// 문자열을 UTF-8로 변환해 해시한다. TCHAR 그대로 넘기면 인증이 실패하므로 반드시 변환한다.
	static void HashUtf8(const FString& Input, uint8 OutDigest[32])
	{
		const FTCHARToUTF8 Utf8(*Input);
		Compute(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length(), OutDigest);
	}

	// UTF-8 해시 결과를 Base64 문자열로 돌려준다. obs-websocket 인증의 두 단계 모두 이 형태를 쓴다.
	FString HashUtf8ToBase64(const FString& Input)
	{
		uint8 Digest[32];
		HashUtf8(Input, Digest);

		return FBase64::Encode(Digest, sizeof(Digest));
	}

	// UTF-8 해시 결과를 소문자 16진 문자열로 돌려준다. 표준 테스트 벡터 대조에 쓴다.
	FString HashUtf8ToHex(const FString& Input)
	{
		uint8 Digest[32];
		HashUtf8(Input, Digest);

		FString Hex;
		Hex.Reserve(64);
		for (int32 Index = 0; Index < 32; ++Index)
		{
			Hex += FString::Printf(TEXT("%02x"), Digest[Index]);
		}

		return Hex;
	}
}
