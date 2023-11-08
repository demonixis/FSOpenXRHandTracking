// Fill out your copyright notice in the Description page of Project Settings.


#include "FSInstancedHand.h"

#include "EnhancedInputSubsystems.h"
#include "XRVisualizationFunctionLibrary.h"
#include "InputActionValue.h"
#include "InputModifiers.h"
#include "InputTriggers.h"
#include "Kismet/GameplayStatics.h"

UFSInstancedHand::UFSInstancedHand()
{
	bHandTracked = false;
	bLeftHand = false;
	HandRendering = EFSOpenXRHandRendering::InstancedMesh;
	PinchThreshold = 1.5f;
	BoneScale = 0.01f;
	WireframeColor = FColor::Blue;
	WireframeThickness = 0.05f;
	bRenderWireframePalm = true;
	bRenderWireframeBones = true;
	bUpdateHandPointer = false;
	PointerContainer = nullptr;
	HandPointerAngleFromPalm = 45;
	HandPointerLocationSpeed = 8.0f;
	HandPointerRotationSpeed = 4.0f;
	HandPointerDepth = 0;
	BoneLocations.Init(FVector::ZeroVector, EHandKeypointCount);
	BoneRotations.Init(FRotator::ZeroRotator, EHandKeypointCount);

	constexpr int InputActionCount = static_cast<int>(EFSOpenXRPinchFingers::Little) + 1;
	InputActions.Init(nullptr, InputActionCount); // 4 Pinchable fingers.
}

bool UFSInstancedHand::UpdateHand(const FXRMotionControllerData& InData)
{
	ClearInstances();

	bHandTracked = InData.bValid;

	if (!bHandTracked)
		return false;

	// Render wireframe if needed
	if (HandRendering == EFSOpenXRHandRendering::Both || HandRendering == EFSOpenXRHandRendering::Wireframe)
	{
		RenderFinger(InData, EHandKeypoint::ThumbMetacarpal, EHandKeypoint::ThumbTip);
		RenderFinger(InData, EHandKeypoint::IndexMetacarpal, EHandKeypoint::IndexTip);
		RenderFinger(InData, EHandKeypoint::MiddleMetacarpal, EHandKeypoint::MiddleTip);
		RenderFinger(InData, EHandKeypoint::RingMetacarpal, EHandKeypoint::RingTip);
		RenderFinger(InData, EHandKeypoint::LittleMetacarpal, EHandKeypoint::LittleTip);
	}

	FTransform BoneTransform;

	// Populate array data and add instances.
	for (int i = 0; i < InData.HandKeyPositions.Num(); i++)
	{
		BoneLocations[i] = InData.HandKeyPositions[i];
		BoneRotations[i] = InData.HandKeyRotations[i].Rotator();

		BoneTransform.SetLocation(InData.HandKeyPositions[i]);
		BoneTransform.SetRotation(InData.HandKeyRotations[i]);

		const float BoneScaleFactor = InData.HandKeyRadii[i] * BoneScale;
		FVector BoneScale3D = FVector(BoneScaleFactor, BoneScaleFactor, BoneScaleFactor);
		BoneTransform.SetScale3D(BoneScale3D);

		AddInstance(BoneTransform, true);
	}

	// Broadcast Pinch Events
	for (int i = 0; i < InputActions.Num(); i++)
	{
		if (InputActions[i] == nullptr) continue;
		const EFSOpenXRPinchFingers Finger = static_cast<EFSOpenXRPinchFingers>(i);
		const bool bPinching = IsPinching(Finger);
		OverrideInputWithAction(InputActions[i], bPinching ? 1.0f : 0.0f);
	}

	return true;
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

void UFSInstancedHand::RegisterInputAction(UInputAction* InInputAction, const EFSOpenXRPinchFingers Finger)
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

void UFSInstancedHand::UpdateHandPointer(const float DeltaTime)
{
	if (!bUpdateHandPointer || PointerContainer == nullptr) return;

	// Get the current pointer transform
	const FVector PointerLocation = PointerContainer->GetRelativeLocation();
	const FRotator PointerRotation = PointerContainer->GetRelativeRotation();

	// Get the target pointer transform and add an angle to the ray
	constexpr int PalmIndex = static_cast<int>(EHandKeypoint::Palm);
	const FVector PalmLocation = BoneLocations[PalmIndex];
	FRotator PalmRotation = BoneRotations[PalmIndex];
	PalmRotation.Pitch += HandPointerAngleFromPalm;

	// Move the container
	const FVector TargetLocation = FMath::Lerp(PointerLocation, PalmLocation, DeltaTime * HandPointerLocationSpeed);
	const FRotator TargetRotation = FMath::Lerp(PointerRotation, PalmRotation, DeltaTime * HandPointerRotationSpeed);
	PointerContainer->SetRelativeLocationAndRotation(TargetLocation, TargetRotation);
}
