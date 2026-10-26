#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RatRaceArenaBuilder.generated.h"

class ACameraActor;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

USTRUCT(BlueprintType)
struct FRatRacePlatform
{
	GENERATED_BODY()

	/** Box center relative to the arena origin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	FVector Center = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	FVector Size = FVector(400.f, 400.f, 100.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	FLinearColor Color = FLinearColor(0.5f, 0.5f, 0.55f);
};

/**
 * Builds the whole play space at runtime so the game works on an empty level:
 * floor + platforms from engine cubes, the fixed shared camera and (if missing) lights.
 * Floor top is at Z = 0; X points away from the camera, Y is screen-right.
 */
UCLASS()
class RATRACE_API ARatRaceArenaBuilder : public AActor
{
	GENERATED_BODY()

public:
	ARatRaceArenaBuilder();

	virtual void BeginPlay() override;

	/** Safe to call more than once; builds only the first time. */
	void BuildArena();

	ACameraActor* GetArenaCamera() const { return ArenaCamera; }

protected:
	/** X = depth (away from camera), Y = width, Z = thickness. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena")
	FVector FloorSize = FVector(2000.f, 3000.f, 100.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena")
	FLinearColor FloorColor = FLinearColor(0.25f, 0.25f, 0.28f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena")
	TArray<FRatRacePlatform> Platforms;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float CameraPitch = -55.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float CameraYaw = 0.f;

	/** Horizontal field of view. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "10", ClampMax = "170"))
	float CameraFOV = 60.f;

	/** Pull the camera back until the floor and all platforms fit in a 16:9 frame. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	bool bAutoFitCamera = true;

	/** Fraction of the frame the arena may occupy when auto-fitting (1 = edge to edge). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (EditCondition = "bAutoFitCamera", ClampMin = "0.1", ClampMax = "1"))
	float CameraFitMargin = 0.92f;

	/** Distance from the arena center, used when bAutoFitCamera is off. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (EditCondition = "!bAutoFitCamera"))
	float CameraDistance = 4000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting")
	bool bSpawnLightsIfMissing = true;

private:
	UStaticMeshComponent* AddBox(const FName Name, const FVector& Center, const FVector& Size, const FLinearColor& Color);
	float ComputeFitDistance(const FRotator& ViewRotation) const;
	void SpawnCamera();
	void SpawnLightsIfMissing();

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> ShapeMaterial;

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> ArenaCamera;

	bool bBuilt = false;
};
