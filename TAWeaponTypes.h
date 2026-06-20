#pragma once

#include "CoreMinimal.h"
#include "TAWeaponTypes.generated.h"

UENUM(BlueprintType)
enum class ETAWeaponType : uint8
{
    Pistol  UMETA(DisplayName = "Pistol"),
    SMG     UMETA(DisplayName = "SMG"),
    Rifle   UMETA(DisplayName = "Rifle"),
    Sniper  UMETA(DisplayName = "Sniper")
};

UENUM(BlueprintType)
enum class EBotDifficulty : uint8
{
    Easy    UMETA(DisplayName = "Easy"),
    Normal  UMETA(DisplayName = "Normal"),
    Difficult UMETA(DisplayName = "Difficult")
};

UENUM(BlueprintType)
enum class ETAMatchMode : uint8
{
    Duel1v1      UMETA(DisplayName = "1v1 Duel"),
    BotMatch     UMETA(DisplayName = "Bot Match"),
    OneVsAll     UMETA(DisplayName = "One vs All"),
    MultiplayerLAN UMETA(DisplayName = "Multiplayer LAN"),
    MultiplayerOnline UMETA(DisplayName = "Multiplayer Online")
};

// Static, data-driven weapon stats. One row per ETAWeaponType.
USTRUCT(BlueprintType)
struct FTAWeaponStats
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) ETAWeaponType WeaponType = ETAWeaponType::Pistol;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FName DisplayName = "Pistol";
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float BaseDamage = 25.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) int32 MagazineSize = 12;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float FireRateRPM = 300.f;     // rounds per minute
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float ReloadTimeSeconds = 1.5f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float BaseSpreadDeg = 1.0f;     // cone half-angle at rest
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float RecoilStepDeg = 0.4f;     // added spread per shot while firing
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float MaxRecoilDeg = 6.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float HeadshotMultiplier = 2.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float MaxRangeUnits = 15000.f; // ~150m in UE units (cm)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) int32 PriceCredits = 0;

    static FTAWeaponStats GetDefaultStats(ETAWeaponType Type)
    {
        FTAWeaponStats S;
        S.WeaponType = Type;
        switch (Type)
        {
            case ETAWeaponType::Pistol:
                S.DisplayName = "Pistol"; S.BaseDamage = 25.f; S.MagazineSize = 12; S.FireRateRPM = 260.f;
                S.ReloadTimeSeconds = 1.2f; S.BaseSpreadDeg = 0.8f; S.RecoilStepDeg = 0.3f; S.MaxRecoilDeg = 4.f;
                S.HeadshotMultiplier = 2.0f; S.PriceCredits = 0; break;
            case ETAWeaponType::SMG:
                S.DisplayName = "SMG"; S.BaseDamage = 20.f; S.MagazineSize = 35; S.FireRateRPM = 720.f;
                S.ReloadTimeSeconds = 1.8f; S.BaseSpreadDeg = 1.4f; S.RecoilStepDeg = 0.25f; S.MaxRecoilDeg = 5.5f;
                S.HeadshotMultiplier = 2.0f; S.PriceCredits = 1200; break;
            case ETAWeaponType::Rifle:
                S.DisplayName = "Rifle"; S.BaseDamage = 35.f; S.MagazineSize = 30; S.FireRateRPM = 480.f;
                S.ReloadTimeSeconds = 2.2f; S.BaseSpreadDeg = 1.0f; S.RecoilStepDeg = 0.3f; S.MaxRecoilDeg = 6.5f;
                S.HeadshotMultiplier = 2.2f; S.PriceCredits = 2900; break;
            case ETAWeaponType::Sniper:
                S.DisplayName = "Sniper"; S.BaseDamage = 80.f; S.MagazineSize = 5; S.FireRateRPM = 60.f;
                S.ReloadTimeSeconds = 2.6f; S.BaseSpreadDeg = 0.15f; S.RecoilStepDeg = 1.2f; S.MaxRecoilDeg = 9.f;
                S.HeadshotMultiplier = 1.5f; S.PriceCredits = 4500; break;
        }
        return S;
    }
};