
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CoopGameInstance.generated.h"

class APlayerController;

UCLASS()
class RATRACE_API UCoopGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coop")
    int32 MaxPlayers = 4;

    UFUNCTION(BlueprintCallable, Category = "Coop")
    bool TryJoinNextPlayer();

    UFUNCTION(BlueprintCallable, Category = "Coop")
    int32 GetJoinedPlayerCount() const;

    UFUNCTION(BlueprintCallable, Category = "Coop")
    int32 GetLocalPlayerIndex(APlayerController* PlayerController) const;
};
