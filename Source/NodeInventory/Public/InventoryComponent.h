// shrbyte, 2026.


#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryNetwork.h"
#include "InventoryComponent.generated.h"


UCLASS( ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent) )
class NODEINVENTORY_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UInventoryComponent();

public:
	/**
	* Array of node pointers.
	* Represents items which we hold directly.
	* Each node may store other nodes like a tree.
	* Nodes do not replicate directly. Node replication is implemented via NetworkGraph field which stores flatten version of graph inside of FFastArraySerializer.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<TObjectPtr<class UInventoryNode>> RootNodes;

	/**
	* Map used client-side in inventory graph reconstruction method.
	* Stores pairs of node guids and pointers for quick search.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TMap<FGuid, TObjectPtr<class UInventoryNode>> NodeMap;

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:

	/**
	* Creates new inventory node by provided item data asset.
	* 
	* Automatically sets the current component from which the call was made as the root.
	* 
	* Requires authority, should be called on server-side.
	* 
	* @param ItemData Pointer to data asset, defined in editor.
	* @return Pointer to newly created node.
	*/
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	class UInventoryNode* CreateNewNode(class UItemDataAsset* ItemData);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool IsValidGridPlacement(class UInventoryNode* ParentNode, int32 GridIndex, int32 X, int32 Y, int32 SizeX, int32 SizeY, class UInventoryNode* IgnoreNode) const;

	UFUNCTION(Category = "Inventory")
	bool AttachItemToGrid(class UInventoryNode* ParentNode, int32 GridIndex, int32 TopLeftX, int32 TopLeftY, bool bRotate, UInventoryNode* ChildNode);

	UFUNCTION(Category = "Inventory")
	void DetachItem(class UInventoryNode* ChildNode);

	/**
	* Method recursively counts total amount of items in provided tree node.
	* @param StartNode Pointer to a first node.
	* @return Int32 counter.
	*/
	UFUNCTION(BLueprintPure, Category = "Inventory")
	int32 CountTotalItemsInSubtree(class UInventoryNode* StartNode) const;

	/**
	* Replicated, packed via FFastArraySerializer, version of inventory graph.
	* Used only to synchronize clients state of inventory graph with server.
	*/
	UPROPERTY(ReplicatedUsing = OnNetworkGraphUpdated)
	FInventoryNetworkGraph NetworkGraph;

	/**
	* NetworkGraph replication callback. (OnRep_*)
	* Calls RefreshNetworkGraph_Client() to reproduce inventory graph locally out of NetworkGraph structure which stores packed TArray representation of inventory.
	*/
	UFUNCTION()
	void OnNetworkGraphUpdated();

private:
	/**
	* Server-side method to update NetworkGraph field.
	* Shrinks inventory graph into TArray using FFastArraySerializer for network optimization.
	*/
	void RefreshNetworkGraph_Server();
	
	/**
	* Client-side method called via OnNetworkGraphUpdated().
	* Unpacks NetworkGraph locally to reproduce server version of inventory graph.
	*/
	void RefreshNetworkGraph_Client();

public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Inventory")
	void K2_AttachItemToNode(class UInventoryNode* ParentNode, int32 GridIndex, int32 TopLeftX, int32 TopLeftY, bool bRotate, class UInventoryNode* ChildNode);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Inventory")
	void K2_DetachItem(class UInventoryNode* ChildNode);

protected:

	UFUNCTION(Server, Unreliable, Category = "Inventory|RPC")
	void Server_AttachItemToNode(FGuid ParentNodeGuid, int32 GridIndex, int32 TopLeftX, int32 TopLeftY, bool bRotate, FGuid ChildNodeGuid);

	UFUNCTION(Server, Unreliable, Category = "Inventory|RPC")
	void Server_DetachItem(FGuid ChildNodeGuid);

};