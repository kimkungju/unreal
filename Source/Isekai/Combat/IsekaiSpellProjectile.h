#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IsekaiSpellProjectile.generated.h"

class UPointLightComponent;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * 직선으로 날아가 처음 닿은 대상에게 데미지를 주는 마법탄.
 * 지금은 엔진 기본 구체와 조명으로만 보인다. 이펙트는 블루프린트로 상속해서 붙인다.
 */
UCLASS(Blueprintable)
class ISEKAI_API AIsekaiSpellProjectile : public AActor
{
	GENERATED_BODY()

public:
	AIsekaiSpellProjectile();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell")
	float Damage = 25.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> Glow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> Movement;

private:
	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);
};
