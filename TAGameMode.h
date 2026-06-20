#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TAWeaponTypes.h"
#include "TAGameMode.generated.h"

class ATACharacter;
class ATAAIController;
class ATAGameState;

/**
 * ATAGameMode
 * Acts as the Match Manager: owns the lobby->countdown->buy->round->score->
 * next-round->match-end flow described in the spec. Server-authoritative;
 * all phase transitions happen here and are pushed to ATAGameState, which
 * replicates them to clients for synchronized countdowns/HUD.
 */
UCLASS()
class TACTICALARENA_API ATAGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ATAGameMode();

    UPROPERTY(EditDefaultsOnly) TSubclassOf<ATACharacter> CharacterClass;
    UPROPERTY(EditDefaultsOnly) TSubclassOf<ATAAIController> BotControllerClass;

    UPROPERTY(EditDefaultsOnly) ETAMatchMode MatchMode = ETAMatchMode::BotMatch;
    UPROPERTY(EditDefaultsOnly) EBotDifficulty BotDifficulty = EBotDifficulty::Normal;
    UPROPERTY(EditDefaultsOnly) int32 BotCount = 1; // 1 for duel/bot match, 3/5/10 for One-vs-All

    virtual void BeginPlay() override;

    /** Called by the Lobby Manager once all players are ready. */
    void StartMatchSequence();

    void OnPlayerKilled(ATACharacter* Victim, ATACharacter* Killer, bool bWasHeadshot);

protected:
    ATAGameState* TAState() const;

    void SpawnCombatants();
    FVector GetSpawnPointForTeam(uint8 TeamId, int32 Index, int32 TotalOnTeam) const;

    void BeginCountdown();
    void TickCountdown();
    void BeginBuyPhase();
    void TickBuyPhase();
    void BeginRound();
    void EndRound(uint8 WinningTeamId);
    void CheckMatchEndCondition();
    void EndMatch(uint8 WinningTeamId);

    FTimerHandle CountdownTimerHandle;
    FTimerHandle BuyPhaseTimerHandle;
    FTimerHandle RoundTimerHandle;

    TArray<ATACharacter*> TeamACharacters; // "me" side
    TArray<ATACharacter*> TeamBCharacters; // bots / opponents

    float BuyPhaseSecondsRemaining = 15.f;
};