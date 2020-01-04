// Copyright(c) 2020 Sheffer Online Services

#include "QuestHandsFunctions.h"
#include "IOculusInputModule.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"

#if OCULUS_INPUT_SUPPORTED_PLATFORMS

#include "IOculusHMDModule.h"
#include "OculusHMD.h"

#if PLATFORM_SUPPORTS_PRAGMA_PACK
#pragma pack (push,8)
#endif

#include <OVR_Plugin.h>

#if PLATFORM_SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif

// Make sure our enums are still ok
static_assert(ovrpBoneId_Max == (int32)EQHandBones::Hand_PinkyTip + 1, "EQHandBones needs to be aligned with the Oculus enum ovrpBoneId");
static_assert(ovrpHandFinger_Max == (int32)EQHandFinger::HandFinger_Pinky + 1, "EQHandFinger needs to be aligned with the Oculus enum ovrpHandFinger");

#endif // OCULUS_INPUT_SUPPORTED_PLATFORMS

namespace QuestHands
{
    bool IsOVRAvailable()
    {
    #if OCULUS_INPUT_SUPPORTED_PLATFORMS
        return IOculusHMDModule::IsAvailable() && ovrp_GetInitialized();
    #else
        return false;
    #endif
    }

    /// Configurable VrApi properties.
    /// This was copied from the SDK file \ovr_sdk_mobile_1.29.0\VrApi\Include\VrApi_Types.h
    typedef enum ovrProperty_
    {
        VRAPI_FOVEATION_LEVEL = 15,		//< Used by apps that want to control swapchain foveation levels.
        VRAPI_REORIENT_HMD_ON_CONTROLLER_RECENTER = 17,		//< Used to determine if a controller recenter should also reorient the headset.
        VRAPI_LATCH_BACK_BUTTON_ENTIRE_FRAME = 18,		//< Used to determine if the 'short press' back button should lasts an entire frame.
        VRAPI_BLOCK_REMOTE_BUTTONS_WHEN_NOT_EMULATING_HMT = 19,//< Used to not send the remote back button java events to the apps.
        VRAPI_EAT_NATIVE_GAMEPAD_EVENTS = 20,				//< Used to tell the runtime not to eat gamepad events.  If this is false on a native app, the app must be listening for the events.
        VRAPI_ACTIVE_INPUT_DEVICE_ID = 24,		//< Used by apps to query which input device is most 'active' or primary, a -1 means no active input device
        VRAPI_DEVICE_EMULATION_MODE = 29,		//< Used by apps to determine if they are running in an emulation mode. Is a ovrDeviceEmulationMode value

        VRAPI_DYNAMIC_FOVEATION_ENABLED = 30,  //< Used by apps to enable / disable dynamic foveation adjustments.
    } ovrProperty;
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
bool UQuestHandsFunctions::IsHandTrackingEnabled()
{
    if(!QuestHands::IsOVRAvailable() || !GEngine->XRSystem.IsValid())
        return false;

#if OCULUS_INPUT_SUPPORTED_PLATFORMS
    ovrpBool bResult = true;
    return OVRP_SUCCESS(ovrp_GetHandTrackingEnabled(&bResult)) && bResult;
#endif
    return false;
}


#if OCULUS_INPUT_SUPPORTED_PLATFORMS
namespace QuestHands
{
    //---------------------------------------------------------------------------------------------------------------------
    /**
     * Transform to scene
    */
    FVector TransformVectorToUnrealScene(const FVector& InVec, OculusHMD::FSettings* Settings, float worldToMeters)
    {
        return (InVec - Settings->BaseOffset) * worldToMeters;
    }

