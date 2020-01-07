// Copyright(c) 2020 Sheffer Online Services

#pragma once

#include "QuestHands.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InputCoreTypes.h"

#include "QuestHandsFunctions.generated.h"

UENUM(BlueprintType, DisplayName = "Hand Bones")
enum class EQHandBones : uint8
{
    // root frame of the hand, where the wrist is located
    Hand_Wrist = 0, // root frame of the hand, where the wrist is located
    Hand_Forearm_Stub = 1, // frame for user's forearm
    Hand_Thumb0 = 2, // thumb trapezium bone
    Hand_Thumb1 = 3, // thumb metacarpal bone
    Hand_Thumb2 = 4, // thumb proximal phalange bone
    Hand_Thumb3 = 5, // thumb distal phalange bone
    Hand_Index1 = 6, // index proximal phalange bone
    Hand_Index2 = 7, // index intermediate phalange bone
    Hand_Index3 = 8, // index distal phalange bone
    Hand_Middle1 = 9, // middle proximal phalange bone
    Hand_Middle2 = 10, // middle intermediate phalange bone
    Hand_Middle3 = 11, // middle distal phalange bone
    Hand_Ring1 = 12, // ring proximal phalange bone
    Hand_Ring2 = 13, // ring intermediate phalange bone
    Hand_Ring3 = 14, // ring distal phalange bone
    Hand_Pinky0 = 15, // pinky metacarpal bone
    Hand_Pinky1 = 16, // pinky proximal phalange bone
    Hand_Pinky2 = 17, // pinky intermediate phalange bone
    Hand_Pinky3 = 18, // pinky distal phalange bone

    // Bone tips are position only. They are not used for skinning but useful for hit-testing.
    // NOTE: ovrBoneId_Hand_ThumbTip == ovrBoneId_Hand_MaxSkinnable since the extended tips need to be contiguous
    Hand_ThumbTip = 19, // tip of the thumb
    Hand_IndexTip = 20, // tip of the index finger
    Hand_MiddleTip = 21, // tip of the middle finger
    Hand_RingTip = 22, // tip of the ring finger
    Hand_PinkyTip = 23, // tip of the pinky
};

UENUM(BlueprintType, DisplayName = "Hand Finger")
enum class EQHandFinger : uint8
{
    HandFinger_Thumb = 0,
    HandFinger_Index = 1,
    HandFinger_Middle = 2,
    HandFinger_Ring = 3,
    HandFinger_Pinky = 4,
};

UENUM(BlueprintType, DisplayName="Hand Tracking Confidence")
enum class EQHandTrackingConfidence : uint8
{
    Confidence_Low = 0,
    Confidence_High = 1
};

// The Unreal version of a special structured used internally by Oculus which describes a pose
USTRUCT(BlueprintType, DisplayName = "Oculus Pose")
struct FOculusPose
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "OculusPose")
    FQuat Orientation;

    UPROPERTY(BlueprintReadWrite, Category = "OculusPose")
    FVector Position;
};

USTRUCT(BlueprintType, DisplayName = "Hand Bone Capsule")
struct FQHandBoneCapsule
{
    GENERATED_BODY()

    // index of the bone
    UPROPERTY(BlueprintReadWrite, Category = "HandBone")
    int32 BoneIndex;

    // Points at either end of the cylinder inscribed in the capsule. Also the center points for
    // spheres at either end of the capsule. Points A and B in the diagram above.
    UPROPERTY(BlueprintReadWrite, Category = "BoneCapsule")
    FVector PointA;

    // Points at either end of the cylinder inscribed in the capsule. Also the center points for
    // spheres at either end of the capsule. Points A and B in the diagram above.
    UPROPERTY(BlueprintReadWrite, Category = "BoneCapsule")
    FVector PointB;

    // The radius of the capsule cylinder and of the half-sphere caps on the ends of the capsule.
    UPROPERTY(BlueprintReadWrite, Category = "BoneCapsule")
    float Radius;
};

USTRUCT(BlueprintType, DisplayName = "Hand Bone")
struct FQHandBone
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "HandBone")
    EQHandBones BoneId;

    // index of this bone's parent bone (-1 if no parent)
    UPROPERTY(BlueprintReadWrite, Category = "HandBone")
    int32 ParentBoneIndex;

    UPROPERTY(BlueprintReadWrite, Category = "HandBone")
    FOculusPose Pose;
};

USTRUCT(BlueprintType, DisplayName="Hand Pinch State")
struct FQHandPinchState
{
    GENERATED_BODY()

    // The finger
    UPROPERTY(BlueprintReadWrite, Category = "HandPinchState")
    EQHandFinger Finger;

