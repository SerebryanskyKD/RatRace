#include "RatRaceCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "RatRaceTailComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace RatRaceCharacter
{
	constexpr float CapsuleRadius = 40.f;
	constexpr float CapsuleHalfHeight = 90.f;
	// Engine basic shapes are 100x100x100 uu and centered on their pivot.
	constexpr float BasicShapeSize = 100.f;
	const FName ColorParam(TEXT("Color"));

	// Tail runs from inside the lower back of the body down and backwards to just above the floor.
	const FVector TailBase(-CapsuleRadius + 10.f, 0.f, -CapsuleHalfHeight * 0.45f);
	const FVector TailTip(-CapsuleRadius - 105.f, 0.f, -CapsuleHalfHeight + 15.f);
	constexpr float TailThickness = 16.f;
}

ARatRaceCharacter::ARatRaceCharacter()
{
	using namespace RatRaceCharacter;

	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(CapsuleRadius, CapsuleHalfHeight);

	// Rotation follows movement direction, not the controller (the camera is fixed).
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetRelativeScale3D(FVector(
		CapsuleRadius * 2.f / BasicShapeSize,
		CapsuleRadius * 2.f / BasicShapeSize,
		CapsuleHalfHeight * 2.f / BasicShapeSize));

	NoseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NoseMesh"));
	NoseMesh->SetupAttachment(GetCapsuleComponent());
	NoseMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NoseMesh->SetRelativeScale3D(FVector(0.25f));
	NoseMesh->SetRelativeLocation(FVector(CapsuleRadius + 5.f, 0.f, CapsuleHalfHeight * 0.5f));

	// Rigidly attached to the capsule, so it follows every move and turn of the rat.
	const FVector TailSpan = TailTip - TailBase;
	TailMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TailMesh"));
	TailMesh->SetupAttachment(GetCapsuleComponent());
	TailMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// The cylinder's long axis is Z; point it along the tail.
	TailMesh->SetRelativeLocationAndRotation((TailBase + TailTip) * 0.5f, FRotationMatrix::MakeFromZ(TailSpan).Rotator());
	TailMesh->SetRelativeScale3D(FVector(TailThickness / BasicShapeSize, TailThickness / BasicShapeSize, TailSpan.Size() / BasicShapeSize));

	Tail = CreateDefaultSubobject<URatRaceTailComponent>(TEXT("Tail"));
	Tail->SetupAttachment(GetCapsuleComponent());
	Tail->SetRelativeLocation(TailTip);

	if (CylinderMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CylinderMesh.Object);
		TailMesh->SetStaticMesh(CylinderMesh.Object);
	}
	if (CubeMesh.Succeeded())
	{
		NoseMesh->SetStaticMesh(CubeMesh.Object);
	}
	if (ShapeMaterial.Succeeded())
	{
		BodyMesh->SetMaterial(0, ShapeMaterial.Object);
		NoseMesh->SetMaterial(0, ShapeMaterial.Object);
		TailMesh->SetMaterial(0, ShapeMaterial.Object);
	}

	ApplyMovementSettings();
}

void ARatRaceCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Re-apply in case the values were tweaked on the instance in the editor.
	ApplyMovementSettings();

	BodyMaterial = BodyMesh->CreateDynamicMaterialInstance(0);
	NoseMaterial = NoseMesh->CreateDynamicMaterialInstance(0);
	// Share the body material so the tail always has its owner's color.
	if (BodyMaterial)
	{
		TailMesh->SetMaterial(0, BodyMaterial);
	}
	if (NoseMaterial)
	{
		NoseMaterial->SetVectorParameterValue(RatRaceCharacter::ColorParam, NoseColor);
	}
	SetPlayerColor(PlayerColor);
}

void ARatRaceCharacter::SetPlayerColor(FLinearColor InColor)
{
	PlayerColor = InColor;
	if (BodyMaterial)
	{
		BodyMaterial->SetVectorParameterValue(RatRaceCharacter::ColorParam, PlayerColor);
	}
}

void ARatRaceCharacter::ApplyMovementSettings()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, TurnRateYaw, 0.f);
	Movement->MaxWalkSpeed = WalkSpeed;
	Movement->JumpZVelocity = JumpVelocity;
	Movement->AirControl = AirControlAmount;
	Movement->GravityScale = GravityScaleAmount;
}
