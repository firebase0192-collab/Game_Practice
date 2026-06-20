#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TAWeaponTypes.h"
#include "TAWeaponBase.generated.h"

class UStaticMeshComponent;
class ATACharacter;

/**
 * ATAWeaponBase
 * Server-authoritative weapon. Owning client predicts fire locally for feel,
 * server re-validates with its own line trace and is the only source of truth
 * for damage application (anti-cheat: never trust client hit results).
 */
UCLASS()
class TACTICALARENA_API ATAWeaponBase : public AActor
{
    GENERATED_BODY()

public:
    ATAWeaponBase();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FTAWeaponStats Stats;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* WeaponMesh;

    // --- replicated runtime state ---
    UPROPERTY(ReplicatedUsing = OnRep_Ammo, BlueprintReadOnly)
    int32 CurrentAmmo;

    UPROPERTY(Replicated, BlueprintReadOnly)
    bool bIsReloading;

    float CurrentRecoilDeg;
    float LastFireTime;
    float ReloadStartTime;

    void Init(ETAWeaponType Type);

    /** Called on owning client/server when the trigger is held. Handles fire-rate gating + recoil. */
    UFUNCTION(BlueprintCallable)
    void TryFire(ATACharacter* InstigatorCharacter);

    UFUNCTION(BlueprintCallable)
    void StartReload();

    bool CanFire(float WorldTime) const;

    UFUNCTION()
    void OnRep_Ammo();

protected:
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Server RPC: client requests a shot, server performs authoritative line trace + damage. */
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerFire(FVector_NetQuantize TraceStart, FVector_NetQuantizeNormal TraceDir);
    void ServerFire_Implementation(FVector_NetQuantize TraceStart, FVector_NetQuantizeNormal TraceDir);
    bool ServerFire_Validate(FVector_NetQuantize TraceStart, FVector_NetQuantizeNormal TraceDir);

    /** Multicast: tells all clients to play muzzle flash / tracer / sound. Cosmetic only. */
    UFUNCTION(NetMulticast, Unreliable)
    void MulticastPlayFireFX(FVector TraceStart, FVector TraceEnd);
    void MulticastPlayFireFX_Implementation(FVector TraceStart, FVector TraceEnd);
};