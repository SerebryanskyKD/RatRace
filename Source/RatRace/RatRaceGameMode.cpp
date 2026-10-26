#include "RatRaceGameMode.h"

#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Kismet/GameplayStatics.h"
#include "RatRaceArenaBuilder.h"
#include "RatRaceCharacter.h"
#include "RatRaceGamepadRouter.h"
#include "RatRacePlayerController.h"

ARatRaceGameMode::ARatRaceGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultPawnClass = ARatRaceCharacter::StaticClass();
	PlayerControllerClass = ARatRacePlayerController::StaticClass();
	ArenaBuilderClass = ARatRaceArenaBuilder::StaticClass();

	SpawnPoints = {
		FVector(-450.f, -650.f, 0.f),
		FVector(-450.f, 650.f, 0.f),
		FVector(450.f, -650.f, 0.f),
		FVector(450.f, 650.f, 0.f),
	};

	PlayerColors = {
		FLinearColor(0.9f, 0.05f, 0.05f), // red
		FLinearColor(0.05f, 0.2f, 0.9f),  // blue
		FLinearColor(0.05f, 0.75f, 0.1f), // green
		FLinearColor(0.95f, 0.8f, 0.05f), // yellow
	};
}

void ARatRaceGameMode::BeginPlay()
{
	Super::BeginPlay();

	EnsureArena();

	UWorld* World = GetWorld();
	if (UGameViewportClient* Viewport = World->GetGameViewport())
	{
		// One shared view: only player 0's view target is rendered, full screen.
		Viewport->SetForceDisableSplitscreen(true);
	}

	// Player 0 already exists (created by the engine). Add the rest with ControllerId == index,
	// which maps them to the platform user / gamepad with the same index.
	const int32 PlayerCount = FMath::Clamp(NumLocalPlayers, 2, 4);
	for (int32 ControllerId = 1; ControllerId < PlayerCount; ++ControllerId)
	{
		if (!UGameplayStatics::GetPlayerControllerFromID(World, ControllerId))
		{
			if (!UGameplayStatics::CreatePlayer(World, ControllerId, true))
			{
				UE_LOG(LogTemp, Warning, TEXT("RatRace: failed to create local player %d"), ControllerId);
			}
		}
	}

	AssignGamepadsToPlayers();
	DeviceConnectionHandle = IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().AddUObject(
		this, &ARatRaceGameMode::HandleInputDeviceConnectionChange);

	// Deliver pads of players 1..N to the game regardless of Slate's per-user focus.
	if (FSlateApplication::IsInitialized())
	{
		GamepadRouter = MakeShared<FRatRaceGamepadRouter>(World->GetGameViewport(), bShowGamepadAssignment);
		FSlateApplication::Get().RegisterInputPreProcessor(GamepadRouter);
	}
}

void ARatRaceGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().Remove(DeviceConnectionHandle);
	if (GamepadRouter.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(GamepadRouter);
	}
	GamepadRouter.Reset();
	Super::EndPlay(EndPlayReason);
}

void ARatRaceGameMode::HandleInputDeviceConnectionChange(EInputDeviceConnectionState NewState, FPlatformUserId UserId, FInputDeviceId DeviceId)
{
	// Fired from inside the input device's own update; remap on the next tick instead.
	bGamepadAssignmentDirty = true;
}

void ARatRaceGameMode::AssignGamepadsToPlayers()
{
	bGamepadAssignmentDirty = false;

	// Input events reach the local player whose FPlatformUserId owns the device. With Windows'
	// default policy (PrimaryUserSharesKeyboardAndFirstGamepad) which user a pad lands on depends
	// on connection order, so pads can end up sharing player 0. Pin them explicitly instead:
	// keyboard stays with player 0, gamepads go to players 0..N-1 in connection order.
	IPlatformInputDeviceMapper& DeviceMapper = IPlatformInputDeviceMapper::Get();
	const TArray<ULocalPlayer*>& LocalPlayers = GetGameInstance()->GetLocalPlayers();

	TArray<FInputDeviceId> Gamepads;
	DeviceMapper.GetAllConnectedInputDevices(Gamepads);
	Gamepads.Remove(DeviceMapper.GetDefaultInputDevice()); // keyboard & mouse
	Gamepads.Sort([](const FInputDeviceId& A, const FInputDeviceId& B) { return A.GetId() < B.GetId(); });

	FString Summary = FString::Printf(TEXT("RatRace: %d gamepad(s), %d player(s)"), Gamepads.Num(), LocalPlayers.Num());
	for (int32 i = 0; i < Gamepads.Num(); ++i)
	{
		if (!LocalPlayers.IsValidIndex(i))
		{
			Summary += FString::Printf(TEXT("\n  device %d -> no free player"), Gamepads[i].GetId());
			continue;
		}

		const FPlatformUserId TargetUser = LocalPlayers[i]->GetPlatformUserId();
		const FPlatformUserId CurrentUser = DeviceMapper.GetUserForInputDevice(Gamepads[i]);
		if (CurrentUser != TargetUser)
		{
			DeviceMapper.Internal_ChangeInputDeviceUserMapping(Gamepads[i], TargetUser, CurrentUser);
		}
		Summary += FString::Printf(TEXT("\n  device %d -> player %d (user %d, was %d)"),
			Gamepads[i].GetId(), i, TargetUser.GetInternalId(), CurrentUser.GetInternalId());
	}

	UE_LOG(LogTemp, Log, TEXT("%s"), *Summary);
	if (bShowGamepadAssignment && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Cyan, Summary);
	}
}

void ARatRaceGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bGamepadAssignmentDirty)
	{
		AssignGamepadsToPlayers();
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		if (!Pawn || Pawn->GetActorLocation().Z >= RespawnZ)
		{
			continue;
		}

		const FTransform Spawn = GetSpawnTransform(GetPlayerIndex(PC));
		Pawn->TeleportTo(Spawn.GetLocation(), Spawn.Rotator(), false, true);
		if (ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			Character->GetCharacterMovement()->StopMovementImmediately();
		}
	}
}

void ARatRaceGameMode::RestartPlayer(AController* NewPlayer)
{
	if (!NewPlayer || NewPlayer->IsPendingKillPending())
	{
		return;
	}

	// The first player is spawned before BeginPlay, so the arena may not exist yet.
	EnsureArena();

	// No PlayerStarts on an empty level: place players at fixed corner spots instead.
	const int32 PlayerIndex = GetPlayerIndex(NewPlayer);
	RestartPlayerAtTransform(NewPlayer, GetSpawnTransform(PlayerIndex));

	if (ARatRaceCharacter* Character = Cast<ARatRaceCharacter>(NewPlayer->GetPawn()))
	{
		if (PlayerColors.Num() > 0)
		{
			Character->SetPlayerColor(PlayerColors[PlayerIndex % PlayerColors.Num()]);
		}
	}
}

ACameraActor* ARatRaceGameMode::GetArenaCamera()
{
	ARatRaceArenaBuilder* Builder = EnsureArena();
	return Builder ? Builder->GetArenaCamera() : nullptr;
}

ARatRaceArenaBuilder* ARatRaceGameMode::EnsureArena()
{
	if (Arena)
	{
		return Arena;
	}

	UWorld* World = GetWorld();

	// Reuse a builder placed in the level by hand, otherwise spawn one.
	for (TActorIterator<ARatRaceArenaBuilder> It(World); It; ++It)
	{
		Arena = *It;
		break;
	}

	if (!Arena && ArenaBuilderClass)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Arena = World->SpawnActor<ARatRaceArenaBuilder>(ArenaBuilderClass, FTransform::Identity, Params);
	}

	if (Arena)
	{
		Arena->BuildArena();
	}
	return Arena;
}

int32 ARatRaceGameMode::GetPlayerIndex(const AController* Controller) const
{
	const APlayerController* PC = Cast<APlayerController>(Controller);
	const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	return LocalPlayer ? FMath::Max(LocalPlayer->GetLocalPlayerIndex(), 0) : 0;
}

FTransform ARatRaceGameMode::GetSpawnTransform(int32 PlayerIndex) const
{
	FVector Location = SpawnPoints.Num() > 0 ? SpawnPoints[PlayerIndex % SpawnPoints.Num()] : FVector::ZeroVector;

	// Spawn points are at foot level; lift by the capsule so the pawn doesn't start inside the floor.
	float HalfHeight = 90.f;
	if (const ACharacter* PawnCDO = Cast<ACharacter>(DefaultPawnClass.GetDefaultObject()))
	{
		HalfHeight = PawnCDO->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	}
	Location.Z += HalfHeight + 5.f;

	// Face the arena center.
	const FVector ToCenter = FVector(-Location.X, -Location.Y, 0.f);
	const FRotator Rotation = ToCenter.IsNearlyZero() ? FRotator::ZeroRotator : ToCenter.Rotation();
	return FTransform(Rotation, Location);
}
