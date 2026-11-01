#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "RatRaceTailComponent.generated.h"

class APawn;

/** Grab point on a pawn's tail and the logic for holding and pulling another pawn. */
UCLASS(ClassGroup = (RatRace), meta = (BlueprintSpawnableComponent))
class RATRACE_API URatRaceTailComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	URatRaceTailComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "RatRace|Tail")
	bool TryGrab();

	UFUNCTION(BlueprintCallable, Category = "RatRace|Tail")
	void Release();

	UFUNCTION(BlueprintPure, Category = "RatRace|Tail")
	bool IsHoldingTail() const { return HeldTail.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "RatRace|Tail")
	URatRaceTailComponent* GetHeldTail() const { return HeldTail.Get(); }

	UFUNCTION(BlueprintPure, Category = "RatRace|Tail")
	APawn* GetHolder() const { return Holder.IsValid() ? Cast<APawn>(Holder->GetOwner()) : nullptr; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Tail")
	FVector GrabPointOffset = FVector(60.f, 0.f, -70.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Tail", meta = (ClampMin = "0"))
	float GrabReach = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Tail", meta = (ClampMin = "0"))
	float SlackLength = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Tail", meta = (ClampMin = "1"))
	float PullRampLength = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Tail", meta = (ClampMin = "0"))
	float MaxPullStrength = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Tail", meta = (ClampMin = "0"))
	float GrabberRecoil = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Tail", meta = (ClampMin = "0"))
	float BreakDistance = 500.f;

private:
	void ApplyPull();

	TWeakObjectPtr<URatRaceTailComponent> HeldTail;
	TWeakObjectPtr<URatRaceTailComponent> Holder;
	bool bOriginalOrientRotationToMovement = true;
};
