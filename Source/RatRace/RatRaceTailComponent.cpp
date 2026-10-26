#include "RatRaceTailComponent.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace RatRaceTail
{
	UCharacterMovementComponent* GetMovement(const AActor* Actor)
	{
		const ACharacter* Character = Cast<ACharacter>(Actor);
		return Character ? Character->GetCharacterMovement() : nullptr;
	}
}

URatRaceTailComponent::URatRaceTailComponent()
{
	// Only the grabber's component ticks, and only while it holds a tail.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

FVector URatRaceTailComponent::GetGrabPointLocation() const
{
	const AActor* Owner = GetOwner();
	return Owner ? Owner->GetActorTransform().TransformPosition(GrabPointOffset) : GetComponentLocation();
}

bool URatRaceTailComponent::TryGrab()
{
	if (IsHoldingTail() || !IsValid(GetOwner()))
	{
		return false;
	}

	const FVector GrabPoint = GetGrabPointLocation();
	URatRaceTailComponent* BestTail = nullptr;
	float BestDistSq = FMath::Square(GrabReach);

	// At most four rats, so a plain scan is enough.
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		APawn* Other = *It;
		if (Other == GetOwner() || !IsValid(Other))
		{
			continue;
		}

		URatRaceTailComponent* Tail = Other->FindComponentByClass<URatRaceTailComponent>();
		// One holder per tail: whoever grabbed first keeps it until they let go.
		if (!Tail || Tail->Holder.IsValid())
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(GrabPoint, Tail->GetComponentLocation());
		if (DistSq <= BestDistSq)
		{
			BestDistSq = DistSq;
			BestTail = Tail;
		}
	}

	if (!BestTail)
	{
		return false;
	}

	HeldTail = BestTail;
	BestTail->Holder = this;

	// The grabber turns the held rat itself (back to the grabber), so the stick must not.
	if (UCharacterMovementComponent* HeldMovement = RatRaceTail::GetMovement(BestTail->GetOwner()))
	{
		BestTail->bSavedOrientRotationToMovement = HeldMovement->bOrientRotationToMovement;
		HeldMovement->bOrientRotationToMovement = false;
	}

	SetComponentTickEnabled(true);
	return true;
}

void URatRaceTailComponent::Release()
{
	if (URatRaceTailComponent* Tail = HeldTail.Get())
	{
		if (Tail->Holder.Get() == this)
		{
			Tail->Holder.Reset();
			if (UCharacterMovementComponent* HeldMovement = RatRaceTail::GetMovement(Tail->GetOwner()))
			{
				HeldMovement->bOrientRotationToMovement = Tail->bSavedOrientRotationToMovement;
			}
		}
	}
	HeldTail.Reset();
	SetComponentTickEnabled(false);
}

void URatRaceTailComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Release();
	if (URatRaceTailComponent* HolderTail = Holder.Get())
	{
		HolderTail->Release();
	}
	Holder.Reset();

	Super::EndPlay(EndPlayReason);
}

void URatRaceTailComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	URatRaceTailComponent* Tail = HeldTail.Get();
	APawn* Grabber = Cast<APawn>(GetOwner());
	APawn* Held = Tail ? Cast<APawn>(Tail->GetOwner()) : nullptr;
	if (!IsValid(Grabber) || !IsValid(Held) || Tail->Holder.Get() != this)
	{
		Release();
		return;
	}

	const FVector Rope = Grabber->GetActorLocation() - Tail->GetComponentLocation();
	if (Rope.Size() > BreakDistance)
	{
		Release();
		return;
	}

	// Measured along the floor; CharacterMovement ignores vertical input while walking anyway.
	const FVector Rope2D(Rope.X, Rope.Y, 0.f);
	const float Length = Rope2D.Size();
	const FVector Direction = Rope2D.GetSafeNormal();
	if (Direction.IsZero())
	{
		return;
	}

	// Held by the tail: the held rat's back faces the grabber, turning at its normal rate.
	if (UCharacterMovementComponent* HeldMovement = RatRaceTail::GetMovement(Held))
	{
		const FRotator Current = Held->GetActorRotation();
		const FRotator Target(0.f, (-Direction).Rotation().Yaw, 0.f);
		Held->SetActorRotation(FMath::RInterpConstantTo(Current, Target, DeltaTime, HeldMovement->RotationRate.Yaw));
	}

	const float Stretch = Length - SlackLength;
	if (Stretch <= 0.f)
	{
		return;
	}

	// Added on top of the players' own input; CharacterMovement clamps the sum to full stick.
	const float Pull = FMath::Clamp(Stretch / PullRampLength, 0.f, 1.f) * MaxPullStrength;
	Held->AddMovementInput(Direction, Pull);
	Grabber->AddMovementInput(-Direction, Pull * GrabberRecoil);
}
