#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TAWeaponTypes.h"
#include "TAGameState.generated.h"

UENUM(BlueprintType)
enum class EMatchPhase : uint8
{
    Lobby, Countdown, BuyPhase, RoundInProgress, RoundEnd, MatchEnd
};

UCLASS()
class TACTICALARENA_API ATAGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    UPROPERTY(Replicated, BlueprintReadOnly) EMatchPhase Phase = EMatchPhase::Lobby;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 CountdownSeconds = 5;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 ScoreTeamA = 0;   // "Me" side in 1v1/bot modes
    UPROPERTY(Replicated, BlueprintReadOnly) int32 ScoreTeamB = 0;   // enemy/bot side
    UPROPERTY(Replicated, BlueprintReadOnly) int32 CurrentRound = 1;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 RoundsToWin = 13;
    UPROPERTY(Replicated, BlueprintReadOnly) bool bSuddenDeath = false;
    UPROPERTY(Replicated, BlueprintReadOnly) float RoundTimeRemaining = 100.f;
    UPROPERTY(Replicated, BlueprintReadOnly) ETAMatchMode MatchMode = ETAMatchMode::BotMatch;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};