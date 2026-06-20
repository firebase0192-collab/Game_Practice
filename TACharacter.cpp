#include "TACharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "Net/UnrealNetwork.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "../Weapons/TAWeaponBase.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"

ATACharacter::ATACharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);

    GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);

    // --- Body: a cube static mesh scaled to a humanoid silhouette.
    // Uses the engine's built-in basic shape so it renders without any imported asset.
    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(GetCapsuleComponent());
    BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -10.f));
    BodyMesh->SetRelativeScale3D(FVector(0.7f, 0.45f, 1.55f));
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // capsule already handles blocking collision

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMeshAsset.Succeeded())
    {
        BodyMesh->SetStaticMesh(CubeMeshAsset.Object);
    }

    // --- Head: a sphere static mesh, explicitly tagged so the weapon's
    // line trace can identify headshots (Hit.BoneName check uses this name).
    HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("head"));
    HeadMesh->SetupAttachment(BodyMesh);
    HeadMesh->SetRelativeLocation(FVector(0.f, 0.f, 95.f));
    HeadMesh->SetRelativeScale3D(FVector(0.45f));
    HeadMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    HeadMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereMeshAsset.Succeeded())
    {
        HeadMesh->SetStaticMesh(SphereMeshAsset.Object);
    }

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(HeadMesh);
    FirstPersonCamera->SetRelativeLocation(FVector(10.f, 0.f, 0.f));
    FirstPersonCamera->bUsePawnControlRotation = true;

    MaxHealth = 100.f;
    Health = 100.f;
    Armor = 50.f;
    bIsAlive = true;
    TeamId = 0;
}

void ATACharacter::BeginPlay()
{
    Super::BeginPlay();
    ApplyTeamTint();

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            if (DefaultMappingContext) Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }
}

void ATACharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATACharacter, Health);
    DOREPLIFETIME(ATACharacter, MaxHealth);
    DOREPLIFETIME(ATACharacter, Armor);
    DOREPLIFETIME(ATACharacter, bIsAlive);
    DOREPLIFETIME(ATACharacter, CurrentWeapon);
    DOREPLIFETIME(ATACharacter, PlayerDisplayName);
    DOREPLIFETIME(ATACharacter, TeamId);
}

void ATACharacter::ApplyTeamTint()
{
    // Team 0 (you / attacker side) renders teal, Team 1 (enemy / bots) renders red.
    // Uses a dynamic material instance over the default engine material so no
    // content-browser material asset is required to see the distinction.
    if (!BodyMesh || !BodyMesh->GetMaterial(0)) return;
    UMaterialInstanceDynamic* DynMat = BodyMesh->CreateAndSetMaterialInstanceDynamic(0);
    if (DynMat)
    {
        const FLinearColor Tint = (TeamId == 0) ? FLinearColor(0.24f, 0.94f, 0.77f) : FLinearColor(1.f, 0.27f, 0.33f);
        DynMat->SetVectorParameterValue(FName("Color"), Tint); // works with engine's default "BasicShapeMaterial"
    }
}

void ATACharacter::OnRep_Team() { ApplyTeamTint(); }

void ATACharacter::EquipWeapon(ETAWeaponType Type)
{
    if (!HasAuthority() || !WeaponClass) return;

    if (CurrentWeapon) CurrentWeapon->Destroy();

    FActorSpawnParameters Params;
    Params.Owner = this;
    CurrentWeapon = GetWorld()->SpawnActor<ATAWeaponBase>(WeaponClass, GetActorTransform(), Params);
    if (CurrentWeapon)
    {
        CurrentWeapon->Init(Type);
        CurrentWeapon->AttachToComponent(BodyMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    }
}

void ATACharacter::ServerApplyDamage(float Damage, bool bWasHeadshot, ATACharacter* Instigator)
{
    if (!HasAuthority() || !bIsAlive) return;

    float RemainingDamage = Damage;
    if (Armor > 0.f)
    {
        const float Absorbed = FMath::Min(Armor, RemainingDamage * 0.33f);
        Armor -= Absorbed;
        RemainingDamage -= Absorbed;
        OnRep_Armor(); // server doesn't get its own OnRep call automatically pre-replication; trigger cosmetic update locally too
    }

    Health = FMath::Max(0.f, Health - RemainingDamage);

    if (Instigator)
    {
        Instigator->DamageDone += Damage;
        if (bWasHeadshot) Instigator->Headshots++;
    }

    if (Health <= 0.f)
    {
        Die(Instigator);
    }
}

void ATACharacter::Die(ATACharacter* Killer)
{
    bIsAlive = false;
    Deaths++;
    if (Killer) Killer->Kills++;

    // Ragdoll / death-cam hook: disable input + collision, GameMode handles kill-feed broadcast + round check.
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetCharacterMovement()->DisableMovement();
}

void ATACharacter::ServerHeal()
{
    if (!HasAuthority()) return;
    Health = MaxHealth;
    Armor = 50.f;
    bIsAlive = true;
}

void ATACharacter::ServerResetForRound(FVector SpawnLocation)
{
    if (!HasAuthority()) return;
    SetActorLocation(SpawnLocation);
    Health = MaxHealth;
    Armor = 50.f;
    bIsAlive = true;
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    if (CurrentWeapon) CurrentWeapon->Init(CurrentWeapon->Stats.WeaponType);
}

void ATACharacter::OnRep_Health() { /* hook: HUD health bar refresh, damage flash */ }
void ATACharacter::OnRep_Armor()  { /* hook: HUD armor bar refresh */ }

// ---------------- Input (Enhanced Input System) ----------------

void ATACharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (MoveAction)   EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATACharacter::Input_Move);
        if (LookAction)   EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATACharacter::Input_Look);
        if (FireAction)   EIC->BindAction(FireAction, ETriggerEvent::Triggered, this, &ATACharacter::Input_FireStarted);
        if (ReloadAction) EIC->BindAction(ReloadAction, ETriggerEvent::Started, this, &ATACharacter::Input_Reload);
    }
}

void ATACharacter::Input_Move(const FInputActionValue& Value)
{
    if (!bIsAlive) return;
    const FVector2D MoveVec = Value.Get<FVector2D>();
    if (!Controller) return;
    AddMovementInput(GetActorForwardVector(), MoveVec.Y);
    AddMovementInput(GetActorRightVector(), MoveVec.X);
}

void ATACharacter::Input_Look(const FInputActionValue& Value)
{
    const FVector2D LookVec = Value.Get<FVector2D>();
    AddControllerYawInput(LookVec.X);
    AddControllerPitchInput(LookVec.Y);
}

void ATACharacter::Input_FireStarted()
{
    if (!bIsAlive || !CurrentWeapon) return;
    ShotsFired++;
    CurrentWeapon->TryFire(this);
}

void ATACharacter::Input_Reload()
{
    if (CurrentWeapon) CurrentWeapon->StartReload();
}