#include "TAPlayerState.h"
#include "Net/UnrealNetwork.h"

void ATAPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATAPlayerState, Kills);
    DOREPLIFETIME(ATAPlayerState, Deaths);
    DOREPLIFETIME(ATAPlayerState, Assists);
    DOREPLIFETIME(ATAPlayerState, ShotsFired);
    DOREPLIFETIME(ATAPlayerState, ShotsHit);
    DOREPLIFETIME(ATAPlayerState, Headshots);
    DOREPLIFETIME(ATAPlayerState, DamageDone);
    DOREPLIFETIME(ATAPlayerState, RoundsWon);
    DOREPLIFETIME(ATAPlayerState, TeamId);
    DOREPLIFETIME(ATAPlayerState, PingMs);
}