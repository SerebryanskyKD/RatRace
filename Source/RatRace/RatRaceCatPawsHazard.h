#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RatRaceCatPawsHazard.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class ARatRaceGameMode;

UCLASS()
class RATRACE_API ARatRaceCatPawsHazard : public AActor
{
	GENERATED_BODY()

public:
	ARatRaceCatPawsHazard();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool StartHazards(const FVector& InArenaCenter, const FVector& InFloorSize, float InMaxSurfaceHeight);
	void StopHazards();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Cat Paws|Timing", meta = (ClampMin = "0.1"))
	float FirstAttackDelay = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Cat Paws|Timing", meta = (ClampMin = "0.1"))
	float TelegraphDuration = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Cat Paws|Timing", meta = (ClampMin = "0.1"))
	float ActiveDuration = 0.65f;

	UPROPERTY(EditDefaultsOnly, Category = "Cat Paws|Timing", meta = (ClampMin = "0.1"))
	float InitialAttackInterval = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Cat Paws|Timing", meta = (ClampMin = "0.1"))
	float MinimumAttackInterval = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Cat Paws|Timing", meta = (ClampMin = "0"))
	float IntervalReductionPerStrike = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Cat Paws|Shape", meta = (ClampMin = "100"))
	float MinStrikeLength = 900.f;

	UPROPERTY(EditDefaultsOnly, Category = "Cat Paws|Shape", meta = (ClampMin = "100"))
	float MaxStrikeLength = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Cat Paws|Shape", meta = (ClampMin = "100"))
	float MinStrikeWidth = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Cat Paws|Shape", meta = (ClampMin = "100"))
	float MaxStrikeWidth = 1000.f;

private:
	void ScheduleNextAttack(float Delay);
	void BeginTelegraph();
	void ActivateStrike();
	void EndStrike();
	void PlaceStrike();

	UFUNCTION()
	void HandleHitboxOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> WarningMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ImpactMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> Hitbox;

	TWeakObjectPtr<ARatRaceGameMode> GameMode;
	FVector ArenaCenter = FVector::ZeroVector;
	FVector2D ArenaHalfSize = FVector2D(1000.f, 1500.f);
	float VisualHeight = 0.f;
	FTimerHandle AttackTimerHandle;
	int32 AttackCount = 0;
	bool bRunning = false;
	bool bStrikeActive = false;
};
