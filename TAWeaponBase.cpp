#include "TAWeaponBase.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetMathLibrary.h"
#include "../Player/TACharacter.h"
#include "DrawDebugHelpers.h"

ATAWeaponBase::ATAWeaponBase()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    RootComponent = WeaponMesh;
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    CurrentAmmo = 0;
    bIsReloading = false;
    CurrentRecoilDeg = 0.f;
    LastFireTime = -1000.f;
    ReloadStartTime = 0.f;
}

void ATAWeaponBase::Init(ETAWeaponType Type)
{
    Stats = FTAWeaponStats::GetDefaultStats(Type);
    CurrentAmmo = Stats.MagazineSize;
}

void ATAWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATAWeaponBase, CurrentAmmo);
    DOREPLIFETIME(ATAWeaponBase, bIsReloading);
}

bool ATAWeaponBase::CanFire(float WorldTime) const
{
    if (bIsReloading || CurrentAmmo <= 0) return false;
    const float FireIntervalSec = 60.f / FMath::Max(1.f, Stats.FireRateRPM);
    return (WorldTime - LastFireTime) >= FireIntervalSec;
}

void ATAWeaponBase::TryFire(ATACharacter* InstigatorCharacter)
{
    if (!InstigatorCharacter) return;
    const float Now = GetWorld()->GetTimeSeconds();
    if (!CanFire(Now)) return;

    LastFireTime = Now;
    CurrentRecoilDeg = FMath::Min(Stats.MaxRecoilDeg, CurrentRecoilDeg + Stats.RecoilStepDeg);

    FVector TraceStart;
    FRotator AimRotation;
    InstigatorCharacter->GetActorEyesViewPoint(TraceStart, AimRotation);

    // apply spread + recoil cone in local space of the aim rotation
    const float TotalSpread = Stats.BaseSpreadDeg + CurrentRecoilDeg;
    const FVector SpreadDir = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(AimRotation.Vector(), TotalSpread);

    // local predicted ammo decrement for instant UI feedback; server is authoritative via replication
    if (InstigatorCharacter->IsLocallyControlled())
    {
        CurrentAmmo = FMath::Max(0, CurrentAmmo - 1);
    }

    ServerFire(TraceStart, SpreadDir);

    if (CurrentAmmo <= 0)
    {
        StartReload();
    }
}

bool ATAWeaponBase::ServerFire_Validate(FVector_NetQuantize TraceStart, FVector_NetQuantizeNormal TraceDir)
{
    // basic sanity check against speed/teleport hacking: direction must be normalized-ish
    return TraceDir.SizeSquared() > 0.5f && TraceDir.SizeSquared() < 1.5f;
}

void ATAWeaponBase::ServerFire_Implementation(FVector_NetQuantize TraceStart, FVector_NetQuantizeNormal TraceDir)
{
    if (CurrentAmmo <= 0 || bIsReloading) return; // server-side truth check, ignore desynced clients

    CurrentAmmo = FMath::Max(0, CurrentAmmo - 1);

    const FVector TraceEnd = TraceStart + (TraceDir * Stats.MaxRangeUnits);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());
    Params.bTraceComplex = true;

    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_GameTraceChannel1 /* Bullet channel */, Params);

    FVector ActualEnd = TraceEnd;
    if (bHit)
    {
        ActualEnd = Hit.Location;
        ATACharacter* HitCharacter = Cast<ATACharacter>(Hit.GetActor());
        if (HitCharacter)
        {
            const bool bIsHeadshot = Hit.BoneName == FName("head");
            const float Damage = Stats.BaseDamage * (bIsHeadshot ? Stats.HeadshotMultiplier : 1.f);
            HitCharacter->ServerApplyDamage(Damage, bIsHeadshot, Cast<ATACharacter>(GetOwner()));
        }
    }

    MulticastPlayFireFX(TraceStart, ActualEnd);

    if (CurrentAmmo <= 0)
    {
        StartReload();
    }
}

void ATAWeaponBase::MulticastPlayFireFX_Implementation(FVector TraceStart, FVector TraceEnd)
{
    // Cosmetic tracer line. Replace with a niagara tracer + muzzle flash particle in content.
#if !UE_BUILD_SHIPPING
    DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Yellow, false, 0.08f, 0, 1.2f);
#endif
}

void ATAWeaponBase::StartReload()
{
    if (bIsReloading || CurrentAmmo == Stats.MagazineSize) return;
    bIsReloading = true;
    ReloadStartTime = GetWorld()->GetTimeSeconds();
}

void ATAWeaponBase::OnRep_Ammo()
{
    // hook for UI refresh on replicated ammo change (HUD widget binds here)
}

void ATAWeaponBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // recoil recovery
    CurrentRecoilDeg = FMath::Max(0.f, CurrentRecoilDeg - DeltaTime * 6.f);

    if (bIsReloading && HasAuthority())
    {
        if (GetWorld()->GetTimeSeconds() - ReloadStartTime >= Stats.ReloadTimeSeconds)
        {
            CurrentAmmo = Stats.MagazineSize;
            bIsReloading = false;
        }
    }
}