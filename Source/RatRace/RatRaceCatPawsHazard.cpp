#include "RatRaceCatPawsHazard.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "RatRaceCharacter.h"
#include "RatRaceGameMode.h"
#include "UObject/ConstructorHelpers.h"

namespace RatRaceCatPaws
{
	constexpr float BasicShapeSize = 100.f;
	constexpr float MeshHeightScale = 0.08f;
	constexpr float MeshHeightOffset = 5.f;
	constexpr float HitboxCenterHeight = 400.f;
	constexpr float HitboxHalfHeight = 400.f;
	const FName ColorParam(TEXT("Color"));
}

ARatRaceCatPawsHazard::ARatRaceCatPawsHazard()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	WarningMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WarningMesh"));
	WarningMesh->SetupAttachment(SceneRoot);
	WarningMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WarningMesh->SetVisibility(false);

	ImpactMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ImpactMesh"));
	ImpactMesh->SetupAttachment(SceneRoot);
	ImpactMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ImpactMesh->SetVisibility(false);

	Hitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("Hitbox"));
	Hitbox->SetupAttachment(SceneRoot);
	Hitbox->SetCollisionObjectType(ECC_WorldDynamic);
	Hitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
	Hitbox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Hitbox->SetGenerateOverlapEvents(true);
	Hitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Hitbox->OnComponentBeginOverlap.AddDynamic(this, &ARatRaceCatPawsHazard::HandleHitboxOverlap);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	if (CubeMesh.Succeeded())
	{
		WarningMesh->SetStaticMesh(CubeMesh.Object);
		ImpactMesh->SetStaticMesh(CubeMesh.Object);
	}

	if (ShapeMaterial.Succeeded())
	{
		if (UMaterialInstanceDynamic* WarningMaterial = WarningMesh->CreateDynamicMaterialInstance(0, ShapeMaterial.Object))
		{
			WarningMaterial->SetVectorParameterValue(RatRaceCatPaws::ColorParam, FLinearColor(1.f, 0.72f, 0.05f));
		}
		if (UMaterialInstanceDynamic* ImpactMaterial = ImpactMesh->CreateDynamicMaterialInstance(0, ShapeMaterial.Object))
		{
			ImpactMaterial->SetVectorParameterValue(RatRaceCatPaws::ColorParam, FLinearColor(0.9f, 0.04f, 0.02f));
		}
	}
}

void ARatRaceCatPawsHazard::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopHazards();
	Super::EndPlay(EndPlayReason);
}

bool ARatRaceCatPawsHazard::StartHazards(
	const FVector& InArenaCenter,
	const FVector& InFloorSize,
	float InMaxSurfaceHeight)
{
	if (!WarningMesh->GetStaticMesh() || !ImpactMesh->GetStaticMesh())
	{
		UE_LOG(LogTemp, Error, TEXT("Cat Paws: warning and impact meshes must be assigned."));
		return false;
	}

	UWorld* World = GetWorld();
	GameMode = World ? Cast<ARatRaceGameMode>(World->GetAuthGameMode()) : nullptr;
	if (!World || !GameMode.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Cat Paws: could not find the active RatRace game mode."));
		return false;
	}

	ArenaCenter = InArenaCenter;
	VisualHeight = FMath::Max(
		InMaxSurfaceHeight + RatRaceCatPaws::MeshHeightOffset,
		ArenaCenter.Z + RatRaceCatPaws::MeshHeightOffset);
	ArenaHalfSize = FVector2D(FMath::Abs(InFloorSize.X) * 0.5f, FMath::Abs(InFloorSize.Y) * 0.5f);
	const float SmallestWidth = FMath::Min(MinStrikeWidth, MaxStrikeWidth);
	if (ArenaHalfSize.X * 2.f < SmallestWidth || ArenaHalfSize.Y * 2.f < SmallestWidth)
	{
		UE_LOG(LogTemp, Error, TEXT("Cat Paws: arena floor is too small for the configured strike width."));
		return false;
	}

	AttackCount = 0;
	bRunning = true;
	ScheduleNextAttack(FirstAttackDelay);
	return true;
}

void ARatRaceCatPawsHazard::StopHazards()
{
	bRunning = false;
	bStrikeActive = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackTimerHandle);
	}
	Hitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WarningMesh->SetVisibility(false);
	ImpactMesh->SetVisibility(false);
}

void ARatRaceCatPawsHazard::ScheduleNextAttack(float Delay)
{
	if (bRunning)
	{
		GetWorldTimerManager().SetTimer(
			AttackTimerHandle,
			this,
			&ARatRaceCatPawsHazard::BeginTelegraph,
			Delay,
			false);
	}
}

