// shrbyte, 2026.


#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ItemDataAsset.h"
#include "InventoryNode.generated.h"

USTRUCT(BlueprintType)
struct FGridPlacementData
{
public:

	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly, Category = "Placement")
	int32 GridIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Placement")
	int32 TopLeftX = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Placement")
	int32 TopLeftY = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "Placement")
	bool bIsRotated = false;

};

UCLASS()
class NODEINVENTORY_API UInventoryNode : public UObject
{
	GENERATED_BODY()
	
public:	

	UInventoryNode();

public:

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<class UItemDataAsset> ItemData = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FGuid NodeGuid;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FIntPoint Location = {0,0};

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UInventoryNode> ParentNode = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FGridPlacementData PlacementInParent;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TMap<TObjectPtr<UInventoryNode>, FGridPlacementData> ChildNodes;

public:

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetCurrentSizeX() const { return PlacementInParent.bIsRotated ? ItemData->SizeY : ItemData->SizeX; };

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetCurrentSizeY() const { return PlacementInParent.bIsRotated ? ItemData->SizeX : ItemData->SizeY; };

};