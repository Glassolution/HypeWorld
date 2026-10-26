// Corpo que aparece na tela, para os efeitos que copiam a malha do personagem (aura e chakra nos pés).

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

/** O Mesh do personagem ou, se ele estiver escondido (só anima, como no retarget do Indra), o primeiro Skeletal Mesh
 *  visível preso nele. */
inline USkeletalMeshComponent* GetNinjaVisibleBody(const ACharacter* Character)
{
	USkeletalMeshComponent* Body = Character ? Character->GetMesh() : nullptr;
	if (Body && !Body->IsVisible())
	{
		TArray<USceneComponent*> Children;
		Body->GetChildrenComponents(false, Children);
		for (USceneComponent* Child : Children)
		{
			USkeletalMeshComponent* Skel = Cast<USkeletalMeshComponent>(Child);
			if (Skel && Skel->IsVisible() && Skel->GetSkeletalMeshAsset())
			{
				return Skel;
			}
		}
	}
	return Body;
}
