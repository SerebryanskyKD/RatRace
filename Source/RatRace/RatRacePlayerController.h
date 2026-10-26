#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RatRacePlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class URatRaceTailComponent;
struct FInputActionValue;

/**
 * One controller per local player. Views the world through the shared arena camera and
 * builds its Enhanced Input actions/mapping context at runtime (no .uasset needed).
 */
UCLASS()
class RATRACE_API ARatRacePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARatRacePlayerController();

	virtual void BeginPlay() override;
	virtual void ReceivedPlayer() override;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Input", meta = (ClampMin = "0", ClampMax = "1"))
	float StickDeadZone = 0.2f;

	/** Adds WASD + Space (+ E to grab) for the first local player (handy for debugging without a pad). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Input")
	bool bKeyboardForFirstPlayer = true;

private:
	void CreateInputActions();
	void AddMappingContext();
	void ViewArenaCamera();

	void HandleMove(const FInputActionValue& Value);
	void HandleJumpStarted();
	void HandleJumpCompleted();
	void HandleGrabTriggered();
	void HandleGrabCompleted();

	URatRaceTailComponent* GetPawnTail() const;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> JumpAction;

	/** Hold to grab (and keep holding) another rat's tail. */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> GrabAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;
};
