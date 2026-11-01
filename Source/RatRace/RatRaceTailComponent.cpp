#include "RatRaceTailComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"

URatRaceTailComponent::URatRaceTailComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickEnabled(false);
}

void URatRaceTailComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Release();

	if (URatRaceTailComponent* CurrentHolder = Holder.Get())
	{
		CurrentHolder->Release();
	}

	Super::EndPlay(EndPlayReason);
}

void URatRaceTailComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ApplyPull();
}

bool URatRaceTailComponent::TryGrab()
{
	if (HeldTail.IsValid())
	{
		return true;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	UWorld* World = GetWorld();
	if (!OwnerPawn || !World)
	{
		return false;
	}

	const FVector GrabPoint = OwnerPawn->GetActorTransform().TransformPosition(GrabPointOffset);
	URatRaceTailComponent* ClosestTail = nullptr;
	float ClosestDistanceSquared = FMath::Square(GrabReach);

	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Candidate = *It;
		if (!Candidate || Candidate == OwnerPawn)
		{
			continue;
		}

		URatRaceTailComponent* CandidateTail = Candidate->FindComponentByClass<URatRaceTailComponent>();
		if (!CandidateTail || CandidateTail->Holder.IsValid())
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(GrabPoint, CandidateTail->GetComponentLocation());
		if (DistanceSquared <= ClosestDistanceSquared)
		{
			ClosestDistanceSquared = DistanceSquared;
			ClosestTail = CandidateTail;
		}
	}

	if (!ClosestTail)
	{
		return false;
	}

	HeldTail = ClosestTail;
	ClosestTail->Holder = this;

	if (ACharacter* HeldCharacter = Cast<ACharacter>(ClosestTail->GetOwner()))
	{
		if (UCharacterMovementComponent* Movement = HeldCharacter->GetCharacterMovement())
		{
			ClosestTail->bOriginalOrientRotationToMovement = Movement->bOrientRotationToMovement;
			Movement->bOrientRotationToMovement = false;
		}
	}

	SetComponentTickEnabled(true);
	return true;
}

void URatRaceTailComponent::Release()
{
	if (URatRaceTailComponent* TargetTail = HeldTail.Get())
	{
		if (TargetTail->Holder.Get() == this)
		{
			TargetTail->Holder.Reset();
			if (ACharacter* HeldCharacter = Cast<ACharacter>(TargetTail->GetOwner()))
			{
				if (UCharacterMovementComponent* Movement = HeldCharacter->GetCharacterMovement())
				{
					Movement->bOrientRotationToMovement = TargetTail->bOriginalOrientRotationToMovement;
				}
			}
		}
	}

	HeldTail.Reset();
	SetComponentTickEnabled(false);
}

void URatRaceTailComponent::ApplyPull()
{
	APawn* GrabberPawn = Cast<APawn>(GetOwner());
	URatRaceTailComponent* TargetTail = HeldTail.Get();
	APawn* HeldPawn = TargetTail ? Cast<APawn>(TargetTail->GetOwner()) : nullptr;
	if (!GrabberPawn || !TargetTail || !HeldPawn || TargetTail->Holder.Get() != this)
	{
		Release();
		return;
	}

	FVector Rope = GrabberPawn->GetActorLocation() - TargetTail->GetComponentLocation();
	Rope.Z = 0.f;
	const float Distance = Rope.Size();
	if (Distance > BreakDistance)
	{
		Release();
		return;
	}

	if (Distance <= UE_SMALL_NUMBER)
	{
		return;
	}

	const FVector Direction = Rope / Distance;
	const float Stretch = FMath::Max(0.f, Distance - SlackLength);
	const float Pull = FMath::Clamp(Stretch / PullRampLength, 0.f, 1.f) * MaxPullStrength;
	if (Pull <= 0.f)
	{
		return;
	}

	HeldPawn->AddMovementInput(Direction, Pull);
	GrabberPawn->AddMovementInput(-Direction, Pull * GrabberRecoil);

	if (ACharacter* HeldCharacter = Cast<ACharacter>(HeldPawn))
	{
		const FVector FacingAwayFromGrabber = HeldPawn->GetActorLocation() - GrabberPawn->GetActorLocation();
		if (!FacingAwayFromGrabber.IsNearlyZero())
		{
			const FRotator DesiredRotation = FacingAwayFromGrabber.Rotation();
			HeldCharacter->SetActorRotation(FMath::RInterpConstantTo(
				HeldCharacter->GetActorRotation(),
				DesiredRotation,
				GetWorld()->GetDeltaSeconds(),
				HeldCharacter->GetCharacterMovement()->RotationRate.Yaw));
		}
	}
}
