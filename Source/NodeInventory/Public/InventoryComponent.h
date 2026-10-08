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

protected:

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
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	class UInventoryNode* CreateNewNode(class UItemDataAsset* ItemData);

	/**
	* Simulates node(item) placement at specified grid at provided coordinates.
	* 
	* @param ParentNode InventoryNode(item) in which we are trying to place another InventoryNode(item).
	* @param GridIndex integer, index of internal grid of provided ParentNode.
	* @param X integer, top left corner X location where node(item) will be placed in 2D grid.
	* @param Y integer, top left corner Y location where node(item) will be placed in 2D grid.
	* @param SizeX integer, horizontal size of InventoryNode(item) being placed.
	* @param SizeY integer, vertical size of InventoryNode(item) being placed.
	* @param IgnoreNode InventoryNode(item) which we are placing. Should be specified if we are trying to emplace item in the same grid where its already in.
	* @return bool, true if placement location valid - false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool IsValidGridPlacement(class UInventoryNode* ParentNode, int32 GridIndex, int32 X, int32 Y, int32 SizeX, int32 SizeY, class UInventoryNode* IgnoreNode) const;

	/**
	* Attaches provided inventory node to new parent node if possible.
	* 
	* Should be called server-side only.
	* 
	* @param ParentNode InventoryNode(item), target where item should be placed.
	* @param GridIndex integer, index of internal grid of provided ParentNode.
	* @param TopLeftX integer, top left corner X location where node(item) will be placed in 2D grid.
	* @param TopLeftY integer, top left corner Y location where node(item) will be placed in 2D grid.
	* @param bRotate bool, should item be rotated when placed.
	* @param ChildNode InventoryNode(item) itself.
	* @return bool, true if success - false otherwise.
	*/
	UFUNCTION(Category = "Inventory")
	bool AttachItemToGrid(class UInventoryNode* ParentNode, int32 GridIndex, int32 TopLeftX, int32 TopLeftY, bool bRotate, UInventoryNode* ChildNode);

	/**
	* Detaches provided node from its parent node and owning inventory component root if needed.
	* 
	* Should be called server-side only.
	* 
	* @param ChildNode InventoryNode(item) which should be detached.
	*/
	UFUNCTION(Category = "Inventory")
	void DetachItem(class UInventoryNode* ChildNode);

	/**
	* Method recursively counts total amount of items in provided tree node.
	* 
	* @param StartNode Pointer to a first node.
	* @return Int32 counter.
	*/
	UFUNCTION(BLueprintPure, Category = "Inventory")
	int32 CountTotalItemsInSubtree(class UInventoryNode* StartNode) const;

public:

	/**
	* Blueprint callable client-side wrapper over server RPC.
	* 
	* Finds corresponding FGuids of provided nodes and calls server RPC.
	* 
	* Because we do not replicate inventory nodes directly we cant just call server RPCs by providing them with object pointers.
	* But we are synchronizing nodes GUIDs, so we use them instead.
	* 
	* @param ParentNode InventoryNode(item), target where item should be placed.
	* @param GridIndex integer, index of internal grid of provided ParentNode.
	* @param TopLeftX integer, top left corner X location where node(item) will be placed in 2D grid.
	* @param TopLeftY integer, top left corner Y location where node(item) will be placed in 2D grid.
	* @param bRotate bool, should item be rotated when placed.
	* @param ChildNode InventoryNode(item) itself.
	*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Inventory", meta = (DisplayName = "⚡ Attach Item to Node (Server)"))
	void K2_AttachItemToNode(class UInventoryNode* ParentNode, int32 GridIndex, int32 TopLeftX, int32 TopLeftY, bool bRotate, class UInventoryNode* ChildNode);

	/**
	* Blueprint callable client-side wrapper over server RPC.
	*
	* Finds corresponding FGuid of provided node and calls server RPC.
	* 
	* @param ChildNode InventoryNode(item) which should be detached.
	*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Inventory", meta = (DisplayName = "⚡ Detach Item (Server)"))
	void K2_DetachItem(class UInventoryNode* ChildNode);

protected:

	/**
	* Server RPC.
	* 
	* May be theoretically be BlueprintCallable, but as it requires nodes guids, which may be inconvenient, its cpp only.
	* 
	* @param ParentNodeGuid InventoryNode(item) guid, target where item should be placed.
	* @param GridIndex integer, index of internal grid of provided ParentNode.
	* @param TopLeftX integer, top left corner X location where node(item) will be placed in 2D grid.
	* @param TopLeftY integer, top left corner Y location where node(item) will be placed in 2D grid.
	* @param bRotate bool, should item be rotated when placed.
	* @param ChildNodeGuid InventoryNode(item) itself guid.
	*/
	UFUNCTION(Server, Unreliable, Category = "Inventory|RPC")
	void Server_AttachItemToNode(FGuid ParentNodeGuid, int32 GridIndex, int32 TopLeftX, int32 TopLeftY, bool bRotate, FGuid ChildNodeGuid);

	/**
	* Server RPC.
	*
	* May be theoretically be BlueprintCallable, but as it requires nodes guid, which may be inconvenient, its cpp only.
	*
	* @param ChildNodeGuid InventoryNode(item) guid which should be detached.
	*/
	UFUNCTION(Server, Unreliable, Category = "Inventory|RPC")
	void Server_DetachItem(FGuid ChildNodeGuid);

};