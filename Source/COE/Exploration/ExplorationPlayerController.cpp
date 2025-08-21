// Fill out your copyright notice in the Description page of Project Settings.


#include "ExplorationPlayerController.h"
#include "ExplorationPlayer.h"

void AExplorationPlayerController::BeginPlay()
{
	Super::BeginPlay();
	ExplorationChar = Cast<AExplorationPlayer>(GetCharacter());
}

void AExplorationPlayerController::DoMouseLeftClick()
{
	if (!ExplorationChar->bIsAiming)
	{
		ExplorationChar->UseExplorationFullHeal();
	}
	else
	{
		Super::DoMouseLeftClick();
	}
	


}
