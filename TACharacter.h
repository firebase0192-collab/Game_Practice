#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TAWeaponTypes.h"
#include "TACharacter.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class ATAWeaponBase;
class UInputAction;
class UInputMappingContext;

/**
 * ATACharacter
 * First-person tactical character. Uses a capsule body + a visible
 * "body block" (cube) and "head sphere" static mesh built from engine
 * basic shapes by default, so the enemy is visibly rendered the moment
 * you compile -- no imported skeletal mesh/animation asset required.
 * Swap BodyMesh/HeadMesh for a USkeletalMeshComponent + animation blueprint
 * once real character art is ready; the gameplay code does not change.
 */
UCLASS()
class TACTICALARENA_API ATACharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ATACharacter();

    // ---- visible representation (engine primitives, visible out of the box) ----
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
    UStaticMeshComponent* BodyMesh;     // torso block, attached to capsule

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
    UStaticMeshComponent* HeadMesh;     // head sphere -- tagged "head" bone-equivalent for headshot trace

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
    UCameraComponent* FirstPersonCamera;

    // ---- team / identity ----
    UPROPERTY(Replicated, BlueprintReadOnly)
    FName PlayerDisplayName;

    UPROPERTY(ReplicatedUsing = OnRep_Team, BlueprintReadOnly)
    uint8 TeamId; // 0 = Me/Attacker side, 1 = Enemy side -- drives body tint color

    // ---- vitals (server authoritative) ----
    UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly)
    float Health;

    UPROPERTY(Replicated, BlueprintReadOnly)
    float MaxHealth;

    UPROPERTY(ReplicatedUsing = OnRep_Armor, BlueprintReadOnly)
    float Armor;

    UPROPERTY(Replicated, BlueprintReadOnly)
    bool bIsAlive;

    // ---- equipped weapon ----
    UPROPERTY(Replicated, BlueprintReadOnly)
    ATAWeaponBase* CurrentWeapon;

    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<ATAWeaponBase> WeaponClass;

    // ---- stats (kept on PlayerState normally; mirrored here for convenience in bot-only matches) ----
    int32 Kills = 0;
    int32 Deaths = 0;
    int32 ShotsFired = 0;
    int32 ShotsHit = 0;
    int32 Headshots = 0;
    float DamageDone = 0.f;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void BeginPlay() override;

    void EquipWeapon(ETAWeaponType Type);

    /** Server-side damage entry point, called by the weapon's authoritative hit trace. */
    void ServerApplyDamage(float Damage, bool bWasHeadshot, ATACharacter* Instigator);

    void ServerHeal();
    void ServerResetForRound(FVector SpawnLocation);

    UFUNCTION()
    void OnRep_Health();
    UFUNCTION()
    void OnRep_Armor();
    UFUNCTION()
    void OnRep_Team();

    // ---- input handlers (Enhanced Input) ----
    void Input_Move(const FInputActionValue& Value);
    void Input_Look(const FInputActionValue& Value);
    void Input_FireStarted();
    void Input_Reload();

protected:
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputMappingContext* DefaultMappingContext;
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction* MoveAction;
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction* LookAction;
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction* FireAction;
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction* ReloadAction;

private:
    void ApplyTeamTint();
    void Die(ATACharacter* Killer);
};