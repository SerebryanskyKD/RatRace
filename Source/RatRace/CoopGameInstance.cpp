
#include "CoopGameInstance.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"

bool UCoopGameInstance::TryJoinNextPlayer()
{
    if (GetNumLocalPlayers() >= MaxPlayers)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Maximum number of local players reached: %d"),
            MaxPlayers
        );

        return false;
    }

    FString ErrorMessage;

    ULocalPlayer* NewPlayer = CreateLocalPlayer(
        -1,
        ErrorMessage,
        true
    );

    if (NewPlayer == nullptr)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Failed to create local player: %s"),
            *ErrorMessage
        );

        return false;
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("Local players count: %d"),
        GetNumLocalPlayers()
    );

    return true;
}

int32 UCoopGameInstance::GetJoinedPlayerCount() const
{
    return GetNumLocalPlayers();
}

int32 UCoopGameInstance::GetLocalPlayerIndex(
    APlayerController* PlayerController
) const
{
    if (PlayerController == nullptr)
    {
        return INDEX_NONE;
    }

    ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();

    if (LocalPlayer == nullptr)
    {
        return INDEX_NONE;
    }

    const auto& Players = GetLocalPlayers();

    return Players.IndexOfByPredicate(
        [LocalPlayer](const TObjectPtr<ULocalPlayer>& Player)
        {
            return Player.Get() == LocalPlayer;
        }
    );
}
