using UnrealBuildTool;

public class TacticalArena : ModuleRules
{
    public TacticalArena(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore",
            "EnhancedInput",      // Enhanced Input System
            "UMG", "Slate", "SlateCore",   // UMG UI
            "AIModule", "NavigationSystem", // AI + NavMesh
            "GameplayTasks",
            "OnlineSubsystem", "OnlineSubsystemUtils", // Sessions / matchmaking
            "Networking", "Sockets"
        });

        PublicIncludePaths.AddRange(new string[] {
            "TacticalArena", "TacticalArena/Core", "TacticalArena/Player",
            "TacticalArena/Weapons", "TacticalArena/AI",
            "TacticalArena/Lobby", "TacticalArena/Networking", "TacticalArena/UI"
        });

        bEnableExceptions = true;
    }
}