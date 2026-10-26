#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RatRaceCharacter.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class URatRaceTailComponent;

/** Cylinder-shaped player pawn. No camera: everyone looks through the shared arena camera. */
UCLASS()
class RATRACE_API ARatRaceCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ARatRaceCharacter();

	virtual void PostInitializeComponents() override;

	UFUNCTION(BlueprintCallable, Category = "RatRace")
	void SetPlayerColor(FLinearColor InColor);

	UFUNCTION(BlueprintPure, Category = "RatRace")
	FLinearColor GetPlayerColor() const { return PlayerColor; }

	UFUNCTION(BlueprintPure, Category = "RatRace")
	URatRaceTailComponent* GetTail() const { return Tail; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	/** Small cube in front of the body so the facing direction is readable from above. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> NoseMesh;

	/** Thin cylinder from the back of the body to the tail tip, in the player's color. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TailMesh;

	/** Sits at the tail tip; other rats grab here. Also lets this rat grab other tails. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<URatRaceTailComponent> Tail;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Appearance")
	FLinearColor PlayerColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Appearance")
	FLinearColor NoseColor = FLinearColor(0.02f, 0.02f, 0.02f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Movement", meta = (ClampMin = "0"))
	float WalkSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Movement", meta = (ClampMin = "0"))
	float JumpVelocity = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Movement", meta = (ClampMin = "0", ClampMax = "1"))
	float AirControlAmount = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Movement", meta = (ClampMin = "0"))
	float GravityScaleAmount = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Movement")
	float TurnRateYaw = 720.f;

private:
	void ApplyMovementSettings();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> NoseMaterial;
};
