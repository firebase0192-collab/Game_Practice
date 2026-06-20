#include "TAGameState.h"
#include "Net/UnrealNetwork.h"

void ATAGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATAGameState, Phase);
    DOREPLIFETIME(ATAGameState, CountdownSeconds);
    DOREPLIFETIME(ATAGameState, ScoreTeamA);
    DOREPLIFETIME(ATAGameState, ScoreTeamB);
    DOREPLIFETIME(ATAGameState, CurrentRound);
    DOREPLIFETIME(ATAGameState, RoundsToWin);
    DOREPLIFETIME(ATAGameState, bSuddenDeath);
    DOREPLIFETIME(ATAGameState, RoundTimeRemaining);
    DOREPLIFETIME(ATAGameState, MatchMode);
}