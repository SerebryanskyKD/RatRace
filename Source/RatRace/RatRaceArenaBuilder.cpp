#include "RatRaceArenaBuilder.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace RatRaceArena
{
	constexpr float BasicShapeSize = 100.f;
	constexpr float ViewAspect = 16.f / 9.f;
	const FName ColorParam(TEXT("Color"));

	FRatRacePlatform MakePlatform(float X, float Y, float Width, float Depth, float Height)
	{
		FRatRacePlatform Platform;
		Platform.Center = FVector(X, Y, Height * 0.5f);
		Platform.Size = FVector(Depth, Width, Height);
		return Platform;
	}
}

ARatRaceArenaBuilder::ARatRaceArenaBuilder()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = Cube.Object;
	ShapeMaterial = Material.Object;

	// Character jump apex is ~165 uu (JumpZVelocity 700, GravityScale 1.5), so steps stay below that
	// and the tall ones need a run-up from a neighbouring platform.
	using RatRaceArena::MakePlatform;
	Platforms = {
		MakePlatform(0.f, 0.f, 500.f, 500.f, 80.f),       // center block
		MakePlatform(750.f, 0.f, 200.f, 200.f, 40.f),     // low step, far side
		MakePlatform(-750.f, 0.f, 500.f, 300.f, 200.f),   // near ledge, reachable from the center block
		MakePlatform(0.f, -1150.f, 400.f, 400.f, 150.f),  // left, max single jump
		MakePlatform(0.f, 1050.f, 300.f, 400.f, 120.f),   // right, first step
		MakePlatform(0.f, 1350.f, 200.f, 300.f, 250.f),   // right, second step
	};
	const FLinearColor PlatformColors[] = {
		FLinearColor(0.55f, 0.45f, 0.35f),
		FLinearColor(0.45f, 0.55f, 0.6f),
	};
	for (int32 i = 0; i < Platforms.Num(); ++i)
	{
		Platforms[i].Color = PlatformColors[i % UE_ARRAY_COUNT(PlatformColors)];
	}
}

void ARatRaceArenaBuilder::BeginPlay()
{
	Super::BeginPlay();
	BuildArena();
}

void ARatRaceArenaBuilder::BuildArena()
{
	if (bBuilt)
	{
		return;
	}
	bBuilt = true;

	// Floor top sits at Z = 0; no walls so players can fall off the edges.
	AddBox(TEXT("Floor"), FVector(0.f, 0.f, -FloorSize.Z * 0.5f), FloorSize, FloorColor);

	for (int32 i = 0; i < Platforms.Num(); ++i)
	{
		const FRatRacePlatform& Platform = Platforms[i];
		AddBox(*FString::Printf(TEXT("Platform_%d"), i), Platform.Center, Platform.Size, Platform.Color);
	}

	SpawnCamera();

	if (bSpawnLightsIfMissing)
	{
		SpawnLightsIfMissing();
	}
}

UStaticMeshComponent* ARatRaceArenaBuilder::AddBox(const FName Name, const FVector& Center, const FVector& Size, const FLinearColor& Color)
{
	UStaticMeshComponent* Box = NewObject<UStaticMeshComponent>(this, Name);
	Box->SetMobility(EComponentMobility::Static);
	Box->SetStaticMesh(CubeMesh);
	Box->SetupAttachment(RootComponent);
	Box->SetRelativeLocation(Center);
	Box->SetRelativeScale3D(Size / RatRaceArena::BasicShapeSize);
	Box->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Box->RegisterComponent();
	AddInstanceComponent(Box);

	if (ShapeMaterial)
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(ShapeMaterial, Box);
		Material->SetVectorParameterValue(RatRaceArena::ColorParam, Color);
		Box->SetMaterial(0, Material);
	}
	return Box;
}

