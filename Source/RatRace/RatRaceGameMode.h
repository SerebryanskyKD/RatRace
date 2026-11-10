#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RatRaceGameMode.generated.h"

class ACameraActor;
class ARatRaceArenaBuilder;
class ARatRaceCatPawsHazard;

UENUM(BlueprintType)
enum class ERatRaceMatchPhase : uint8
{
	Countdown,
	Playing,
	RoundResults,
	MatchResults
};

/** Local multiplayer on one shared screen: creates local players, spawns and respawns them. */
UCLASS()
class RATRACE_API ARatRaceGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARatRaceGameMode();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	/** Shared fixed camera every player controller views through. Builds the arena on demand. */
	ACameraActor* GetArenaCamera();

	UFUNCTION(BlueprintCallable, Category = "RatRace|Match")
	void FinishCurrentRound();

	void EliminatePlayerForRound(AController* Controller);
	bool IsPlayerEliminated(AController* Controller) const;

	UFUNCTION(BlueprintCallable, Category = "RatRace|Match")
	bool StartNextMatch();

	UFUNCTION(BlueprintPure, Category = "RatRace|Match")
	ERatRaceMatchPhase GetMatchPhase() const { return MatchPhase; }

	UFUNCTION(BlueprintPure, Category = "RatRace|Match")
	int32 GetCurrentRound() const { return CurrentRound; }

	UFUNCTION(BlueprintPure, Category = "RatRace|Match")
	float GetPhaseTimeRemaining() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace", meta = (ClampMin = "2", ClampMax = "4"))
	int32 NumLocalPlayers = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Match", meta = (ClampMin = "1"))
	int32 NumRounds = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Match", meta = (ClampMin = "0"))
	float CountdownDuration = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Match", meta = (ClampMin = "1"))
	float RoundDuration = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Match", meta = (ClampMin = "0"))
	float ResultsDuration = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Match")
	bool bShowMatchStatus = true;

	/** Spawn point per player index (feet on the floor, capsule offset is added automatically). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace")
	TArray<FVector> SpawnPoints;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace")
	TArray<FLinearColor> PlayerColors;

	/** Players below this height are teleported back to their spawn point. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace")
	float RespawnZ = -500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace")
	TSubclassOf<ARatRaceArenaBuilder> ArenaBuilderClass;

	/** Print the gamepad -> player assignment and live pad input on screen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RatRace|Debug")
	bool bShowGamepadAssignment = true;

private:
	/** Maps the N-th connected gamepad to the N-th local player's platform user. */
	void AssignGamepadsToPlayers();
	void HandleInputDeviceConnectionChange(EInputDeviceConnectionState NewState, FPlatformUserId UserId, FInputDeviceId DeviceId);

	ARatRaceArenaBuilder* EnsureArena();
	int32 GetPlayerIndex(const AController* Controller) const;
	FTransform GetSpawnTransform(int32 PlayerIndex) const;
	void StartMatch();
	void StartRoundCountdown();
	void StartRound();
	void ContinueAfterRoundResults();
	void SetPlayersCanMove(bool bCanMove);
	void ResetPlayersForRound();
	bool StartCatPawsHazards();
	void StopCatPawsHazards();
	int32 GetAlivePlayerCount() const;

	UPROPERTY(Transient)
	TObjectPtr<ARatRaceArenaBuilder> Arena;

	UPROPERTY(Transient)
	TObjectPtr<ARatRaceCatPawsHazard> CatPawsHazard;

	TSet<TWeakObjectPtr<AController>> EliminatedPlayers;

	ERatRaceMatchPhase MatchPhase = ERatRaceMatchPhase::Countdown;

	int32 CurrentRound = 0;

	FTimerHandle PhaseTimerHandle;
	FTimerHandle RoundTimerHandle;

	bool bMatchMovementLockApplied = false;

	FDelegateHandle DeviceConnectionHandle;
	TSharedPtr<class FRatRaceGamepadRouter> GamepadRouter;
	bool bGamepadAssignmentDirty = false;
};