    FQuat TransformQuatToUnrealScene(const FQuat& InQuat, OculusHMD::FSettings* Settings, float worldToMeters)
    {
        FQuat quatOut = Settings->BaseOrientation.Inverse() * InQuat;
        quatOut.Normalize();
        return quatOut;
    }
}
#endif

//---------------------------------------------------------------------------------------------------------------------
/**
*/
bool UQuestHandsFunctions::GetTrackingState(const UObject* WorldContextObject, const EControllerHand Hand, FQHandTrackingState& stateOut)
{
    if(!QuestHands::IsOVRAvailable())
    {
        UE_LOG(LogQuestHands, Error, TEXT("Calling QuestHandsFunctions::GetTrackingState when OVR is not available!"));
        return false;
    }

    UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
    if(!World)
    {
        UE_LOG(LogQuestHands, Error, TEXT("Calling QuestHandsFunctions::GetTrackingState when World is not available!"));
        return false;
    }

#if OCULUS_INPUT_SUPPORTED_PLATFORMS
    float worldToMeters = 100.0f;
    AWorldSettings *worldSettings = World->GetWorldSettings();
    if(worldSettings)
    {
        worldToMeters = worldSettings->WorldToMeters;
    }
    return GetTrackingState_Internal(Hand, stateOut, worldToMeters);
#else
    return false;
#endif
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
bool UQuestHandsFunctions::GetTrackingState_Internal(const EControllerHand Hand, FQHandTrackingState& stateOut, const float worldToMeters)
{
#if OCULUS_INPUT_SUPPORTED_PLATFORMS
    if(!GEngine->XRSystem.IsValid())
    {
        UE_LOG(LogQuestHands, Error, TEXT("Calling QuestHandsFunctions::GetTrackingState_Internal when XRSystem is not available!"));
        return false;
    }

    OculusHMD::FOculusHMD* OculusHMD = static_cast<OculusHMD::FOculusHMD*>(GEngine->XRSystem->GetHMDDevice());
    if(!OculusHMD)
    {
        UE_LOG(LogQuestHands, Error, TEXT("Calling QuestHandsFunctions::GetTrackingState_Internal when HMD is not available!"));
        return false;
    }

    OculusHMD::FSettings* Settings;
    if(IsInGameThread())
    {
        Settings = OculusHMD->GetSettings();
    }
    else
    {
        Settings = OculusHMD->GetSettings_RenderThread();
    }
    if(!Settings)
    {
        UE_LOG(LogQuestHands, Error, TEXT("Calling QuestHandsFunctions::GetTrackingState_Internal when HMD Settings is not available!"));
        return false;
    }

    ovrpHand hand = Hand == EControllerHand::Left ? ovrpHand_Left : ovrpHand_Right;
    ovrpHandState handState;
    if(OVRP_SUCCESS(ovrp_GetHandState(ovrpStep_Render, hand, &handState)))
    {
        // Status Out
        stateOut.IsTracked = (handState.Status & ovrpHandStatus_HandTracked) != 0;
        stateOut.InputValid = (handState.Status & ovrpHandStatus_InputValid) != 0;
        stateOut.SystemGestureInProgress = (handState.Status & ovrpHandStatus_SystemGestureInProgress) != 0;

        // Root Pose
        OculusHMD::FPose poseOut;
        if(!OculusHMD->ConvertPose_Internal(handState.RootPose, poseOut, Settings, worldToMeters))
        {
            UE_LOG(LogQuestHands, Warning, TEXT("Calling QuestHandsFunctions::GetTrackingState_Internal, RootPose not convertable!"));
        }
        stateOut.RootPose.Orientation = poseOut.Orientation;
        stateOut.RootPose.Position = poseOut.Position;

        // Bone Rotations
        stateOut.BoneRotations.AddDefaulted(((int32)EQHandBones::Hand_PinkyTip + 1) - stateOut.BoneRotations.Num());
        for(int32 boneIndex = 0; boneIndex < (int32)EQHandBones::Hand_PinkyTip + 1; ++boneIndex)
        {
            stateOut.BoneRotations[boneIndex] = OculusHMD::ToFQuat(handState.BoneRotations[boneIndex]);
        }

        // Pinch State
        stateOut.PinchState.AddDefaulted(((int32)EQHandFinger::HandFinger_Pinky + 1) - stateOut.PinchState.Num());
        for(int32 pinchIndex = 0; pinchIndex < (int32)EQHandFinger::HandFinger_Pinky + 1; ++pinchIndex)
        {
            stateOut.PinchState[pinchIndex].Finger = (EQHandFinger)pinchIndex;
            stateOut.PinchState[pinchIndex].Pinched = (handState.Pinches & (1 << pinchIndex)) != 0;
            stateOut.PinchState[pinchIndex].Strength = handState.PinchStrength[pinchIndex];
        }

        // Pointer Pose
        if(!OculusHMD->ConvertPose_Internal(handState.PointerPose, poseOut, Settings, worldToMeters))
        {
            UE_LOG(LogQuestHands, Warning, TEXT("Calling QuestHandsFunctions::GetTrackingState_Internal, RootPose not convertable!"));
        }
        stateOut.PointerPose.Orientation = poseOut.Orientation;
        stateOut.PointerPose.Position = poseOut.Position;

        // Hand Scale
        stateOut.HandScale = handState.HandScale;

        // Hand Confidence
        stateOut.HandConfidence = handState.HandConfidence == ovrpTrackingConfidence_High ?
            EQHandTrackingConfidence::Confidence_High : EQHandTrackingConfidence::Confidence_Low;

        return true;
    }
#endif

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
bool UQuestHandsFunctions::GetHandSkeleton(const UObject* WorldContextObject, const EControllerHand Hand, FQHandSkeleton& skeletonOut)
{
    if(!QuestHands::IsOVRAvailable())
    {
        UE_LOG(LogQuestHands, Error, TEXT("Calling QuestHandsFunctions::GetHandSkeleton when OVR is not available!"));
        return false;
    }

    UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
    if(!World)
    {
        UE_LOG(LogQuestHands, Error, TEXT("Calling QuestHandsFunctions::GetHandSkeleton when World is not available!"));
        return false;
    }

#if OCULUS_INPUT_SUPPORTED_PLATFORMS
    float worldToMeters = 100.0f;
    AWorldSettings *worldSettings = World->GetWorldSettings();
    if(worldSettings)
    {
        worldToMeters = worldSettings->WorldToMeters;
    }
    return GetHandSkeleton_Internal(Hand, skeletonOut, worldToMeters);
#else
    return false;
#endif
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
bool UQuestHandsFunctions::GetHandSkeleton_Internal(const EControllerHand Hand, FQHandSkeleton& skeletonOut, const float worldToMeters)
{
#if OCULUS_INPUT_SUPPORTED_PLATFORMS
    if(!GEngine->XRSystem.IsValid())
    {
        UE_LOG(LogQuestHands, Error, TEXT("Calling QuestHandsFunctions::GetHandSkeleton_Internal when XRSystem is not available!"));
        return false;
    }

    OculusHMD::FOculusHMD* OculusHMD = static_cast<OculusHMD::FOculusHMD*>(GEngine->XRSystem->GetHMDDevice());
    if(!OculusHMD)
    {
        UE_LOG(LogQuestHands, Error, TEXT("Calling QuestHandsFunctions::GetHandSkeleton_Internal when HMD is not available!"));
        return false;
    }

    OculusHMD::FSettings* Settings;
    if(IsInGameThread())
    {
        Settings = OculusHMD->GetSettings();
    }
    else
    {
        Settings = OculusHMD->GetSettings_RenderThread();
    }
    if(!Settings)
    {
        UE_LOG(LogQuestHands, Error, TEXT("Calling QuestHandsFunctions::GetHandSkeleton_Internal when HMD Settings is not available!"));
        return false;
    }

    if(!Settings->BaseOrientation.IsIdentity())
    {
        UE_LOG(LogQuestHands, Warning, TEXT("BaseOrientation not identity!"));
    }
    if(!Settings->BaseOffset.IsZero())
    {
        UE_LOG(LogQuestHands, Warning, TEXT("BaseOffset not zero!"));
    }

    ovrpSkeletonType hand = Hand == EControllerHand::Left ? ovrpSkeletonType_HandLeft : ovrpSkeletonType_HandRight;
    ovrpSkeleton skeleton;
    if(OVRP_SUCCESS(ovrp_GetSkeleton(hand, &skeleton)))
    {
        OculusHMD::FPose poseOut;

        // Bones
        skeletonOut.Bones.AddDefaulted(skeleton.NumBones - skeletonOut.Bones.Num());
        for(int32 boneIndex = 0; boneIndex < (int32)skeleton.NumBones; ++boneIndex)
        {
            skeletonOut.Bones[boneIndex].BoneId = (EQHandBones)skeleton.Bones[boneIndex].BoneId;
            skeletonOut.Bones[boneIndex].ParentBoneIndex = skeleton.Bones[boneIndex].ParentBoneIndex;

            if(!OculusHMD->ConvertPose_Internal(skeleton.Bones[boneIndex].Pose, poseOut, Settings, worldToMeters))
            {
                UE_LOG(LogQuestHands, Warning, TEXT("Calling QuestHandsFunctions::GetHandSkeleton_Internal, bone pose %d not convertable!"), boneIndex);
            }
            skeletonOut.Bones[boneIndex].Pose.Orientation = poseOut.Orientation;
            skeletonOut.Bones[boneIndex].Pose.Position = poseOut.Position;
        }

        // Capsules
        skeletonOut.BoneCapsules.AddDefaulted(skeleton.NumBoneCapsules - skeletonOut.BoneCapsules.Num());
        for(int32 capsuleIndex = 0; capsuleIndex < (int32)skeleton.NumBoneCapsules; ++capsuleIndex)
        {
            skeletonOut.BoneCapsules[capsuleIndex].PointA = OculusHMD::ToFVector(skeleton.BoneCapsules[capsuleIndex].Points[0]);
            skeletonOut.BoneCapsules[capsuleIndex].PointB = OculusHMD::ToFVector(skeleton.BoneCapsules[capsuleIndex].Points[1]);
            skeletonOut.BoneCapsules[capsuleIndex].Radius = skeleton.BoneCapsules[capsuleIndex].Radius;
        }

        return true;
    }
#endif

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
/**

FVector UQuestHandsFunctions::OVecToScene(const UObject* WorldContextObject, const FVector& vecIn)
{
    if(!QuestHands::IsOVRAvailable())
    {
        UE_LOG(LogQuestHands, Error, TEXT("Calling QuestHandsFunctions::OVecToScene when OVR is not available!"));
        return FVector::ZeroVector;
    }

    UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
    if(!World)
    {
        UE_LOG(LogQuestHands, Error, TEXT("Calling QuestHandsFunctions::OVecToScene when World is not available!"));
        return FVector::ZeroVector;
    }

#if OCULUS_INPUT_SUPPORTED_PLATFORMS
    OculusHMD::FOculusHMD* OculusHMD = static_cast<OculusHMD::FOculusHMD*>(GEngine->XRSystem->GetHMDDevice());
    if(!OculusHMD)
    {
        UE_LOG(LogQuestHands, Error, TEXT("Calling QuestHandsFunctions::OVecToScene when HMD is not available!"));
        return FVector::ZeroVector;
    }

    OculusHMD::FSettings* Settings;
    if(IsInGameThread())
    {
        Settings = OculusHMD->GetSettings();
    }
    else
    {
        Settings = OculusHMD->GetSettings_RenderThread();
    }
    if(!Settings)
    {
        UE_LOG(LogQuestHands, Error, TEXT("Calling QuestHandsFunctions::OVecToScene when HMD Settings is not available!"));
        return FVector::ZeroVector;
    }

    float worldToMeters = 100.0f;
    AWorldSettings *worldSettings = World->GetWorldSettings();
    if(worldSettings)
    {
        worldToMeters = worldSettings->WorldToMeters;
    }

    return QuestHands::TransformVectorToUnrealScene(vecIn, Settings, worldToMeters);
#endif

    return FVector::ZeroVector;
}*/

//---------------------------------------------------------------------------------------------------------------------
/**

FQuat UQuestHandsFunctions::OQuatToScene(const UObject* WorldContextObject, const FQuat& quatIn)
{
    if(!QuestHands::IsOVRAvailable())
    {
        UE_LOG(LogQuestHands, Error, TEXT("Calling QuestHandsFunctions::OQuatToScene when OVR is not available!"));
        return FQuat::Identity;
    }

    UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
    if(!World)
    {
        UE_LOG(LogQuestHands, Error, TEXT("Calling QuestHandsFunctions::OQuatToScene when World is not available!"));
        return FQuat::Identity;
    }

#if OCULUS_INPUT_SUPPORTED_PLATFORMS
    OculusHMD::FOculusHMD* OculusHMD = static_cast<OculusHMD::FOculusHMD*>(GEngine->XRSystem->GetHMDDevice());
    if(!OculusHMD)
    {
        UE_LOG(LogQuestHands, Error, TEXT("Calling QuestHandsFunctions::OQuatToScene when HMD is not available!"));
        return FQuat::Identity;
    }

    OculusHMD::FSettings* Settings;
    if(IsInGameThread())
    {
        Settings = OculusHMD->GetSettings();
    }
    else
    {
        Settings = OculusHMD->GetSettings_RenderThread();
    }
    if(!Settings)
    {
        UE_LOG(LogQuestHands, Error, TEXT("Calling QuestHandsFunctions::OQuatToScene when HMD Settings is not available!"));
        return FQuat::Identity;
    }

    float worldToMeters = 100.0f;
    AWorldSettings *worldSettings = World->GetWorldSettings();
    if(worldSettings)
    {
        worldToMeters = worldSettings->WorldToMeters;
    }

    return QuestHands::TransformQuatToUnrealScene(quatIn, Settings, worldToMeters);
#endif

    return FQuat::Identity;
}*/

//---------------------------------------------------------------------------------------------------------------------
/**
*/
FString UQuestHandsFunctions::GetHandBoneName(const EQHandBones bone, bool left)
{
    if((uint8)bone > (uint8)EQHandBones::Hand_PinkyTip)
    {
        UE_LOG(LogQuestHands, Error, TEXT("Invalid bone index %d passed into GetHandBoneName"), (int32)bone);
        return TEXT("");
    }

    FString nameOut;
    if(bone > EQHandBones::Hand_Pinky3)
    {
        nameOut = left ? TEXT("l_") : TEXT("r_");
        switch(bone)
        {
            case EQHandBones::Hand_ThumbTip:
                nameOut += TEXT("thumb_finger_tip_marker");
                break;
            case EQHandBones::Hand_IndexTip:
                nameOut += TEXT("index_finger_tip_marker");
                break;
            case EQHandBones::Hand_MiddleTip:
                nameOut += TEXT("middle_finger_tip_marker");
                break;
            case EQHandBones::Hand_RingTip:
                nameOut += TEXT("ring_finger_tip_marker");
                break;
            case EQHandBones::Hand_PinkyTip:
                nameOut += TEXT("pinky_finger_tip_marker");
                break;
            default:
                UE_LOG(LogQuestHands, Error, TEXT("Invalid bone index %d passed into GetHandBoneName"), (int32)bone);
                return TEXT("");
        }
    }
    else
    {
        const UEnum* const QHandBonesTypeEnum = StaticEnum<EQHandBones>();
        FString boneName = QHandBonesTypeEnum->GetNameStringByValue((uint8)bone);


        FString boneNameStr;
        boneName.Split(TEXT("_"), nullptr, &boneNameStr);
        boneNameStr.ToLowerInline();
        nameOut = FString::Printf(TEXT("b_%s_%s"), left ? TEXT("l") : TEXT("r"), *boneNameStr);
    }

    return nameOut;
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
bool UQuestHandsFunctions::SetDynamicFixedFoveatedEnabled(bool enabled)
{
#if OCULUS_INPUT_SUPPORTED_PLATFORMS
    return OVRP_SUCCESS(ovrp_SetVrApiPropertyInt(QuestHands::VRAPI_DYNAMIC_FOVEATION_ENABLED, enabled ? 1 : 0));
#else
    return false;
#endif
}
