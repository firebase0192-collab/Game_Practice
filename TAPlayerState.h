#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TAPlayerState.generated.h"

UCLASS()
class TACTICALARENA_API ATAPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    UPROPERTY(Replicated, BlueprintReadOnly) int32 Kills = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 Deaths = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 Assists = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 ShotsFired = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 ShotsHit = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 Headshots = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) float DamageDone = 0.f;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 RoundsWon = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) uint8 TeamId = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 PingMs = 0;

    float GetAccuracyPct() const { return ShotsFired > 0 ? (100.f * ShotsHit / ShotsFired) : 0.f; }

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};