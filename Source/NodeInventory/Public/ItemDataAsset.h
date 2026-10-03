

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FInventoryGridDefinition
{
public:
	
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grid")
	FName GridName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grid")
	int32 Width = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grid")
	int32 Height = 1;

};

UCLASS()
class NODEINVENTORY_API UItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:	
	
	UItemDataAsset();

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText Name;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 SizeX = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 SizeY = 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TArray<FInventoryGridDefinition> InternalGrids;

public:

	UFUNCTION(BlueprintCallable, Category = "Item")
	bool CanStoreItems() const { return true; };

	UFUNCTION(BlueprintCallable, Category = "Item")
	int32 GetCapacity() const { return 0; };

};