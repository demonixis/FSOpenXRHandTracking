// Fill out your copyright notice in the Description page of Project Settings.


#include "FSInstancedHand.h"
#include "EnhancedInputSubsystems.h"
#include "XRVisualizationFunctionLibrary.h"
#include "InputActionValue.h"
#include "InputModifiers.h"
#include "InputTriggers.h"
#if WITH_METAXR
#include "OculusXRInputFunctionLibrary.h"
#endif
#include "PoseableMeshComponent.h"
#include "Kismet/GameplayStatics.h"

UFSInstancedHand::UFSInstancedHand()
{
	bHandTracked = false;
	bPreviousHandTracked = false;
	bLeftHand = false;
	HandRendering = EFSOpenXRHandRendering::Both;
	PinchThreshold = 1.5f;
	BoneScale = 0.015f;
	WireframeColor = FColor::Blue;
	WireframeThickness = 0.35f;
	bRenderWireframePalm = false;
	bRenderWireframeBones = false;
	bUpdateHandPointer = false;
	PointerContainer = nullptr;
	HandPointerAngleFromPalm = -45.0f;
	HandPointerLocationSpeed = 8.0f;
	HandPointerRotationSpeed = 2.0f;
	HandPointerDepth = 0;
	bHideHand = false;
	bHideHandPointerWhenNotTracked = false;
	BoneLocations.Init(FVector::ZeroVector, EHandKeypointCount);
	BoneRotations.Init(FRotator::ZeroRotator, EHandKeypointCount);
	BoneRelativeRotations.Init(FRotator::ZeroRotator, EHandKeypointCount);

	// Use the default Cube by default
	const auto MeshAsset =
		ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));

	if (MeshAsset.Object != nullptr)
		UStaticMeshComponent::SetStaticMesh(MeshAsset.Object);

	constexpr int InputActionCount = static_cast<int>(EFSOpenXRPinchFingers::Little) + 1;
	InputActions.Init(nullptr, InputActionCount); // 4 Pinchable fingers.
}

FTransform UFSInstancedHand::GetHandTransform() const
{
	return bHandTracked ? CurrentHandTransform : FallbackTransform;
}

