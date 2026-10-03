

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryNetwork.h"
#include "InventoryComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class NODEINVENTORY_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UInventoryComponent();

public:

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<TObjectPtr<class UInventoryNode>> RootNodes;

	UPROPERTY()
	TMap<FGuid, TObjectPtr<class UInventoryNode>> NodeMap;

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:

	UFUNCTION(BLueprintCallable, Category = "Inventory")
	class UInventoryNode* CreateNewNode(class UItemDataAsset* ItemData);

	UFUNCTION(BLueprintCallable, Category = "Inventory")
	bool IsValidGridPlacement(class UInventoryNode* ParentNode, int32 GridIndex, int32 X, int32 Y, int32 SizeX, int32 SizeY, class UInventoryNode* IgnoreNode) const;

	UFUNCTION(BLueprintCallable, Category = "Inventory")
	bool AttachItemToGrid(class UInventoryNode* ParentNode, int32 GridIndex, int32 TopLeftX, int32 TopLeftY, bool bRotate, UInventoryNode* ChildNode);

	UFUNCTION(BLueprintCallable, Category = "Inventory")
	void DetachItem(class UInventoryNode* ChildNode);

	UFUNCTION(BLueprintPure, Category = "Inventory")
	int32 CountTotalItemsInSubtree(class UInventoryNode* StartNode) const;

	UPROPERTY(ReplicatedUsing = OnNetworkGraphUpdated)
	FInventoryNetworkGraph NetworkGraph;

	UFUNCTION()
	void OnNetworkGraphUpdated();

private:

	void RefreshNetworkGraph_Server();
	
	void RefreshNetworkGraph_Client();
};