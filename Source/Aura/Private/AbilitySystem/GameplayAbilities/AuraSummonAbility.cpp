// Copyright Kerem Avcil.


#include "AbilitySystem/GameplayAbilities/AuraSummonAbility.h"

#include "Kismet/KismetSystemLibrary.h"


TArray<FVector> UAuraSummonAbility::GetSpawnLocations()
{
	const FVector Forward = GetAvatarActorFromActorInfo()->GetActorForwardVector();
	const FVector Location = GetAvatarActorFromActorInfo()->GetActorLocation();
	const float DeltaSpread = SpawnSpread / NumMinions;

	// Valid NavMesh Debug Option
	static TAutoConsoleVariable CVarDebugSummonAbility
	(
	TEXT("DebugSummonAbility"),
	0,
	TEXT("Show debug logs and shapes for summon ability\n")
	TEXT("  0: Off\n")
	TEXT("  1: On\n"),
	ECVF_Cheat
	);
	// End Valid NavMesh Debug Option
	
	const FVector LeftOfSpread = Forward.RotateAngleAxis(-SpawnSpread / 2.f, FVector::UpVector);
	TArray<FVector> SpawnLocations;
	for (int32 i = 0; i < NumMinions; i++)
	{
		const FVector Direction = LeftOfSpread.RotateAngleAxis(DeltaSpread * i, FVector::UpVector);
		FVector ChosenSpawnLocation = Location + Direction * FMath::FRandRange(MinSpawnDistance, MaxSpawnDistance);

		// Line Trace valid floor check before spawn
		FHitResult Hit;
		GetWorld()->LineTraceSingleByChannel(Hit, ChosenSpawnLocation + FVector(0.f, 0.f, 400.f), ChosenSpawnLocation - FVector(0.f, 0.f, 400.f), ECC_Visibility);
		if (Hit.bBlockingHit)
			{
				ChosenSpawnLocation = Hit.ImpactPoint;
			}
		// End Line Trace
		
		SpawnLocations.Add(ChosenSpawnLocation);

		// Debug Option Toggle
		if (CVarDebugSummonAbility.GetValueOnGameThread() > 0)
		{
			DrawDebugSphere(GetWorld(), ChosenSpawnLocation, 18.f, 12, FColor::Cyan, false, 3.f );
			UKismetSystemLibrary::DrawDebugArrow(GetAvatarActorFromActorInfo(), Location, Location + Direction * MaxSpawnDistance, 4.f, FLinearColor::Green, 3.f );
			DrawDebugSphere(GetWorld(), Location + Direction * MinSpawnDistance, 5.f, 12, FColor::Red, false, 3.f );
			DrawDebugSphere(GetWorld(), Location + Direction * MaxSpawnDistance, 5.f, 12, FColor::Red, false, 3.f );
		}
		// End Debug Option Toggle
	}

	return SpawnLocations;
}

TSubclassOf<APawn> UAuraSummonAbility::GetRandomMinionClass()
{
	int32 Selection = FMath::RandRange(0, MinionClasses.Num() -1);
	return MinionClasses[Selection];
}