void ARatRaceCatPawsHazard::BeginTelegraph()
{
	if (!bRunning)
	{
		return;
	}

	PlaceStrike();
	bStrikeActive = false;
	Hitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ImpactMesh->SetVisibility(false);
	WarningMesh->SetVisibility(true);

	GetWorldTimerManager().SetTimer(
		AttackTimerHandle,
		this,
		&ARatRaceCatPawsHazard::ActivateStrike,
		TelegraphDuration,
		false);
}

void ARatRaceCatPawsHazard::ActivateStrike()
{
	if (!bRunning)
	{
		return;
	}

	bStrikeActive = true;
	WarningMesh->SetVisibility(false);
	ImpactMesh->SetVisibility(true);
	Hitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	GetWorldTimerManager().SetTimer(
		AttackTimerHandle,
		this,
		&ARatRaceCatPawsHazard::EndStrike,
		ActiveDuration,
		false);
}

void ARatRaceCatPawsHazard::EndStrike()
{
	bStrikeActive = false;
	Hitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ImpactMesh->SetVisibility(false);

	++AttackCount;
	const float NextInterval = FMath::Max(
		MinimumAttackInterval,
		InitialAttackInterval - AttackCount * IntervalReductionPerStrike);
	ScheduleNextAttack(NextInterval);
}

void ARatRaceCatPawsHazard::PlaceStrike()
{
	const bool bAlongX = FMath::RandBool();
	const bool bPositiveSide = FMath::RandBool();
	const float LengthMin = FMath::Min(MinStrikeLength, MaxStrikeLength);
	const float LengthMax = FMath::Max(MinStrikeLength, MaxStrikeLength);
	const float WidthMin = FMath::Min(MinStrikeWidth, MaxStrikeWidth);
	const float WidthMax = FMath::Max(MinStrikeWidth, MaxStrikeWidth);
	const float MaxLength = bAlongX ? ArenaHalfSize.X * 2.f : ArenaHalfSize.Y * 2.f;
	const float MaxWidth = bAlongX ? ArenaHalfSize.Y * 2.f : ArenaHalfSize.X * 2.f;
	const float Length = FMath::Min(FMath::FRandRange(LengthMin, LengthMax), MaxLength);
	const float Width = FMath::Min(FMath::FRandRange(WidthMin, WidthMax), MaxWidth);

	float X = ArenaCenter.X;
	float Y = ArenaCenter.Y;

	if (bAlongX)
	{
		const float LateralLimit = FMath::Max(0.f, ArenaHalfSize.Y - Width * 0.5f);
		X += bPositiveSide ? ArenaHalfSize.X - Length * 0.5f : -ArenaHalfSize.X + Length * 0.5f;
		Y += FMath::FRandRange(-LateralLimit, LateralLimit);
	}
	else
	{
		const float LateralLimit = FMath::Max(0.f, ArenaHalfSize.X - Width * 0.5f);
		Y += bPositiveSide ? ArenaHalfSize.Y - Length * 0.5f : -ArenaHalfSize.Y + Length * 0.5f;
		X += FMath::FRandRange(-LateralLimit, LateralLimit);
	}

	SetActorLocation(FVector(X, Y, ArenaCenter.Z));

	const float SizeX = bAlongX ? Length : Width;
	const float SizeY = bAlongX ? Width : Length;
	const FVector MeshScale(SizeX / RatRaceCatPaws::BasicShapeSize,
		SizeY / RatRaceCatPaws::BasicShapeSize,
		RatRaceCatPaws::MeshHeightScale);

	const float VisualHeightOffset = VisualHeight - ArenaCenter.Z;
	WarningMesh->SetRelativeLocation(FVector(0.f, 0.f, VisualHeightOffset));
	ImpactMesh->SetRelativeLocation(FVector(0.f, 0.f, VisualHeightOffset));
	WarningMesh->SetRelativeScale3D(MeshScale);
	ImpactMesh->SetRelativeScale3D(MeshScale);
	Hitbox->SetRelativeLocation(FVector(0.f, 0.f, RatRaceCatPaws::HitboxCenterHeight));
	Hitbox->SetBoxExtent(FVector(
		SizeX * 0.5f,
		SizeY * 0.5f,
		RatRaceCatPaws::HitboxHalfHeight));
}

void ARatRaceCatPawsHazard::HandleHitboxOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bStrikeActive || !OtherActor)
	{
		return;
	}

	ARatRaceCharacter* Rat = Cast<ARatRaceCharacter>(OtherActor);
	ARatRaceGameMode* CurrentGameMode = GameMode.Get();
	if (Rat && Rat->GetController() && CurrentGameMode)
	{
		CurrentGameMode->EliminatePlayerForRound(Rat->GetController());
	}
}