bool UFSInstancedHand::UpdateHand(const FXRMotionControllerData& InData, const float DeltaTime)
{
	ClearInstances();

	bHandTracked = InData.bValid;

	if (bHandTracked != bPreviousHandTracked)
	{
		bPreviousHandTracked = bHandTracked;

		// Enable/Disable the hand ray if needed.
		if (bUpdateHandPointer && PointerContainer != nullptr)
			PointerContainer->SetVisibility(bHandTracked, true);

		HandTrackingEnableChanged.Broadcast(bLeftHand, bHandTracked);
	}

	if (!bHandTracked) return false;

	// Render wireframe if needed
	if (!bHideHand && HandRendering == EFSOpenXRHandRendering::Both || HandRendering ==
		EFSOpenXRHandRendering::Wireframe)
	{
		RenderFinger(InData, EHandKeypoint::ThumbMetacarpal, EHandKeypoint::ThumbTip);
		RenderFinger(InData, EHandKeypoint::IndexMetacarpal, EHandKeypoint::IndexTip);
		RenderFinger(InData, EHandKeypoint::MiddleMetacarpal, EHandKeypoint::MiddleTip);
		RenderFinger(InData, EHandKeypoint::RingMetacarpal, EHandKeypoint::RingTip);
		RenderFinger(InData, EHandKeypoint::LittleMetacarpal, EHandKeypoint::LittleTip);
	}

	FTransform BoneTransform;

	constexpr uint8 PalmIndex = static_cast<uint8>(EHandKeypoint::Palm);

	// Populate array data and add instances.
	for (int i = 0; i < InData.HandKeyPositions.Num(); i++)
	{
		BoneLocations[i] = InData.HandKeyPositions[i];
		BoneRotations[i] = InData.HandKeyRotations[i].Rotator();

		if (i == PalmIndex)
		{
			CurrentHandTransform.SetLocation(InData.HandKeyPositions[i]);
			CurrentHandTransform.SetRotation(InData.HandKeyRotations[i]);
		}

		BoneTransform.SetLocation(InData.HandKeyPositions[i]);
		BoneTransform.SetRotation(InData.HandKeyRotations[i]);

		const float BoneScaleFactor = InData.HandKeyRadii[i] * BoneScale;
		FVector BoneScale3D = FVector(BoneScaleFactor, BoneScaleFactor, BoneScaleFactor);
		BoneTransform.SetScale3D(BoneScale3D);

		bool bDisplayBone = !bHideHand;

		if (bDisplayBone && bOnlyDisplayTips)
		{
			bDisplayBone =
				i == static_cast<uint8>(EHandKeypoint::ThumbTip) ||
				i == static_cast<uint8>(EHandKeypoint::IndexTip) ||
				i == static_cast<uint8>(EHandKeypoint::MiddleTip) ||
				i == static_cast<uint8>(EHandKeypoint::RingTip) ||
				i == static_cast<uint8>(EHandKeypoint::LittleTip);
		}

		if (bDisplayBone)
			AddInstance(BoneTransform, true);
	}

	// Broadcast Pinch Events
	for (int i = 0; i < InputActions.Num(); i++)
	{
		if (InputActions[i] == nullptr) continue;
		const EFSOpenXRPinchFingers Finger = static_cast<EFSOpenXRPinchFingers>(i);

		if (IsPinching(Finger))
			OverrideInputWithAction(InputActions[i], 1.0f);
	}

	// Update the Hand Pointer if needed
	if (bUpdateHandPointer && PointerContainer != nullptr)
	{
		// Get the current pointer transform
		const FVector PointerLocation = PointerContainer->GetComponentLocation();
		const FRotator PointerRotation = PointerContainer->GetComponentRotation();

		// Get the target pointer transform and add an angle to the ray
		const FVector PalmLocation = BoneLocations[PalmIndex];
		FRotator PalmRotation = BoneRotations[PalmIndex];
		PalmRotation.Pitch += HandPointerAngleFromPalm;

		// Move the container
		const FVector TargetLocation = FMath::Lerp(PointerLocation, PalmLocation, DeltaTime * HandPointerLocationSpeed);
		const FRotator TargetRotation =
			FMath::Lerp(PointerRotation, PalmRotation, DeltaTime * HandPointerRotationSpeed);
		PointerContainer->SetWorldLocationAndRotation(TargetLocation, TargetRotation);
	}

	if (bComputeRelativeRotations)
	{
		for (int i = 0; i < InData.HandKeyRotations.Num(); ++i)
		{
			const int ParentIndex = GetParentIndex(static_cast<EHandKeypoint>(i));
			if (ParentIndex != INDEX_NONE)
			{
				FQuat ParentQuat = InData.HandKeyRotations[ParentIndex];
				FQuat BoneQuat = InData.HandKeyRotations[i];
				FQuat RelativeQuat = ParentQuat.Inverse() * BoneQuat;
				BoneRelativeRotations[i] = RelativeQuat.Rotator();
			}
			else
			{
				BoneRelativeRotations[i] = InData.HandKeyRotations[i].Rotator();
			}
		}
	}

	return true;
}

