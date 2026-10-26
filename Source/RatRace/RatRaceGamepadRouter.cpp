#include "RatRaceGamepadRouter.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "InputKeyEventArgs.h"
#include "Misc/App.h"
#include "Widgets/SViewport.h"

FRatRaceGamepadRouter::FRatRaceGamepadRouter(UGameViewportClient* InViewportClient, bool bInShowDebug)
	: ViewportClient(InViewportClient)
	, bShowDebug(bInShowDebug)
{
}

bool FRatRaceGamepadRouter::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	UGameViewportClient* Client = GetViewportClient();
	if (!Key.IsGamepadKey() || !Client || !IsGameFocused(SlateApp))
	{
		return false;
	}

	ShowDebug(InKeyEvent.GetInputDeviceId(), Key, 1.f);
	Client->InputKey(FInputKeyEventArgs(Client->Viewport, InKeyEvent.GetInputDeviceId(), Key,
		InKeyEvent.IsRepeat() ? IE_Repeat : IE_Pressed, 1.f, false, InKeyEvent.GetEventTimestamp()));
	// Consume so Slate doesn't route it a second time (player 0 would get double input).
	return true;
}

bool FRatRaceGamepadRouter::HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	// Releases are forwarded even without focus so buttons never get stuck down.
	const FKey Key = InKeyEvent.GetKey();
	UGameViewportClient* Client = GetViewportClient();
	if (!Key.IsGamepadKey() || !Client)
	{
		return false;
	}

	Client->InputKey(FInputKeyEventArgs(Client->Viewport, InKeyEvent.GetInputDeviceId(), Key,
		IE_Released, 0.f, false, InKeyEvent.GetEventTimestamp()));
	return true;
}

bool FRatRaceGamepadRouter::HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent)
{
	const FKey Key = InAnalogInputEvent.GetKey();
	UGameViewportClient* Client = GetViewportClient();
	if (!Key.IsGamepadKey() || !Client || !IsGameFocused(SlateApp))
	{
		return false;
	}

	if (!FMath::IsNearlyZero(InAnalogInputEvent.GetAnalogValue(), 0.01f))
	{
		ShowDebug(InAnalogInputEvent.GetInputDeviceId(), Key, InAnalogInputEvent.GetAnalogValue());
	}

	// Same conversion FSceneViewport::OnAnalogValueChanged does (including the RightY flip).
	const float Value = Key == EKeys::Gamepad_RightY ? -InAnalogInputEvent.GetAnalogValue() : InAnalogInputEvent.GetAnalogValue();
	Client->InputAxis(FInputKeyEventArgs(Client->Viewport, InAnalogInputEvent.GetInputDeviceId(), Key,
		Value, FApp::GetDeltaTime(), 1, InAnalogInputEvent.GetEventTimestamp()));
	return true;
}

bool FRatRaceGamepadRouter::IsGameFocused(FSlateApplication& SlateApp) const
{
	const TSharedPtr<SViewport> GameViewport = SlateApp.GetGameViewport();
	return GameViewport.IsValid() && GameViewport->HasAnyUserFocusOrFocusedDescendants();
}

UGameViewportClient* FRatRaceGamepadRouter::GetViewportClient() const
{
	UGameViewportClient* Client = ViewportClient.Get();
	return Client && Client->Viewport ? Client : nullptr;
}

void FRatRaceGamepadRouter::ShowDebug(const FInputDeviceId DeviceId, const FKey& Key, float Value) const
{
	if (!bShowDebug || !GEngine)
	{
		return;
	}

	const FPlatformUserId User = IPlatformInputDeviceMapper::Get().GetUserForInputDevice(DeviceId);
	const UGameInstance* GameInstance = ViewportClient.IsValid() ? ViewportClient->GetGameInstance() : nullptr;
	const ULocalPlayer* Player = GameInstance ? GameInstance->FindLocalPlayerFromPlatformUserId(User) : nullptr;
	const FString Target = Player ? FString::Printf(TEXT("player %d"), Player->GetLocalPlayerIndex()) : TEXT("NO PLAYER");

	// One line per device, replaced on every event.
	GEngine->AddOnScreenDebugMessage(1000 + DeviceId.GetId(), 2.f, FColor::Yellow,
		FString::Printf(TEXT("device %d (user %d) -> %s: %s %.2f"), DeviceId.GetId(), User.GetInternalId(), *Target, *Key.ToString(), Value));
}
