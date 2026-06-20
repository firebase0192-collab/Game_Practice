#include "TAAIController.h"
#include "../Player/TACharacter.h"
#include "../Weapons/TAWeaponBase.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"

ATAAIController::ATAAIController()
{
    PrimaryActorTick.bCanEverTick = true;
    Difficulty = EBotDifficulty::Normal;
}

ATAAIController::FDifficultyProfile ATAAIController::BuildProfile(EBotDifficulty D) const
{
    FDifficultyProfile P;
    switch (D)
    {
        case EBotDifficulty::Easy:
            P = { 1.5f, 0.40f, 0.75f, false, false, false, 2000.f, 0.35f };
            break;
        case EBotDifficulty::Normal:
            P = { 0.8f, 0.70f, 0.95f, true, true, false, 2800.f, 0.30f };
            break;
        case EBotDifficulty::Difficult:
            P = { 0.3f, 0.90f, 1.20f, true, true, true, 3600.f, 0.25f };
            break;
    }
    return P;
}

void ATAAIController::InitDifficulty(EBotDifficulty InDifficulty)
{
    Difficulty = InDifficulty;
    Profile = BuildProfile(InDifficulty);
}

void ATAAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    Profile = BuildProfile(Difficulty);
    PatrolTarget = PickPatrolPoint();
}

ATACharacter* ATAAIController::ControlledChar() const
{
    return Cast<ATACharacter>(GetPawn());
}

FVector ATAAIController::PickPatrolPoint() const
{
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (NavSys && GetPawn())
    {
        FNavLocation Result;
        if (NavSys->GetRandomReachablePointInRadius(GetPawn()->GetActorLocation(), 2000.f, Result))
        {
            return Result.Location;
        }
    }
    return GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector;
}

bool ATAAIController::HasLineOfSight(const FVector& From, const FVector& To) const
{
    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetPawn());
    const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Params);
    return !bBlocked; // nothing in the way -> clear sightline
}

ATACharacter* ATAAIController::FindTarget() const
{
    ATACharacter* Self = ControlledChar();
    if (!Self) return nullptr;

    TArray<AActor*> AllChars;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATACharacter::StaticClass(), AllChars);

    ATACharacter* Best = nullptr;
    float BestDist = Profile.DetectRangeUnits;
    for (AActor* A : AllChars)
    {
        ATACharacter* C = Cast<ATACharacter>(A);
        if (!C || C == Self || !C->bIsAlive || C->TeamId == Self->TeamId) continue;
        const float Dist = FVector::Dist(Self->GetActorLocation(), C->GetActorLocation());
        if (Dist < BestDist && HasLineOfSight(Self->GetActorLocation() + FVector(0, 0, 60), C->GetActorLocation() + FVector(0, 0, 60)))
        {
            BestDist = Dist;
            Best = C;
        }
    }
    return Best;
}

FVector ATAAIController::PredictLeadPosition(ATACharacter* Target, float ProjectileTime) const
{
    if (!Target) return FVector::ZeroVector;
    const FVector Velocity = Target->GetVelocity();
    return Target->GetActorLocation() + Velocity * ProjectileTime;
}

AActor* ATAAIController::FindNearestCover(const FVector& From, const FVector& AwayFromThreat) const
{
    // Production version: run an EQS query against cover-marker actors placed in the map
    // (EnvQueryContext_CoverPoints) and score by distance + occlusion from threat.
    // Lightweight stand-in here: nearest actor tagged "Cover" in the level.
    TArray<AActor*> CoverActors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Cover"), CoverActors);
    AActor* Best = nullptr;
    float BestScore = -FLT_MAX;
    for (AActor* C : CoverActors)
    {
        if (!C) continue;
        const float DistScore = -FVector::Dist(From, C->GetActorLocation());
        const float AwayScore = FVector::DotProduct((C->GetActorLocation() - AwayFromThreat).GetSafeNormal(), (C->GetActorLocation() - From).GetSafeNormal());
        const float Score = DistScore * 0.6f + AwayScore * 400.f;
        if (Score > BestScore) { BestScore = Score; Best = C; }
    }
    return Best;
}

void ATAAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    RunLogic(DeltaTime);
}