void UFSInstancedHand::GetDataFromSkeleton(UPoseableMeshComponent* Target, const bool bLeft, FXRMotionControllerData& OutData)
{
#if WITH_METAXR
	auto Pose = UOculusXRInputFunctionLibrary::GetPointerPose(
		bLeft ? EOculusXRHandType::HandLeft : EOculusXRHandType::HandRight);
	
	OutData.bValid = Target->IsVisible();
	OutData.HandIndex = bLeft ? EControllerHand::Left : EControllerHand::Right;
	OutData.AimPosition = Pose.GetLocation();
	OutData.AimRotation = Pose.GetRotation();
	OutData.DeviceName = bLeft ? "OculusXRLeftHand" : "OculusXRRightHand";
	OutData.TrackingStatus = OutData.bValid ? ETrackingStatus::Tracked : ETrackingStatus::NotTracked;
	OutData.DeviceVisualType = EXRVisualType::Hand;
	OutData.HandKeyPositions.Empty();
	OutData.HandKeyRotations.Empty();
	OutData.HandKeyRadii.Empty();
	EHandKeypoint HandKeyPoint;
	uint8 BoneIndex;
	EOculusXRBone Bone;
	FString BoneName;
	FName BoneFName;
	FVector WorldLocation;
	FRotator WorldRotation;

	for (int i = 0; i < EHandKeypointCount; i++)
	{
		HandKeyPoint = static_cast<EHandKeypoint>(i);
		BoneIndex = GetOculusBone(HandKeyPoint);
		Bone = static_cast<EOculusXRBone>(BoneIndex);
		BoneName = UOculusXRInputFunctionLibrary::GetBoneName(Bone);
		BoneFName = FName(*BoneName);

		WorldLocation = Target->GetBoneLocationByName(BoneFName, EBoneSpaces::WorldSpace);
		WorldRotation = Target->GetBoneRotationByName(BoneFName, EBoneSpaces::WorldSpace);

		if (HandKeyPoint == EHandKeypoint::Palm)
		{
			OutData.PalmPosition = WorldLocation;
			OutData.PalmRotation = WorldRotation.Quaternion();
			OutData.GripPosition = WorldLocation;
			OutData.GripRotation = WorldRotation.Quaternion();
		}

		OutData.HandKeyPositions.Add(WorldLocation);
		OutData.HandKeyRotations.Add(WorldRotation.Quaternion());
		OutData.HandKeyRadii.Add(0.2f);
	}
#endif
}

bool UFSInstancedHand::IsHandTracked() const
{
	return bHandTracked;
}

bool UFSInstancedHand::IsPinching(const EFSOpenXRPinchFingers Finger) const
{
	constexpr uint8 ThumbIndex = static_cast<uint8>(EHandKeypoint::ThumbTip);
	uint8 OtherIndex = static_cast<uint8>(EHandKeypoint::IndexTip);

	if (Finger == EFSOpenXRPinchFingers::Middle)
		OtherIndex = static_cast<uint8>(EHandKeypoint::MiddleTip);
	else if (Finger == EFSOpenXRPinchFingers::Ring)
		OtherIndex = static_cast<uint8>(EHandKeypoint::RingTip);
	else if (Finger == EFSOpenXRPinchFingers::Little)
		OtherIndex = static_cast<uint8>(EHandKeypoint::LittleTip);

	return FVector::Dist(BoneLocations[ThumbIndex], BoneLocations[OtherIndex]) <= PinchThreshold;
}

void UFSInstancedHand::RegisterInputAction(const EFSOpenXRPinchFingers Finger, UInputAction* InInputAction)
{
	const int Index = static_cast<int>(Finger);
	InputActions[Index] = InInputAction;
}

void UFSInstancedHand::RegisterHandRay(USceneComponent* InRayContainer)
{
	PointerContainer = InRayContainer;
	bUpdateHandPointer = InRayContainer != nullptr;
}

FRotator UFSInstancedHand::GetBoneRotation(const EHandKeypoint Keypoint) const
{
	const int Index = static_cast<int>(Keypoint);
	return BoneRotations[Index];
}

FVector UFSInstancedHand::GetBoneLocation(const EHandKeypoint Keypoint) const
{
	const int Index = static_cast<int>(Keypoint);
	return BoneLocations[Index];
}

FRotator UFSInstancedHand::GetBoneRelativeRotation(const EHandKeypoint Keypoint) const
{
	const int Index = static_cast<int>(Keypoint);
	return BoneRelativeRotations[Index];
}

void UFSInstancedHand::OverrideInputWithAction(const UInputAction* InInputAction, const float Value) const
{
	const APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());

	Subsystem->InjectInputForAction(
		InInputAction,
		FInputActionValue(Value),
		TArray<UInputModifier*>(),
		TArray<UInputTrigger*>());
}

