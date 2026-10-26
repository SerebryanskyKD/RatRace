#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"

class UGameViewportClient;

/**
 * Slate input preprocessor that hands gamepad events straight to the game viewport client.
 *
 * Normally Slate routes a gamepad event along the focus path of the Slate user that owns the
 * device. Only user 0 reliably gets the game viewport focused (clicking the window, PIE start),
 * so pads owned by other local players never reach the game. Preprocessors run before focus
 * routing, so forwarding here makes every pad work; the viewport client then picks the local
 * player from the device's platform user as usual.
 */
class FRatRaceGamepadRouter : public IInputProcessor
{
public:
	FRatRaceGamepadRouter(UGameViewportClient* InViewportClient, bool bInShowDebug);

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) override;
	virtual const TCHAR* GetDebugName() const override { return TEXT("RatRaceGamepadRouter"); }

private:
	/** True while the game viewport (not editor UI) has keyboard/mouse focus. */
	bool IsGameFocused(FSlateApplication& SlateApp) const;
	UGameViewportClient* GetViewportClient() const;
	void ShowDebug(const FInputDeviceId DeviceId, const FKey& Key, float Value) const;

	TWeakObjectPtr<UGameViewportClient> ViewportClient;
	bool bShowDebug = false;
};
