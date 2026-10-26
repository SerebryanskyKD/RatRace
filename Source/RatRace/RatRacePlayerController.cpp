#include "RatRacePlayerController.h"

#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Character.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "RatRaceGameMode.h"

ARatRacePlayerController::ARatRacePlayerController()
{
	// Otherwise Possess() would switch the view target to the pawn.
	bAutoManageActiveCameraTarget = false;
}

void ARatRacePlayerController::BeginPlay()
{
	Super::BeginPlay();
	AddMappingContext();
	ViewArenaCamera();
}

void ARatRacePlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	// Controllers created by UGameplayStatics::CreatePlayer begin play *before* they get their
	// ULocalPlayer, so the mapping context can only be added once the player arrives.
	AddMappingContext();
}

void ARatRacePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ViewArenaCamera();
}

void ARatRacePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	CreateInputActions();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogTemp, Error, TEXT("RatRace: InputComponent is not UEnhancedInputComponent, check DefaultInputComponentClass in DefaultInput.ini"));
		return;
	}

	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARatRacePlayerController::HandleMove);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ARatRacePlayerController::HandleJumpStarted);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ARatRacePlayerController::HandleJumpCompleted);
}

void ARatRacePlayerController::CreateInputActions()
{
	if (MoveAction)
	{
		return;
	}

	MoveAction = NewObject<UInputAction>(this, TEXT("IA_Move"));
	MoveAction->ValueType = EInputActionValueType::Axis2D;

	JumpAction = NewObject<UInputAction>(this, TEXT("IA_Jump"));
	JumpAction->ValueType = EInputActionValueType::Boolean;
}

void ARatRacePlayerController::AddMappingContext()
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
	if (!Subsystem)
	{
		return;
	}

	CreateInputActions();

	if (!MappingContext)
	{
		MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_RatRace"));

		// Gamepad: the device -> player routing is done by the engine (gamepad N -> local player N,
		// see bOffsetPlayerGamepadIds), so every player maps the same keys.
		FEnhancedActionKeyMapping& StickMapping = MappingContext->MapKey(MoveAction, EKeys::Gamepad_Left2D);
		UInputModifierDeadZone* DeadZone = NewObject<UInputModifierDeadZone>(MappingContext);
		DeadZone->Type = EDeadZoneType::Radial;
		DeadZone->LowerThreshold = StickDeadZone;
		StickMapping.Modifiers.Add(DeadZone);

		MappingContext->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);

		// Keyboard always routes to the first local player, so only map it there.
		if (bKeyboardForFirstPlayer && LocalPlayer->GetLocalPlayerIndex() == 0)
		{
			// Keys produce (1,0,0); swizzle moves the value to Y for forward/back, negate flips it.
			UInputModifierSwizzleAxis* ToY = NewObject<UInputModifierSwizzleAxis>(MappingContext);
			ToY->Order = EInputAxisSwizzle::YXZ;
			UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(MappingContext);

			MappingContext->MapKey(MoveAction, EKeys::W).Modifiers.Add(ToY);
			FEnhancedActionKeyMapping& Back = MappingContext->MapKey(MoveAction, EKeys::S);
			Back.Modifiers.Add(ToY);
			Back.Modifiers.Add(Negate);
			MappingContext->MapKey(MoveAction, EKeys::A).Modifiers.Add(Negate);
			MappingContext->MapKey(MoveAction, EKeys::D);

			MappingContext->MapKey(JumpAction, EKeys::SpaceBar);
		}
	}

	if (!Subsystem->HasMappingContext(MappingContext))
	{
		Subsystem->AddMappingContext(MappingContext, 0);
	}
}

void ARatRacePlayerController::ViewArenaCamera()
{
	if (ARatRaceGameMode* GameMode = GetWorld()->GetAuthGameMode<ARatRaceGameMode>())
	{
		if (ACameraActor* Camera = GameMode->GetArenaCamera())
		{
			SetViewTarget(Camera);
		}
	}
}

void ARatRacePlayerController::HandleMove(const FInputActionValue& Value)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	const FVector2D Input = Value.Get<FVector2D>();

	// Move relative to the shared camera's yaw (stick up = away from camera), ignoring pitch.
	// The controller rotation is meaningless here since nobody steers the camera.
	const float CameraYaw = PlayerCameraManager ? PlayerCameraManager->GetCameraRotation().Yaw : 0.f;
	const FRotator YawRotation(0.f, CameraYaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	ControlledPawn->AddMovementInput(Forward, Input.Y);
	ControlledPawn->AddMovementInput(Right, Input.X);
}

void ARatRacePlayerController::HandleJumpStarted()
{
	if (ACharacter* ControlledCharacter = GetCharacter())
	{
		ControlledCharacter->Jump();
	}
}

void ARatRacePlayerController::HandleJumpCompleted()
{
	if (ACharacter* ControlledCharacter = GetCharacter())
	{
		ControlledCharacter->StopJumping();
	}
}
