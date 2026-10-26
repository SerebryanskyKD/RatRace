#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "RatRaceTailComponent.generated.h"

/**
 * Marks the tip of a rat's tail and handles both sides of a tail grab: the owner can grab
 * another rat's tail, and other rats can grab this one.
 *
 * The pull is not a physics constraint (the pawns are driven by CharacterMovement): while a grab
 * is active the grabber's component injects movement input into both pawns, proportional to how
 * far the "rope" (grabber center -> held tail tip, along the floor) is stretched beyond its slack.
 * It mixes with the players' own stick input, so it behaves like a tug of war and can never exceed
 * the normal walk speed. The held rat keeps its back (and tail) turned to the grabber.
 */
UCLASS(ClassGroup = (RatRace), meta = (BlueprintSpawnableComponent))
class RATRACE_API URatRaceTailComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	URatRaceTailComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Grabs the closest free tail of another rat within reach. Returns true if a grab started. */
	UFUNCTION(BlueprintCallable, Category = "RatRace|Tail")
	bool TryGrab();

	/** Lets go of the held tail, if any. */
	UFUNCTION(BlueprintCallable, Category = "RatRace|Tail")
	void Release();

	/** Tail this rat is currently holding (another rat's component), or null. */
	UFUNCTION(BlueprintPure, Category = "RatRace|Tail")
	URatRaceTailComponent* GetHeldTail() const { return HeldTail.Get(); }

	/** Component of the rat holding this tail, or null. */
	UFUNCTION(BlueprintPure, Category = "RatRace|Tail")
	URatRaceTailComponent* GetHolder() const { return Holder.Get(); }

	UFUNCTION(BlueprintPure, Category = "RatRace|Tail")
	bool IsHoldingTail() const { return HeldTail.IsValid(); }

	/** World position the owner grabs with (in front of the rat, near the floor). */
	UFUNCTION(BlueprintPure, Category = "RatRace|Tail")
	FVector GetGrabPointLocation() const;

protected:
	/** Grab point in the owner actor's local space. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Tail")
	FVector GrabPointOffset = FVector(60.f, 0.f, -70.f);

	/** Max distance from the grab point to another rat's tail tip to start a grab. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Tail", meta = (ClampMin = "0"))
	float GrabReach = 100.f;

	/** Rope length (grabber center to held tail tip, along the floor) with no pull at all. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Tail", meta = (ClampMin = "0"))
	float SlackLength = 120.f;

	/** Extra stretch beyond the slack at which the pull reaches MaxPullStrength. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Tail", meta = (ClampMin = "1"))
	float PullRampLength = 150.f;

	/**
	 * Movement input added to the held rat at full stretch (1 = full stick). Above 1 the grabber
	 * wins a tug of war against a held rat pushing the stick the other way.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Tail", meta = (ClampMin = "0", ClampMax = "3"))
	float MaxPullStrength = 2.f;

	/** Fraction of the pull the grabber feels back from the held rat. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Tail", meta = (ClampMin = "0", ClampMax = "1"))
	float GrabberRecoil = 0.5f;

	/** The grab breaks when the rats get this far apart (e.g. one of them fell off and respawned). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Tail", meta = (ClampMin = "0"))
	float BreakDistance = 500.f;

private:
	TWeakObjectPtr<URatRaceTailComponent> HeldTail;
	TWeakObjectPtr<URatRaceTailComponent> Holder;

	/** Owner's bOrientRotationToMovement before its tail was grabbed; restored on release. */
	bool bSavedOrientRotationToMovement = true;
};