int32 UFSInstancedHand::GetParentIndex(EHandKeypoint Keypoint)
{
	static TMap<EHandKeypoint, EHandKeypoint> ParentMap;
	if (ParentMap.Num() == 0)
	{
		// Thumb
		ParentMap.Add(EHandKeypoint::ThumbTip, EHandKeypoint::ThumbDistal);
		ParentMap.Add(EHandKeypoint::ThumbDistal, EHandKeypoint::ThumbProximal);
		ParentMap.Add(EHandKeypoint::ThumbProximal, EHandKeypoint::ThumbMetacarpal);

		// Index
		ParentMap.Add(EHandKeypoint::IndexTip, EHandKeypoint::IndexDistal);
		ParentMap.Add(EHandKeypoint::IndexDistal, EHandKeypoint::IndexIntermediate);
		ParentMap.Add(EHandKeypoint::IndexIntermediate, EHandKeypoint::IndexProximal);
		ParentMap.Add(EHandKeypoint::IndexProximal, EHandKeypoint::IndexMetacarpal);

		// Middle
		ParentMap.Add(EHandKeypoint::MiddleTip, EHandKeypoint::MiddleDistal);
		ParentMap.Add(EHandKeypoint::MiddleDistal, EHandKeypoint::MiddleIntermediate);
		ParentMap.Add(EHandKeypoint::MiddleIntermediate, EHandKeypoint::MiddleProximal);
		ParentMap.Add(EHandKeypoint::MiddleProximal, EHandKeypoint::MiddleMetacarpal);

		// Ring
		ParentMap.Add(EHandKeypoint::RingTip, EHandKeypoint::RingDistal);
		ParentMap.Add(EHandKeypoint::RingDistal, EHandKeypoint::RingIntermediate);
		ParentMap.Add(EHandKeypoint::RingIntermediate, EHandKeypoint::RingProximal);
		ParentMap.Add(EHandKeypoint::RingProximal, EHandKeypoint::RingMetacarpal);

		// Little
		ParentMap.Add(EHandKeypoint::LittleTip, EHandKeypoint::LittleDistal);
		ParentMap.Add(EHandKeypoint::LittleDistal, EHandKeypoint::LittleIntermediate);
		ParentMap.Add(EHandKeypoint::LittleIntermediate, EHandKeypoint::LittleProximal);
		ParentMap.Add(EHandKeypoint::LittleProximal, EHandKeypoint::LittleMetacarpal);

		// Metacarpals to Palm
		ParentMap.Add(EHandKeypoint::ThumbMetacarpal, EHandKeypoint::Palm);
		ParentMap.Add(EHandKeypoint::IndexMetacarpal, EHandKeypoint::Palm);
		ParentMap.Add(EHandKeypoint::MiddleMetacarpal, EHandKeypoint::Palm);
		ParentMap.Add(EHandKeypoint::RingMetacarpal, EHandKeypoint::Palm);
		ParentMap.Add(EHandKeypoint::LittleMetacarpal, EHandKeypoint::Palm);

		// Wrist to Palm
		ParentMap.Add(EHandKeypoint::Palm, EHandKeypoint::Wrist);
	}

	if (ParentMap.Contains(Keypoint))
	{
		return static_cast<int32>(ParentMap[Keypoint]);
	}
	return INDEX_NONE;
}

