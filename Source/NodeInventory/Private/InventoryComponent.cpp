


#include "InventoryComponent.h"
#include "InventoryNode.h"
#include "ItemDataAsset.h"
#include "InventoryNetwork.h"
#include "Net/UnrealNetwork.h"
#include "Logging/StructuredLog.h"


UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, NetworkGraph);
}

UInventoryNode* UInventoryComponent::CreateNewNode(UItemDataAsset* ItemData)
{
	if (not GetOwner()->HasAuthority())
	{
		return nullptr;
	}
	if (not ItemData)
	{
		return nullptr;
	}

	TObjectPtr<UInventoryNode> NewNode = NewObject<UInventoryNode>(this);
	NewNode->ItemData = ItemData;
	NewNode->NodeGuid = FGuid::NewGuid();

	NodeMap.Add(NewNode->NodeGuid, NewNode);
	RootNodes.Add(NewNode);

	RefreshNetworkGraph_Server();
	return NewNode;
}

bool UInventoryComponent::IsValidGridPlacement(UInventoryNode* ParentNode, int32 GridIndex, int32 X, int32 Y, int32 SizeX, int32 SizeY, UInventoryNode* IgnoreNode) const
{
	if (not ParentNode)
	{
		return false;
	}
	if (not ParentNode->ItemData->InternalGrids.IsValidIndex(GridIndex))
	{
		return false;
	}

	const FInventoryGridDefinition& Grid = ParentNode->ItemData->InternalGrids[GridIndex];
	if (X < 0 or Y < 0 or (X + SizeX) > Grid.Width or (Y + SizeY) > Grid.Height)
	{
		return false;
	}

	for (const auto& Pair : ParentNode->ChildNodes)
	{
		TObjectPtr<UInventoryNode> Child = Pair.Key;
		const FGridPlacementData& ChildPlacement = Pair.Value;

		if (Child == IgnoreNode)
		{
			continue;
		}
		if (ChildPlacement.GridIndex != GridIndex)
		{
			continue;
		}

		int32 ChildSizeX = ChildPlacement.bIsRotated ? Child->ItemData->SizeY : Child->ItemData->SizeX;
		int32 ChildSizeY = ChildPlacement.bIsRotated ? Child->ItemData->SizeX : Child->ItemData->SizeY;
		
		bool bOverlapX = (X < ChildPlacement.TopLeftX + ChildSizeX) and (X + SizeX > ChildPlacement.TopLeftX);
		bool bOverlapY = (Y < ChildPlacement.TopLeftY + ChildSizeY) and (Y + SizeY > ChildPlacement.TopLeftY);
	
		if (bOverlapX and bOverlapY)
		{
			return false;
		}
	}

	return true;
}

bool UInventoryComponent::AttachItemToGrid(UInventoryNode* ParentNode, int32 GridIndex, int32 TopLeftX, int32 TopLeftY, bool bRotate, UInventoryNode* ChildNode)
{
	if (not GetOwner()->HasAuthority())
	{
		return false;
	}
	if (not ParentNode)
	{
		return false;
	}
	if (not ChildNode)
	{
		return false;
	}
	if (ParentNode == ChildNode)
	{
		return false;
	}

	int32 TargetSizeX = bRotate ? ChildNode->ItemData->SizeY : ChildNode->ItemData->SizeX;
	int32 TargetSizeY = bRotate ? ChildNode->ItemData->SizeX : ChildNode->ItemData->SizeY;

	if (not IsValidGridPlacement(ParentNode, GridIndex, TopLeftX, TopLeftY, TargetSizeX, TargetSizeY, ChildNode))
	{
		return false;
	}

	DetachItem(ChildNode);
	RootNodes.Remove(ChildNode);

	FGridPlacementData NewPlacement;
	NewPlacement.GridIndex = GridIndex;
	NewPlacement.TopLeftX = TopLeftX;
	NewPlacement.TopLeftY = TopLeftY;
	NewPlacement.bIsRotated = bRotate;

	ChildNode->ParentNode = ParentNode;
	ChildNode->PlacementInParent = NewPlacement;
	ParentNode->ChildNodes.Add(ChildNode, NewPlacement);

	RefreshNetworkGraph_Server();
	return true;
}

void UInventoryComponent::DetachItem(UInventoryNode* ChildNode)
{
	if (not GetOwner()->HasAuthority())
	{
		return;
	}
	if (not ChildNode)
	{
		return;
	}
	if (not ChildNode->ParentNode)
	{
		return;
	}

	TObjectPtr<UInventoryNode> OldParent = ChildNode->ParentNode;
	OldParent->ChildNodes.Remove(ChildNode);

	ChildNode->ParentNode = nullptr;

	RefreshNetworkGraph_Server();
}

