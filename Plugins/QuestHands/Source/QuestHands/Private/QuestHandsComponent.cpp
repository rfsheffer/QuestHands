// Copyright(c) 2020 Sheffer Online Services

#include "QuestHandsComponent.h"
#include "GameFramework/WorldSettings.h"
#include "UObject/ConstructorHelpers.h"

#include "Components/PoseableMeshComponent.h"
#include "Components/CapsuleComponent.h"

DECLARE_STATS_GROUP(TEXT("Quest Hands"), STATGROUP_QuestHands, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("RenderTick"), STAT_QuestHands_RenderTick, STATGROUP_QuestHands);
DECLARE_CYCLE_STAT(TEXT("PhysicsTick"), STAT_QuestHands_PhysicsTick, STATGROUP_QuestHands);

//---------------------------------------------------------------------------------------------------------------------
/**
*/
void FQuestHandsPhysicsTickFunction::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, 
                                                 const FGraphEventRef& MyCompletionGraphEvent)
{
    FActorComponentTickFunction::ExecuteTickHelper(Target, /*bTickInEditor=*/ false, DeltaTime, TickType, [this, DeltaTime](float DilatedTime)
    {
        Target->PhysicsTickComponent(*this, DeltaTime);
    });
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
FString FQuestHandsPhysicsTickFunction::DiagnosticMessage()
{
    return TEXT("FQuestHandsPhysicsTickFunction");
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
FName FQuestHandsPhysicsTickFunction::DiagnosticContext(bool bDetailed)
{
    return FName(TEXT("FQuestHandsPhysicsTick"));
}

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
    , LeftHandBoneRotationOffset(0.0f, 90.0f, 90.0f)
    , RightHandBoneRotationOffset(0.0f, 90.0f, 90.0f)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.bTickEvenWhenPaused = true;
    PrimaryComponentTick.bHighPriority = 1;
    PrimaryComponentTick.TickInterval = 0;

    QuestHandsPhysicsTick.TickGroup = TG_PrePhysics;
    QuestHandsPhysicsTick.bCanEverTick = true;
    QuestHandsPhysicsTick.bStartWithTickEnabled = true;
    QuestHandsPhysicsTick.bTickEvenWhenPaused = true;
    QuestHandsPhysicsTick.bHighPriority = 1;
    QuestHandsPhysicsTick.TickInterval = 0;

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
    UpdateHandTrackingData(EQHandUpdateStep::UpdateStep_Render);
    UpdateHandTrackingData(EQHandUpdateStep::UpdateStep_Physics);

    if(CreateHandMeshComponents)
    {
        if(LeftHandMesh)
        {
            UPoseableMeshComponent* leftPoseable = NewObject<UPoseableMeshComponent>(GetOwner(), UPoseableMeshComponent::StaticClass());
            if(leftPoseable)
            {
                leftPoseable->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
                leftPoseable->SetSkeletalMesh(LeftHandMesh);
                leftPoseable->RegisterComponent();
                leftPoseables.Add(leftPoseable);
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
            UPoseableMeshComponent* rightPoseable = NewObject<UPoseableMeshComponent>(GetOwner(), UPoseableMeshComponent::StaticClass());
            if(rightPoseable)
            {
                rightPoseable->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
                rightPoseable->SetSkeletalMesh(RightHandMesh);
                rightPoseable->RegisterComponent();
                rightPoseables.Add(rightPoseable);
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
                UPoseableMeshComponent* leftPoseable = Cast<UPoseableMeshComponent>(child);
                if(leftPoseable)
                {
                    leftPoseables.Add(leftPoseable);
                }
            }
            else if(child->GetName().Contains(RightHandMeshComponentName))
            {
                UPoseableMeshComponent* rightPoseable = Cast<UPoseableMeshComponent>(child);
                if(rightPoseable)
                {
                    rightPoseables.Add(rightPoseable);
                }
            }
        }

        if(leftPoseables.Num() == 0)
        {
            UE_LOG(LogQuestHands, Warning, TEXT("UQuestHandsComponent wasn't able to find left hand mesh component named: %s"), *LeftHandMeshComponentName);
        }
        if(rightPoseables.Num() == 0)
        {
            UE_LOG(LogQuestHands, Warning, TEXT("UQuestHandsComponent wasn't able to find right hand mesh component named: %s"), *RightHandMeshComponentName);
        }
    }

    if(UpdateHandMeshComponents && UpdatePhysicsCapsules)
    {
        SetupCapsuleComponents();
    }

    // Register the Hands Physics Tick Function
    if(!QuestHandsPhysicsTick.IsTickFunctionRegistered())
    {
        if(SetupActorComponentTickFunction(&QuestHandsPhysicsTick))
        {
            QuestHandsPhysicsTick.Target = this;
        }
    }

    Super::BeginPlay();
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
void UQuestHandsComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if(QuestHandsPhysicsTick.IsTickFunctionRegistered())
    {
        QuestHandsPhysicsTick.UnRegisterTickFunction();
    }

    Super::EndPlay(EndPlayReason);
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
void UQuestHandsComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    SCOPE_CYCLE_COUNTER(STAT_QuestHands_RenderTick);

    if(!UQuestHandsFunctions::IsHandTrackingEnabled())
    {
        return;
    }

    UpdateHandTrackingData(EQHandUpdateStep::UpdateStep_Render);

    if(OnPreHandMeshesUpdate.IsBound())
    {
        OnPreHandMeshesUpdate.Broadcast(DeltaTime);
    }

    // Do we have a poseable mesh to update? Do so!
    if(UpdateHandMeshComponents)
    {
        DoUpdateHandMeshComponents(true, false);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
void UQuestHandsComponent::PhysicsTickComponent(FQuestHandsPhysicsTickFunction& tickFunc, float DeltaTime)
{
    SCOPE_CYCLE_COUNTER(STAT_QuestHands_PhysicsTick);

    if(!UQuestHandsFunctions::IsHandTrackingEnabled())
    {
        return;
    }

    UpdateHandTrackingData(EQHandUpdateStep::UpdateStep_Physics);

    if(OnPreCapsulesUpdate.IsBound())
    {
        OnPreCapsulesUpdate.Broadcast(DeltaTime);
    }

    // Do we have a poseable mesh to update? Do so!
    if(UpdateHandMeshComponents)
    {
        DoUpdateHandMeshComponents(false, true);
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
void UQuestHandsComponent::SaveHandDataDump()
{
    UQuestHandsDataDump *tmpTracking = GetMutableDefault<UQuestHandsDataDump>();
    tmpTracking->LeftHandSkeletonData = LeftHandSkeletonData;
    tmpTracking->LeftHandTrackingData = LeftHandTrackingData;
    tmpTracking->RightHandSkeletonData = RightHandSkeletonData;
    tmpTracking->RightHandTrackingData = RightHandTrackingData;
    tmpTracking->SaveConfig(CPF_Config, *(FPaths::ProjectSavedDir() / TEXT("HandTrackingDump.txt")));
    UE_LOG(LogQuestHands, Log, TEXT("Saving Hand Data Dump to %s"), *(FPaths::ProjectSavedDir() / TEXT("HandTrackingDump.txt")));
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
bool UQuestHandsComponent::LoadHandDataDump()
{
    FString dataPath = FPaths::ProjectSavedDir() / TEXT("HandTrackingDump.txt");
    if(FPaths::FileExists(dataPath))
    {
        UQuestHandsDataDump *tmpTracking = GetMutableDefault<UQuestHandsDataDump>();
        tmpTracking->LoadConfig(NULL, *dataPath);

        LeftHandSkeletonData = tmpTracking->LeftHandSkeletonData;
        LeftHandTrackingData = tmpTracking->LeftHandTrackingData;
        RightHandSkeletonData = tmpTracking->RightHandSkeletonData;
        RightHandTrackingData = tmpTracking->RightHandTrackingData;

        SetupCapsuleComponents();

        SetupBoneTransforms(LeftHandSkeletonData, LeftHandTrackingData, leftHandBones, true);
        SetupBoneTransforms(RightHandSkeletonData, RightHandTrackingData, rightHandBones, false);

        DoUpdateHandMeshComponents(true, true);
        return true;
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
void UQuestHandsComponent::UpdateHandTrackingData(const EQHandUpdateStep Step)
{
    checkf(GetWorld(), TEXT("UQuestHandsComponent : Invalid world!?"));

    if(!UQuestHandsFunctions::IsHandTrackingEnabled())
    {
        return;
    }

    float worldToMeters = 100.0f;
    AWorldSettings* worldSettings = GetWorld()->GetWorldSettings();
    if(worldSettings)
    {
        worldToMeters = worldSettings->WorldToMeters;
    }

    // Get the latest hand skeleton and tracking data
    UQuestHandsFunctions::GetHandSkeleton_Internal(EControllerHand::Left, LeftHandSkeletonData, worldToMeters);
    UQuestHandsFunctions::GetHandSkeleton_Internal(EControllerHand::Right, RightHandSkeletonData, worldToMeters);
    UQuestHandsFunctions::GetTrackingState_Internal(EControllerHand::Left, Step, LeftHandTrackingData, worldToMeters);
    UQuestHandsFunctions::GetTrackingState_Internal(EControllerHand::Right, Step, RightHandTrackingData, worldToMeters);

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
        }

        boneTransforms[boneIndex].SetComponents(rotationIn, 
                                                bone.Pose.Position, 
                                                FVector::OneVector);
        if(bone.ParentBoneIndex != -1)
        {
            // Apply our parents transform which converts us into world space and applies the root scale, etc
            boneTransforms[boneIndex] *= boneTransforms[bone.ParentBoneIndex];
        }
        else
        {
            // Apply our VR root transform AND our rootPose transform
            FTransform rootTransform(trackingState.RootPose.Orientation, 
                                     trackingState.RootPose.Position, 
                                     FVector(UpdateHandScale ? trackingState.HandScale : 1.0f));
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

    bool isLeftHand = leftPoseables.Contains(poseable);
    FQuat rotationOffset = isLeftHand ? LeftHandBoneRotationOffset.Quaternion() : RightHandBoneRotationOffset.Quaternion();

    for(int32 boneIndex = 0; boneIndex < boneTransforms.Num(); ++boneIndex)
    {
        FString boneName = UQuestHandsFunctions::GetHandBoneName((EQHandBones)boneIndex, isLeftHand);

        FTransform transSet = boneTransforms[boneIndex];
        transSet.SetRotation(transSet.GetRotation() * rotationOffset);

        poseable->SetBoneTransformByName(FName(*boneName), transSet, EBoneSpaces::WorldSpace);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
void UQuestHandsComponent::DoUpdateHandMeshComponents(bool visualComponents, bool physicsComponents)
{
    if(visualComponents)
    {
        if(leftPoseables.Num())
        {
            for(UPoseableMeshComponent* leftPoseable : leftPoseables)
            {
                if(!leftPoseable)
                    continue;

                if(UpdateHandScale)
                {
                    leftPoseable->SetRelativeScale3D(FVector(LeftHandTrackingData.HandScale));
                }

                FTransform rootPose(LeftHandTrackingData.RootPose.Orientation, LeftHandTrackingData.RootPose.Position, FVector::OneVector);
                leftPoseable->SetRelativeTransform(rootPose);
                UpdatePoseableWithBoneTransforms(leftPoseable, leftHandBones);
            }
        }
        if(rightPoseables.Num())
        {
            for(UPoseableMeshComponent* rightPoseable : rightPoseables)
            {
                if(!rightPoseable)
                    continue;

                if(UpdateHandScale)
                {
                    rightPoseable->SetRelativeScale3D(FVector(RightHandTrackingData.HandScale));
                }

                FTransform rootPose(RightHandTrackingData.RootPose.Orientation, RightHandTrackingData.RootPose.Position, FVector::OneVector);
                rightPoseable->SetRelativeTransform(rootPose);
                UpdatePoseableWithBoneTransforms(rightPoseable, rightHandBones);
            }
        }
    }

    if(physicsComponents)
    {
        if(UpdatePhysicsCapsules)
        {
            if(leftCapsules.Num() == 0 && LeftHandSkeletonData.BoneCapsules.Num() != 0 &&
               rightCapsules.Num() == 0 && RightHandSkeletonData.BoneCapsules.Num() != 0)
            {
                SetupCapsuleComponents();
            }
            UpdateCapsules(leftHandBones, leftCapsules, LeftHandSkeletonData);
            UpdateCapsules(rightHandBones, rightCapsules, RightHandSkeletonData);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
void UQuestHandsComponent::SetupCapsuleComponents()
{
    if(leftCapsules.Num() != LeftHandSkeletonData.BoneCapsules.Num())
    {
        leftCapsules.SetNum(LeftHandSkeletonData.BoneCapsules.Num());
        for(int32 capsuleIndex = 0; capsuleIndex < leftCapsules.Num(); ++capsuleIndex)
        {
            UCapsuleComponent* capsuleComp = NewObject<UCapsuleComponent>(GetOwner(), UCapsuleComponent::StaticClass());
            if(capsuleComp)
            {
                leftCapsules[capsuleIndex] = capsuleComp;
                capsuleComp->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
                capsuleComp->BodyInstance = CapsuleBodyData;
                capsuleComp->ShapeColor = FColor::Blue;
                capsuleComp->RegisterComponent();
            }
            else
            {
                UE_LOG(LogQuestHands, Warning, TEXT("UQuestHandsComponent unable to create UCapsuleComponent!"));
            }
        }
    }

    if(rightCapsules.Num() != RightHandSkeletonData.BoneCapsules.Num())
    {
        rightCapsules.SetNum(RightHandSkeletonData.BoneCapsules.Num());
        for(int32 capsuleIndex = 0; capsuleIndex < rightCapsules.Num(); ++capsuleIndex)
        {
            UCapsuleComponent* capsuleComp = NewObject<UCapsuleComponent>(GetOwner(), UCapsuleComponent::StaticClass());
            if(capsuleComp)
            {
                rightCapsules[capsuleIndex] = capsuleComp;
                capsuleComp->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
                capsuleComp->BodyInstance = CapsuleBodyData;
                capsuleComp->ShapeColor = FColor::Blue;
                capsuleComp->RegisterComponent();
            }
            else
            {
                UE_LOG(LogQuestHands, Warning, TEXT("UQuestHandsComponent unable to create UCapsuleComponent!"));
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
void UQuestHandsComponent::UpdateCapsules(const TArray<FTransform>& bones, TArray<UCapsuleComponent*>& capsules, const FQHandSkeleton& skeleton)
{
    float worldToMeters = 100.0f;
    AWorldSettings* worldSettings = GetWorld()->GetWorldSettings();
    if(worldSettings)
    {
        worldToMeters = worldSettings->WorldToMeters;
    }

    if(skeleton.BoneCapsules.Num() != capsules.Num())
    {
        UE_LOG(LogQuestHands, Warning, TEXT("UQuestHandsComponent::UpdateCapsules with incorrect number of capsule components! Wanted %d, got %d!"), 
               skeleton.BoneCapsules.Num(), capsules.Num());
        return;
    }

    TSet<int32> usedindices;
    for(int32 boneIndex = 0; boneIndex < bones.Num(); ++boneIndex)
    {
        int32 parentBoneIndex = skeleton.Bones[boneIndex].ParentBoneIndex;
        if(parentBoneIndex == -1 || usedindices.Contains(parentBoneIndex) || !capsules[parentBoneIndex])
            continue;

        FVector bonePos = bones[boneIndex].GetLocation();
        FVector parentBonePos = bones[parentBoneIndex].GetLocation();

        FVector capsuleLocation = parentBonePos + (bonePos - parentBonePos) / 2.8f;
        FQuat capsuleRotation = bones[parentBoneIndex].GetRotation() * FRotator(0.0f, 0.0f, 90.0f).Quaternion();

        float halfHeight = ((skeleton.BoneCapsules[parentBoneIndex].PointB - skeleton.BoneCapsules[parentBoneIndex].PointA).Size() * worldToMeters) / 2.0f;
        float radius = skeleton.BoneCapsules[parentBoneIndex].Radius * worldToMeters * 0.8f;

        capsules[parentBoneIndex]->SetCapsuleSize(radius, halfHeight);
        capsules[parentBoneIndex]->SetWorldLocationAndRotation(capsuleLocation, capsuleRotation);

        usedindices.Add(parentBoneIndex);
    }
}