float ARatRaceArenaBuilder::ComputeFitDistance(const FRotator& ViewRotation) const
{
	// Top corners of the floor and every platform, in world space.
	TArray<FVector> Points;
	auto AddTopCorners = [this, &Points](const FVector& Center, const FVector& Size)
	{
		const FVector Half = Size * 0.5f;
		for (const float SX : { -1.f, 1.f })
		{
			for (const float SY : { -1.f, 1.f })
			{
				Points.Add(GetActorTransform().TransformPosition(Center + FVector(SX * Half.X, SY * Half.Y, Half.Z)));
			}
		}
	};
	AddTopCorners(FVector(0.f, 0.f, -FloorSize.Z * 0.5f), FloorSize);
	for (const FRatRacePlatform& Platform : Platforms)
	{
		AddTopCorners(Platform.Center, Platform.Size);
	}

	// CameraFOV is horizontal; derive the vertical half-angle for a 16:9 frame.
	const float TanHalfH = FMath::Tan(FMath::DegreesToRadians(CameraFOV * 0.5f)) * CameraFitMargin;
	const float TanHalfV = TanHalfH / RatRaceArena::ViewAspect;
	const FVector Target = GetActorLocation();
	const FVector Forward = ViewRotation.Vector();

	for (float Distance = 500.f; Distance < 50000.f; Distance += 50.f)
	{
		const FVector CameraLocation = Target - Forward * Distance;
		bool bAllVisible = true;
		for (const FVector& Point : Points)
		{
			// View space: X forward, Y right, Z up.
			const FVector Local = ViewRotation.UnrotateVector(Point - CameraLocation);
			if (Local.X <= 1.f || FMath::Abs(Local.Y) > Local.X * TanHalfH || FMath::Abs(Local.Z) > Local.X * TanHalfV)
			{
				bAllVisible = false;
				break;
			}
		}
		if (bAllVisible)
		{
			return Distance;
		}
	}
	return CameraDistance;
}

void ARatRaceArenaBuilder::SpawnCamera()
{
	const FRotator ViewRotation(CameraPitch, CameraYaw, 0.f);
	const float Distance = bAutoFitCamera ? ComputeFitDistance(ViewRotation) : CameraDistance;
	const FVector Location = GetActorLocation() - ViewRotation.Vector() * Distance;

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ArenaCamera = GetWorld()->SpawnActor<ACameraActor>(Location, ViewRotation, Params);
	if (ArenaCamera)
	{
		UCameraComponent* Camera = ArenaCamera->GetCameraComponent();
		Camera->SetFieldOfView(CameraFOV);
		// No letterboxing: on non-16:9 screens the horizontal FOV is kept and the arena still fits.
		Camera->SetConstraintAspectRatio(false);
	}
}

void ARatRaceArenaBuilder::SpawnLightsIfMissing()
{
	UWorld* World = GetWorld();

	if (!TActorIterator<ADirectionalLight>(World))
	{
		const FTransform SunTransform(FRotator(-50.f, -35.f, 0.f));
		ADirectionalLight* Sun = World->SpawnActorDeferred<ADirectionalLight>(ADirectionalLight::StaticClass(), SunTransform);
		if (UDirectionalLightComponent* SunLight = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
		{
			SunLight->SetMobility(EComponentMobility::Movable);
			SunLight->bAtmosphereSunLight = true;
		}
		Sun->FinishSpawning(SunTransform);
	}

	// Gives the empty level a sky for the sky light to capture.
	if (!TActorIterator<ASkyAtmosphere>(World))
	{
		World->SpawnActor<ASkyAtmosphere>();
	}

	if (!TActorIterator<ASkyLight>(World))
	{
		ASkyLight* Sky = World->SpawnActorDeferred<ASkyLight>(ASkyLight::StaticClass(), FTransform::Identity);
		if (USkyLightComponent* SkyLight = Sky->GetLightComponent())
		{
			SkyLight->SetMobility(EComponentMobility::Movable);
			// Real-time capture works with a movable sky light and no baked cubemap.
			SkyLight->bRealTimeCapture = true;
		}
		Sky->FinishSpawning(FTransform::Identity);
	}
}
