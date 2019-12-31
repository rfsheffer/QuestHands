// Copyright(c) 2020 Sheffer Online Services

#include "QuestHandsComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Components/PoseableMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

//---------------------------------------------------------------------------------------------------------------------
/**
*/
UQuestHandsComponent::UQuestHandsComponent() :
      CreateHandMeshComponents(true)
    , UpdateHandMeshComponents(true)
    , LeftHandMesh(nullptr)
    , RightHandMesh(nullptr)
    , UpdateHandScale(true)
    , UpdatePhysicsCapsules(true)
    , leftPoseable(nullptr)
    , rightPoseable(nullptr)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;

    FString defaultLeftHandPath = TEXT("/QuestHands/Meshes/hand_left");
    LeftHandMesh = ConstructorHelpersInternal::FindOrLoadObject<USkeletalMesh>(defaultLeftHandPath);

    FString defaultRightHandPath = TEXT("/QuestHands/Meshes/hand_right");
    RightHandMesh = ConstructorHelpersInternal::FindOrLoadObject<USkeletalMesh>(defaultRightHandPath);
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
void UQuestHandsComponent::BeginPlay()
{
    UpdateHandTrackingData();

    if(CreateHandMeshComponents)
    {
        if(LeftHandMesh)
        {
            leftPoseable = NewObject<UPoseableMeshComponent>(GetOwner(), UPoseableMeshComponent::StaticClass());
            if(leftPoseable)
            {
                leftPoseable->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
                leftPoseable->SetSkeletalMesh(LeftHandMesh);
                leftPoseable->RegisterComponent();
            }
            else
            {
                UE_LOG(LogQuestHands, Warning, TEXT("UQuestHandsComponent unable to create PoseableMeshComponent!"));
            }
        }
        else
        {
            UE_LOG(LogQuestHands, Warning, TEXT("UQuestHandsComponent has no valid LeftHandMesh assigned to create a poseable component for!"));
        }

        if(RightHandMesh)
        {
            rightPoseable = NewObject<UPoseableMeshComponent>(GetOwner(), UPoseableMeshComponent::StaticClass());
            if(rightPoseable)
            {
                rightPoseable->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
                rightPoseable->SetSkeletalMesh(RightHandMesh);
                rightPoseable->RegisterComponent();
            }
            else
            {
                UE_LOG(LogQuestHands, Warning, TEXT("UQuestHandsComponent unable to create PoseableMeshComponent!"));
            }
        }
        else
        {
            UE_LOG(LogQuestHands, Warning, TEXT("UQuestHandsComponent has no valid RightHandMesh assigned to create a poseable component for!"));
        }
    }
    else if(LeftHandMeshComponentName.Len() != 0 && RightHandMeshComponentName.Len() != 0)
    {
        TArray<USceneComponent*> Children;
        GetChildrenComponents(false, Children);
        for(USceneComponent* child : Children)
        {
            if(!child)
                continue;

            if(child->GetName().Contains(LeftHandMeshComponentName))
            {
                leftPoseable = Cast<UPoseableMeshComponent>(child);
            }
            else if(child->GetName().Contains(RightHandMeshComponentName))
            {
                rightPoseable = Cast<UPoseableMeshComponent>(child);
            }
        }

        if(!leftPoseable)
        {
            UE_LOG(LogQuestHands, Warning, TEXT("UQuestHandsComponent wasn't able to find left hand mesh component named: %s"), *LeftHandMeshComponentName);
        }
        if(!rightPoseable)
        {
            UE_LOG(LogQuestHands, Warning, TEXT("UQuestHandsComponent wasn't able to find right hand mesh component named: %s"), *RightHandMeshComponentName);
        }
    }

    Super::BeginPlay();
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
void UQuestHandsComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if(!UQuestHandsFunctions::IsHandTrackingEnabled())
    {
        return;
    }

    UpdateHandTrackingData();

    // Do we have a poseable mesh to update? Do so!
    if(UpdateHandMeshComponents)
    {
        if(leftPoseable)
        {
            FTransform rootPose(LeftHandTrackingData.RootPose.Orientation, LeftHandTrackingData.RootPose.Position, FVector::OneVector);
            leftPoseable->SetRelativeTransform(rootPose);
            UpdatePoseableWithBoneTransforms(leftPoseable, leftHandBones);
        }
        if(rightPoseable)
        {
            FTransform rootPose(RightHandTrackingData.RootPose.Orientation, RightHandTrackingData.RootPose.Position, FVector::OneVector);
            rightPoseable->SetRelativeTransform(rootPose);
            UpdatePoseableWithBoneTransforms(rightPoseable, rightHandBones);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
bool UQuestHandsComponent::IsHandTrackingAvailable()
{
    return UQuestHandsFunctions::IsHandTrackingEnabled();
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
void UQuestHandsComponent::UpdateHandTrackingData()
{
    checkf(GetWorld(), TEXT("UQuestHandsComponent : Invalid world!?"));

    float worldToMeters = 100.0f;
    AWorldSettings* worldSettings = GetWorld()->GetWorldSettings();
    if(worldSettings)
    {
        worldToMeters = worldSettings->WorldToMeters;
    }

    // Get the latest hand skeleton and tracking data
    UQuestHandsFunctions::GetHandSkeleton_Internal(EControllerHand::Left, LeftHandSkeletonData, worldToMeters);
    UQuestHandsFunctions::GetHandSkeleton_Internal(EControllerHand::Right, RightHandSkeletonData, worldToMeters);
    UQuestHandsFunctions::GetTrackingState_Internal(EControllerHand::Left, LeftHandTrackingData, worldToMeters);
    UQuestHandsFunctions::GetTrackingState_Internal(EControllerHand::Right, RightHandTrackingData, worldToMeters);

    // Update our cached skeleton bone transforms
    SetupBoneTransforms(LeftHandSkeletonData, LeftHandTrackingData, leftHandBones, true);
    SetupBoneTransforms(RightHandSkeletonData, RightHandTrackingData, rightHandBones, false);
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
void UQuestHandsComponent::SetupBoneTransforms(const FQHandSkeleton& skeleton, const FQHandTrackingState& trackingState, TArray<FTransform>& boneTransforms, bool leftHand)
{
    // Don't do anything if we aren't tracked, but make sure we at least have a base line if it hasn't been established yet.
    if(!trackingState.IsTracked && boneTransforms.Num() == skeleton.Bones.Num())
        return;

    // Ensure the array size is correct
    boneTransforms.SetNum(skeleton.Bones.Num());

    for(int32 boneIndex = 0; boneIndex < skeleton.Bones.Num(); ++boneIndex)
    {
        const FQHandBone& bone = skeleton.Bones[boneIndex];
        if((int32)bone.BoneId != boneIndex)
        {
            UE_LOG(LogQuestHands, Warning, TEXT("UQuestHandsComponent::SetupBoneTransforms - bone ID mismatch!"));
        }

        FQuat rotationIn = bone.Pose.Orientation;
        if(trackingState.IsTracked)
        {
            rotationIn = trackingState.BoneRotations[boneIndex];
            if(leftHand)
            {
                rotationIn *= LeftHandBoneRotationOffset.Quaternion();
            }
            else
            {
                rotationIn *= RightHandBoneRotationOffset.Quaternion();
            }
        }

        boneTransforms[boneIndex].SetComponents(rotationIn, 
                                                bone.Pose.Position, 
                                                FVector::OneVector);
        if(bone.ParentBoneIndex != -1)
        {
            // Apply our parents transform which converts us into world space
            boneTransforms[boneIndex] *= boneTransforms[bone.ParentBoneIndex];
        }
        else
        {
            // Apply our VR root transform AND our rootPose transform
            FTransform rootTransform(trackingState.RootPose.Orientation, 
                                     trackingState.RootPose.Position, 
                                     FVector::OneVector);
            rootTransform *= GetComponentTransform();
            boneTransforms[boneIndex] *= rootTransform;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
void UQuestHandsComponent::UpdatePoseableWithBoneTransforms(UPoseableMeshComponent* poseable, const TArray<FTransform>& boneTransforms)
{
    if(!poseable)
    {
        return;
    }

    for(int32 boneIndex = 0; boneIndex < boneTransforms.Num(); ++boneIndex)
    {
        FString boneName = UQuestHandsFunctions::GetHandBoneName((EQHandBones)boneIndex, poseable == leftPoseable);
        poseable->SetBoneTransformByName(FName(*boneName), boneTransforms[boneIndex], EBoneSpaces::WorldSpace);
    }
}
