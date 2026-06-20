#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TAWeaponTypes.h"
#include "TAAIController.generated.h"

class ATACharacter;

UENUM()
enum class EBotState : uint8 { Patrol, Engage, SeekCover, Retreat, Flank };

/**
 * ATAAIController
 * Note on Behavior Trees: a real BT graph is an editor-authored binary
 * asset (.uasset), so it can't be generated as source text here. This
 * controller implements the same decision logic (patrol / search / chase /
 * aim / shoot / use cover / retreat-if-low-health / flank) as an explicit
 * C++ state machine driven by a tick timer, which is functionally
 * equivalent and fully data-driven by EBotDifficulty. If you later want
 * the visual BT graph for designers to tune in-editor, this class's
 * Tick_Implementation logic maps 1:1 onto BT Selector/Sequence nodes and
 * an EQS query for cover scoring -- swap RunLogic() for RunBehaviorTree().
 */
UCLASS()
class TACTICALARENA_API ATAAIController : public AAIController
{
    GENERATED_BODY()

public:
    ATAAIController();

    UPROPERTY(BlueprintReadOnly)
    EBotDifficulty Difficulty;

    void InitDifficulty(EBotDifficulty InDifficulty);

protected:
    virtual void Tick(float DeltaTime) override;
    virtual void OnPossess(APawn* InPawn) override;

private:
    struct FDifficultyProfile
    {
        float ReactionTimeSec;
        float AccuracyPct;       // 0..1, chance any given shot is "on target" vs deliberately missed
        float MoveSpeedMult;
        bool  bUsesCover;
        bool  bStrafes;
        bool  bPredictsMovement;
        float DetectRangeUnits;
        float RetreatHealthPct;
    };

    FDifficultyProfile Profile;
    FDifficultyProfile BuildProfile(EBotDifficulty D) const;

    EBotState State = EBotState::Patrol;
    float TargetSeenAt = -1000.f;
    bool bCanSeeTarget = false;
    FVector PatrolTarget;
    float StrafeDir = 1.f;
    float NextDecisionTime = 0.f;

    ATACharacter* ControlledChar() const;
    ATACharacter* FindTarget() const; // returns nearest visible enemy character
    bool HasLineOfSight(const FVector& From, const FVector& To) const;
    FVector PredictLeadPosition(ATACharacter* Target, float ProjectileTime) const;
    FVector PickPatrolPoint() const;
    AActor* FindNearestCover(const FVector& From, const FVector& AwayFromThreat) const;

    void RunLogic(float DeltaTime);
};