int32 UInventoryComponent::CountTotalItemsInSubtree(UInventoryNode* StartNode) const
{
	if (not StartNode)
	{
		return 0;
	}
	
	int32 Count = 1;
	for (const auto& Pair : StartNode->ChildNodes)
	{
		Count += CountTotalItemsInSubtree(Pair.Key);
	}
	return Count;
}

void UInventoryComponent::OnNetworkGraphUpdated()
{
	RefreshNetworkGraph_Client();
}

void UInventoryComponent::RefreshNetworkGraph_Server()
{
	if (not GetOwner()->HasAuthority())
	{
		return;
	}

	NetworkGraph.Items.Empty();

	for (const auto& Pair : NodeMap)
	{
		TObjectPtr<UInventoryNode> RuntimeNode = Pair.Value;
		if (not IsValid(RuntimeNode))
		{
			continue;
		}

		FInventoryNetworkNode NetworkNode;
		NetworkNode.NodeGuid = RuntimeNode->NodeGuid;
		NetworkNode.ItemData = RuntimeNode->ItemData;

		if (RuntimeNode->ParentNode)
		{
			NetworkNode.ParentNodeGuid  = RuntimeNode->ParentNode->NodeGuid;
			NetworkNode.GridIndex		= RuntimeNode->PlacementInParent.GridIndex;
			NetworkNode.TopLeftX		= RuntimeNode->PlacementInParent.TopLeftX;
			NetworkNode.TopLeftY		= RuntimeNode->PlacementInParent.TopLeftY;
			NetworkNode.bIsRotated		= RuntimeNode->PlacementInParent.bIsRotated;
		}
		else
		{
			NetworkNode.ParentNodeGuid  = FGuid();
			NetworkNode.GridIndex		= 0;
			NetworkNode.TopLeftX		= 0;
			NetworkNode.TopLeftY		= 0;
			NetworkNode.bIsRotated		= false;
		}

		NetworkGraph.Items.Add(NetworkNode);
	}

	NetworkGraph.MarkArrayDirty();
}

void UInventoryComponent::RefreshNetworkGraph_Client()
{
	if (GetOwner()->HasAuthority())
	{
		return;
	}

	TMap<FGuid, TObjectPtr<UInventoryNode>> NewNodeMap;
	RootNodes.Empty();

	for (const FInventoryNetworkNode& NetworkNode : NetworkGraph.Items)
	{
		TObjectPtr<UInventoryNode> TargetNode = nullptr;

		if (TObjectPtr<UInventoryNode>* ExistingNodePtr = NodeMap.Find(NetworkNode.NodeGuid))
		{
			TargetNode = *ExistingNodePtr;
		}
		else
		{
			TargetNode = NewObject<UInventoryNode>(this);
			TargetNode->ItemData = NetworkNode.ItemData;
			TargetNode->NodeGuid = NetworkNode.NodeGuid;
		}

		TargetNode->ParentNode = nullptr;
		TargetNode->ChildNodes.Empty();

		NewNodeMap.Add(NetworkNode.NodeGuid, TargetNode);
	}

	NodeMap = NewNodeMap;

	for (const FInventoryNetworkNode& NetworkNode : NetworkGraph.Items)
	{
		TObjectPtr<UInventoryNode>* CurrentNodePtr = NodeMap.Find(NetworkNode.NodeGuid);
		if (not CurrentNodePtr)
		{
			continue;
		}
		TObjectPtr<UInventoryNode> CurrentNode = *CurrentNodePtr;

		if (NetworkNode.ParentNodeGuid.IsValid())
		{
			if (TObjectPtr<UInventoryNode>* ParentNodePtr = NodeMap.Find(NetworkNode.ParentNodeGuid))
			{
				TObjectPtr<UInventoryNode> ParentNode = *ParentNodePtr;

				FGridPlacementData ReconstructedPlacement;
				ReconstructedPlacement.GridIndex = NetworkNode.GridIndex;
				ReconstructedPlacement.TopLeftX = NetworkNode.TopLeftX;
				ReconstructedPlacement.TopLeftY = NetworkNode.TopLeftY;
				ReconstructedPlacement.bIsRotated = NetworkNode.bIsRotated;


				CurrentNode->ParentNode = ParentNode;
				CurrentNode->PlacementInParent = ReconstructedPlacement;

				ParentNode->ChildNodes.Add(CurrentNode, ReconstructedPlacement);
			}
		}
		else
		{
			RootNodes.Add(CurrentNode);
		}
	}
}
