#pragma once

UENUM(BlueprintType)
enum class ETeam : uint8
{
	ET_AttackingTeam UMETA(DisplayName = "AttackingTeam"),
	ET_DefendingTeam UMETA(DisplayName = "DefendingTeam"),
	ET_NoTeam UMETA(DisplayName = "NoTeam"),
	
	ET_MAX UMETA(DisplayName = "DefaultMAX"),
};