uint8 UFSInstancedHand::GetOculusBone(EHandKeypoint Keypoint)
{
#if WITH_METAXR
	if (Keypoint == EHandKeypoint::Wrist || Keypoint == EHandKeypoint::Palm)
		return static_cast<uint8>(EOculusXRBone::Hand_Start);

	if (Keypoint == EHandKeypoint::ThumbMetacarpal)
		return static_cast<uint8>(EOculusXRBone::Thumb_1);
	if (Keypoint == EHandKeypoint::ThumbProximal)
		return static_cast<uint8>(EOculusXRBone::Thumb_2);
	if (Keypoint == EHandKeypoint::ThumbDistal)
		return static_cast<uint8>(EOculusXRBone::Thumb_3);
	if (Keypoint == EHandKeypoint::ThumbTip)
		return static_cast<uint8>(EOculusXRBone::Thumb_Tip);

	if (Keypoint == EHandKeypoint::IndexProximal || Keypoint == EHandKeypoint::IndexMetacarpal)
		return static_cast<uint8>(EOculusXRBone::Index_1);
	if (Keypoint == EHandKeypoint::IndexIntermediate)
		return static_cast<uint8>(EOculusXRBone::Index_2);
	if (Keypoint == EHandKeypoint::IndexDistal)
		return static_cast<uint8>(EOculusXRBone::Index_3);
	if (Keypoint == EHandKeypoint::IndexTip)
		return static_cast<uint8>(EOculusXRBone::Index_Tip);

	if (Keypoint == EHandKeypoint::MiddleProximal || Keypoint == EHandKeypoint::MiddleMetacarpal)
		return static_cast<uint8>(EOculusXRBone::Middle_1);
	if (Keypoint == EHandKeypoint::MiddleIntermediate)
		return static_cast<uint8>(EOculusXRBone::Middle_2);
	if (Keypoint == EHandKeypoint::MiddleDistal)
		return static_cast<uint8>(EOculusXRBone::Middle_3);
	if (Keypoint == EHandKeypoint::MiddleTip)
		return static_cast<uint8>(EOculusXRBone::Middle_Tip);

	if (Keypoint == EHandKeypoint::RingProximal || Keypoint == EHandKeypoint::RingMetacarpal)
		return static_cast<uint8>(EOculusXRBone::Ring_1);
	if (Keypoint == EHandKeypoint::RingIntermediate)
		return static_cast<uint8>(EOculusXRBone::Ring_2);
	if (Keypoint == EHandKeypoint::RingDistal)
		return static_cast<uint8>(EOculusXRBone::Ring_3);
	if (Keypoint == EHandKeypoint::RingTip)
		return static_cast<uint8>(EOculusXRBone::Ring_Tip);

	if (Keypoint == EHandKeypoint::LittleMetacarpal)
		return static_cast<uint8>(EOculusXRBone::Pinky_0);
	if (Keypoint == EHandKeypoint::LittleProximal)
		return static_cast<uint8>(EOculusXRBone::Pinky_1);
	if (Keypoint == EHandKeypoint::LittleIntermediate)
		return static_cast<uint8>(EOculusXRBone::Pinky_2);
	if (Keypoint == EHandKeypoint::LittleDistal)
		return static_cast<uint8>(EOculusXRBone::Pinky_3);
	if (Keypoint == EHandKeypoint::LittleTip)
		return static_cast<uint8>(EOculusXRBone::Pinky_Tip);
#endif

	return 0;
}

// Code forked from UXRVisualizationFunctionLibrary::RenderFinger
void UFSInstancedHand::RenderFinger(const FXRMotionControllerData& InData, const EHandKeypoint FingerStart,
                                    const EHandKeypoint FingerEnd) const
{
	const uint32 IndexStart = static_cast<uint32>(FingerStart);
	const uint32 IndexStop = static_cast<uint32>(FingerEnd);

	const bool bValid = ((IndexStart > 0) && (IndexStart < EHandKeypointCount)) &&
		((IndexStop > 0) && (IndexStop < EHandKeypointCount)) &&
		(EHandKeypointCount == InData.HandKeyPositions.Num()) && (EHandKeypointCount == InData.HandKeyRadii.Num());

	if (!bValid) return;

	const UWorld* World = GetWorld();

	if (bRenderWireframePalm)
	{
		DrawDebugLine(World, InData.HandKeyPositions[0], InData.HandKeyPositions[1], WireframeColor, false,
		              -1, HandPointerDepth, WireframeThickness);
		DrawDebugLine(World, InData.HandKeyPositions[1], InData.HandKeyPositions[IndexStart], WireframeColor,
		              false, -1, HandPointerDepth, WireframeThickness);
	}

	//Iterate from FingerStart to Finger End
	for (uint32 DigitIndex = IndexStart; DigitIndex < IndexStop; ++DigitIndex)
	{
		DrawDebugLine(World, InData.HandKeyPositions[DigitIndex], InData.HandKeyPositions[DigitIndex + 1],
		              WireframeColor, false, -1, HandPointerDepth, WireframeThickness);

		if (bRenderWireframeBones)
		{
			DrawDebugSphere(World, InData.HandKeyPositions[DigitIndex + 1], InData.HandKeyRadii[DigitIndex + 1], 4,
			                WireframeColor, false, -1, HandPointerDepth, WireframeThickness);
		}
	}
}