    // Is considered pinched?
    UPROPERTY(BlueprintReadWrite, Category = "HandPinchState")
    bool Pinched;

    // The strength of the pinch
    UPROPERTY(BlueprintReadWrite, Category = "HandPinchState")
    float Strength;
};

USTRUCT(BlueprintType, DisplayName="Hand Tracking State")
struct FQHandTrackingState
{
    GENERATED_BODY()

    // hand is currently tracked by hand tracking
    UPROPERTY(BlueprintReadWrite, Category = "HandTrackingState")
    bool IsTracked;

    // if this is set the pointer pose and pinch data is usable
    UPROPERTY(BlueprintReadWrite, Category = "HandTrackingState")
    bool InputValid;

    // if this is set the user is performing the system gesture
    UPROPERTY(BlueprintReadWrite, Category = "HandTrackingState")
    bool SystemGestureInProgress;

    // Root pose of the hand in world space. Not to be confused with the root bone's transform.
    // The root bone can still be offset from this by the skeleton's rest pose.
    UPROPERTY(BlueprintReadWrite, Category = "HandTrackingState")
    FOculusPose RootPose;

    // Current rotation of each bone.
    // The order of this array coincides with the enum Hand Bones
    UPROPERTY(BlueprintReadWrite, Category = "HandTrackingState")
    TArray<FQuat> BoneRotations;

    // The pinch state per finger.
    UPROPERTY(BlueprintReadWrite, Category = "HandTrackingState")
    TArray<FQHandPinchState> PinchState;

    // World space position and translation of the pointer attached to the hand.
    UPROPERTY(BlueprintReadWrite, Category = "HandTrackingState")
    FOculusPose PointerPose;

    UPROPERTY(BlueprintReadWrite, Category = "HandTrackingState")
    float HandScale;

    // The confidence level of the tracking for this hand
    UPROPERTY(BlueprintReadWrite, Category = "HandTrackingState")
    EQHandTrackingConfidence HandConfidence;
};

USTRUCT(BlueprintType, DisplayName = "Hand Skeleton")
struct FQHandSkeleton
{
    GENERATED_BODY()

    // Bones corresponding with the Enum Hand Bones
    UPROPERTY(BlueprintReadWrite, Category = "HandSkeleton")
    TArray<FQHandBone> Bones;

    // Bone Capsules corresponding with the Enum Hand Bones minus the bone tips
    UPROPERTY(BlueprintReadWrite, Category = "HandSkeleton")
    TArray<FQHandBoneCapsule> BoneCapsules;
};

//---------------------------------------------------------------------------------------------------------------------
/**
  * Blueprint Functions for querying the Oculus Hands Interface
*/
UCLASS()
class QUESTHANDS_API UQuestHandsFunctions : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:

    /** Quest Hand tracking enabled state */
    UFUNCTION(BlueprintPure, Category = "QuestHands")
    static bool IsHandTrackingEnabled();

    /**
     * Get the hand tracking state for a hand
     * Use this to update rendering, gestures, etc...
    */
    UFUNCTION(BlueprintCallable, Category = "QuestHands", meta = (WorldContext = "WorldContextObject"))
    static bool GetTrackingState(const UObject* WorldContextObject, const EControllerHand Hand, FQHandTrackingState& stateOut);

    // Internal version for native, not blueprint accessible!
    static bool GetTrackingState_Internal(const EControllerHand Hand, FQHandTrackingState& stateOut, const float worldToMeters);

    /**
     * Get the hand skeleton in its reference pose.
     * This is used to initialize a set of bones and collision capsules.
    */
    UFUNCTION(BlueprintCallable, Category = "QuestHands", meta = (WorldContext = "WorldContextObject"))
    static bool GetHandSkeleton(const UObject* WorldContextObject, const EControllerHand Hand, FQHandSkeleton& skeletonOut);

    // Internal version for native, not blueprint accessible!
    static bool GetHandSkeleton_Internal(const EControllerHand Hand, FQHandSkeleton& skeletonOut, const float worldToMeters);

    /** From the bone enum value return the standard bone name that would have been assigned to the Oculus example hand skeletal mesh */
    UFUNCTION(BlueprintPure, Category = "QuestHands", meta = (WorldContext = "WorldContextObject"))
    static FString GetHandBoneName(const EQHandBones bone, bool left);

    /**
     * Set the state of dynamic fixed foveated rendering.
     * Enabling sets the foveation level to be automatically adjusted based on GPU utilization
     * From the documentation:
     * You must first specify a maximum level of foveation. Once this has been set, dynamic foveation can be enabled.
     * Results if the success value from the OVR API
    */
    UFUNCTION(BlueprintCallable, Category="QuestHands")
    static bool SetDynamicFixedFoveatedEnabled(bool enabled);
};
