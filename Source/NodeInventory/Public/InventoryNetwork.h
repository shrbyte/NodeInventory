

#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "InventoryNetwork.generated.h"

USTRUCT(BlueprintType)
struct NODEINVENTORY_API FInventoryNetworkNode : public FFastArraySerializerItem
{
public:

	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY()
	FGuid NodeGuid;

	UPROPERTY()
	TObjectPtr<class UItemDataAsset> ItemData = nullptr;

	UPROPERTY()
	FGuid ParentNodeGuid;

	UPROPERTY()
	int32 GridIndex = 0;

	UPROPERTY()
	int32 TopLeftX = 0;

	UPROPERTY()
	int32 TopLeftY = 0;

	UPROPERTY()
	bool bIsRotated = false;

};

USTRUCT(BlueprintType)
struct NODEINVENTORY_API FInventoryNetworkGraph : public FFastArraySerializer
{
public:

	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY()
	TArray<FInventoryNetworkNode> Items;

public:

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FInventoryNetworkNode, FInventoryNetworkGraph>(Items, DeltaParams, *this);
	};

};

template<>
struct NODEINVENTORY_API TStructOpsTypeTraits<FInventoryNetworkGraph> : public TStructOpsTypeTraitsBase2<FInventoryNetworkGraph>
{
	enum { WithNetDeltaSerializer = true };
};