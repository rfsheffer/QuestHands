// Copyright(c) 2020 Sheffer Online Services

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/BodyInstance.h"
#include "QuestHandsFunctions.h"

#include "QuestHands.h"

#include "QuestHandsComponent.generated.h"

//---------------------------------------------------------------------------------------------------------------------
/**
  * A component which keeps a record of the Oculus Quest hand shape which can be queried via the supplied functions.
  * It can also be configured to create or update supplied poseable hand mesh components.
*/
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = MotionController)
class QUESTHANDS_API UQuestHandsComponent : public USceneComponent
{
public:
    GENERATED_BODY()
    UQuestHandsComponent();

    virtual void BeginPlay() override;

    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

    // Is hand tracking currently enabled by the user? Returns false if the user has hand tracking disabled on their Oculus Dashboard.
    UFUNCTION(BlueprintPure, Category = "QuestHands")
    bool IsHandTrackingAvailable();

    // Save a dump of the hand state to a file on the device called HandTrackingDump.txt located in the Saved directory for the game.
    // This is useful for getting a hand state data dump from device and loading that in the PC editor to figure out problems with setting up visuals for tracking.
    UFUNCTION(BlueprintCallable, Category = "QuestHands")
    void SaveHandDataDump();

    // Try to locate a file called HandTrackingDump.txt in the Saved directory for the game and load that hand data and pose the hands.
    UFUNCTION(BlueprintCallable, Category = "QuestHands")
    bool LoadHandDataDump();

    // Create poseable mesh components and assign LeftHandMesh and RightHandMesh
    // If this is disabled
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestHands")
    bool CreateHandMeshComponents;

    // Enable to let this component update the skeletal meshes 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestHands")
    bool UpdateHandMeshComponents;

    // The left hand mesh component name to lookup if UpdateChildSkeletalMeshes is true
    // This should be a PoseableMeshComponent which is parented to this component
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestHands", meta = (EditCondition = "!CreateHandMeshComponents"))
    FString LeftHandMeshComponentName;

    // The right hand mesh component name to lookup if UpdateChildSkeletalMeshes is true
    // This should be a PoseableMeshComponent which is parented to this component
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestHands", meta = (EditCondition = "!CreateHandMeshComponents"))
    FString RightHandMeshComponentName;

    // The mesh to use for the left hand (Needs to conform to Oculus's example hand mesh bone structure OR you will get errors)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestHands", meta = (EditCondition = "CreateHandMeshComponents"))
    USkeletalMesh* LeftHandMesh;

    // The mesh to use for the right hand (Needs to conform to Oculus's example hand mesh bone structure OR you will get errors)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestHands", meta = (EditCondition = "CreateHandMeshComponents"))
    USkeletalMesh* RightHandMesh;

    // Should the hand mesh scale update based on what the OVR API thinks the users hand size is in relation to the standard hand mesh?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestHands", meta = (EditCondition = "UpdateHandMeshComponents"))
    bool UpdateHandScale;

    // Should the hand mesh have physics capsules created and updated for physical interactions with the world?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestHands", meta = (EditCondition = "UpdateHandMeshComponents"))
    bool UpdatePhysicsCapsules;

	// Physics scene information for the generated capsules
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "QuestHands", meta = (SkipUCSModifiedProperties, EditCondition = "UpdatePhysicsCapsules"))
	FBodyInstance CapsuleBodyData;

    // Used to correct the rotation from the Oculus hand bone rotations to conform to your mesh
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestHands")
    FRotator LeftHandBoneRotationOffset;

    // Used to correct the rotation from the Oculus hand bone rotations to conform to your mesh
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestHands")
    FRotator RightHandBoneRotationOffset;

    // The current left hand skeleton data
    UPROPERTY(BlueprintReadWrite, Category = "QuestHands")
    FQHandSkeleton LeftHandSkeletonData;

    // The current right hand skeleton data
    UPROPERTY(BlueprintReadWrite, Category = "QuestHands")
    FQHandSkeleton RightHandSkeletonData;

    // The current left hand tracking data
    UPROPERTY(BlueprintReadWrite, Category = "QuestHands")
    FQHandTrackingState LeftHandTrackingData;

    // The current right hand tracking data
    UPROPERTY(BlueprintReadWrite, Category = "QuestHands")
    FQHandTrackingState RightHandTrackingData;

protected:

    // Left hand poseable mesh component
    UPROPERTY(BlueprintReadOnly, Category = "QuestHands")
    class UPoseableMeshComponent* leftPoseable;

    // Right hand poseable mesh component
    UPROPERTY(BlueprintReadOnly, Category = "QuestHands")
    class UPoseableMeshComponent* rightPoseable;

    // Left hand bone transforms in world space
    UPROPERTY(BlueprintReadWrite, Category = "QuestHands")
    TArray<FTransform> leftHandBones;

    // Right hand bone transforms in world space
    UPROPERTY(BlueprintReadWrite, Category = "QuestHands")
    TArray<FTransform> rightHandBones;

    // Capsules on the left hand
    UPROPERTY(BlueprintReadOnly, Category = "QuestHands")
    TArray<class UCapsuleComponent*> leftCapsules;

    // Capsules on the right hand
    UPROPERTY(BlueprintReadOnly, Category = "QuestHands")
    TArray<class UCapsuleComponent*> rightCapsules;

private:

    void UpdateHandTrackingData();
    void SetupBoneTransforms(const FQHandSkeleton& skeleton, const FQHandTrackingState& trackingState, TArray<FTransform>& boneTransforms, bool leftHand);
    void UpdatePoseableWithBoneTransforms(class UPoseableMeshComponent* poseable, const TArray<FTransform>& boneTransforms);
    void DoUpdateHandMeshComponents();
    void SetupCapsuleComponents();
    void UpdateCapsules(TArray<class UCapsuleComponent*>& capsules, const FQHandSkeleton& skeleton);
};

// Special class for dumping hand tracking data out to a configuration file
UCLASS(config = Game)
class UQuestHandsDataDump : public UObject
{
public:
    GENERATED_BODY()

    // The current left hand skeleton data
    UPROPERTY(config)
    FQHandSkeleton LeftHandSkeletonData;

    // The current left hand tracking data
    UPROPERTY(config)
    FQHandTrackingState LeftHandTrackingData;

    UPROPERTY(config)
    FQHandSkeleton RightHandSkeletonData;

    UPROPERTY(config)
    FQHandTrackingState RightHandTrackingData;
};
