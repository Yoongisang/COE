// Fill out your copyright notice in the Description page of Project Settings.


#include "ExplorationEnemy.h"
#include "ExplorationPlayer.h"
#include "Engine/OverlapResult.h"
/*
void AExplorationEnemy::DetectPlayer()
{
	if (this != nullptr)
	{
		FVector Center = this->GetActorLocation();
		float DetectDistance = 500.f;
		TArray<FOverlapResult> OverlapResults;
		FCollisionQueryParams QueryParams(NAME_Name, false, this);

		bool Result = GetWorld()->OverlapMultiByChannel
		(
			OverlapResults,
			Center,
			FQuat::Identity,
			ECollisionChannel::ECC_GameTraceChannel1,
			FCollisionShape::MakeSphere(DetectDistance),
			QueryParams
		);

		if (Result)
		{
			for (auto& OverlapResult : OverlapResults)
			{
				// �÷��̾ Ž���Ǹ� ����׽��Ǿ� �ʷ�
				auto Player = Cast<AExplorationPlayer>(OverlapResult.GetActor());
				if (Player)
				{
					DrawDebugSphere(GetWorld(), Center, DetectDistance, 10, FColor::Green, false, 0.5f);
					return;
				}

			}

		}
		//Ž������ ������ ������
		DrawDebugSphere(GetWorld(), Center, DetectDistance, 10, FColor::Red, false, 0.5f);
	}
}
*/