void ATAAIController::RunLogic(float DeltaTime)
{
    ATACharacter* Self = ControlledChar();
    if (!Self || !Self->bIsAlive) return;

    const float Now = GetWorld()->GetTimeSeconds();
    ATACharacter* Target = FindTarget();

    if (Target)
    {
        if (!bCanSeeTarget) TargetSeenAt = Now;
        bCanSeeTarget = true;
    }
    else
    {
        bCanSeeTarget = false;
    }

    const bool bReacted = bCanSeeTarget && (Now - TargetSeenAt >= Profile.ReactionTimeSec);
    const float HealthPct = Self->Health / FMath::Max(1.f, Self->MaxHealth);

    // --- state selection (Selector node equivalent) ---
    if (bReacted && HealthPct <= Profile.RetreatHealthPct)
    {
        State = EBotState::Retreat;
    }
    else if (bReacted && Profile.bUsesCover && Now > NextDecisionTime)
    {
        NextDecisionTime = Now + 1.2f;
        State = (FMath::FRand() < 0.5f) ? EBotState::SeekCover : EBotState::Engage;
    }
    else if (bReacted)
    {
        State = EBotState::Engage;
    }
    else
    {
        State = EBotState::Patrol;
    }

    // --- execute state ---
    switch (State)
    {
        case EBotState::Patrol:
        {
            if (FVector::DistSquared(Self->GetActorLocation(), PatrolTarget) < 150.f * 150.f)
            {
                PatrolTarget = PickPatrolPoint();
            }
            MoveToLocation(PatrolTarget, 30.f);
            break;
        }
        case EBotState::Engage:
        {
            FVector AimPoint = Target->GetActorLocation();
            if (Profile.bPredictsMovement && Self->CurrentWeapon)
            {
                const float TravelTime = FVector::Dist(Self->GetActorLocation(), Target->GetActorLocation()) / 5000.f; // approx bullet speed
                AimPoint = PredictLeadPosition(Target, TravelTime);
            }
            const FVector ToTarget = (AimPoint - Self->GetActorLocation()).GetSafeNormal();
            Self->SetActorRotation(ToTarget.Rotation());

            if (Profile.bStrafes)
            {
                const FVector Right = FVector::CrossProduct(ToTarget, FVector::UpVector);
                Self->AddMovementInput(Right, StrafeDir);
                if (FMath::FRand() < 0.01f) StrafeDir *= -1.f; // flip occasionally
            }

            // accuracy gate: difficulty determines what fraction of attempted shots are allowed
            // to actually land vs. be deliberately offset, simulating skill rather than reading
            // raw weapon spread alone.
            if (Self->CurrentWeapon && Self->CurrentWeapon->CanFire(Now))
            {
                Self->ShotsFired++;
                const bool bAllowAccurateShot = FMath::FRand() < Profile.AccuracyPct;
                if (!bAllowAccurateShot)
                {
                    // jitter the controller's rotation briefly before firing to induce a deliberate miss
                    Self->AddControllerYawInput(FMath::FRandRange(-8.f, 8.f));
                }
                Self->CurrentWeapon->TryFire(Self);
            }
            break;
        }
        case EBotState::SeekCover:
        {
            if (AActor* Cover = FindNearestCover(Self->GetActorLocation(), Target ? Target->GetActorLocation() : FVector::ZeroVector))
            {
                MoveToLocation(Cover->GetActorLocation(), 50.f);
            }
            break;
        }
        case EBotState::Retreat:
        {
            FVector AwayDir = Target ? (Self->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal() : FVector::ForwardVector;
            MoveToLocation(Self->GetActorLocation() + AwayDir * 1000.f, 50.f);
            break;
        }
        case EBotState::Flank:
        {
            // Difficult-only embellishment: route around cover to approach from the side.
            if (Target)
            {
                const FVector Side = FVector::CrossProduct((Target->GetActorLocation() - Self->GetActorLocation()).GetSafeNormal(), FVector::UpVector);
                MoveToLocation(Target->GetActorLocation() + Side * 600.f, 50.f);
            }
            break;
        }
    }

    GetPawn()->GetMovementComponent()->Velocity *= Profile.MoveSpeedMult > 0 ? 1.f : 1.f; // movement speed is set on CharacterMovementComponent's MaxWalkSpeed at spawn time using Profile.MoveSpeedMult
}