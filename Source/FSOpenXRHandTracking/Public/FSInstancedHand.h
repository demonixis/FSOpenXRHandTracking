// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HeadMountedDisplayTypes.h"
#include "InputAction.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "FSInstancedHand.generated.h"

UENUM(BlueprintType)
enum class EFSOpenXRPinchFingers : uint8
{
	Index UMETA(DisplayName="Index"),
	Middle UMETA(DisplayName="Middle"),
	Ring UMETA(DisplayName="Ring"),
	Little UMETA(DisplayName="Little")
};

UENUM(BlueprintType)
enum class EFSOpenXRHandRendering : uint8
{
	InstancedMesh UMETA(DisplayName="InstancedMesh"),
	Wireframe UMETA(DisplayName="Wireframe"),
	Both UMETA(DisplayName="Both")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHandTrackingEnabledDelegate, bool, bLeft, bool, bEnabled);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FSOPENXRHANDTRACKING_API UFSInstancedHand : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()

	bool bHandTracked;
	bool bPreviousHandTracked;
	FTransform CurrentHandTransform;

	UPROPERTY()
	TArray<FVector> BoneLocations;
	UPROPERTY()
	TArray<FRotator> BoneRotations;
	UPROPERTY()
	TArray<FRotator> BoneRelativeRotations;
	UPROPERTY()
	TArray<UInputAction*> InputActions;

public:
	// Settings
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FSOpenXRHandTracking|Settings")
	bool bHideHand;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FSOpenXRHandTracking|Settings")
	bool bOnlyDisplayTips;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FSOpenXRHandTracking|Settings")
	float BoneScale;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FSOpenXRHandTracking|Settings")
	bool bLeftHand;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FSOpenXRHandTracking|Settings")
	float PinchThreshold;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FSOpenXRHandTracking|Settings")
	FTransform FallbackTransform;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FSOpenXRHandTracking|Settings")
	bool bComputeRelativeRotations;
	
	// Rendering
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FSOpenXRHandTracking|Rendering")
	EFSOpenXRHandRendering HandRendering;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FSOpenXRHandTracking|Rendering")
	FColor WireframeColor;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FSOpenXRHandTracking|Rendering")
	float WireframeThickness;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FSOpenXRHandTracking|Rendering")
	bool bRenderWireframePalm;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FSOpenXRHandTracking|Rendering")
	bool bRenderWireframeBones;

	// Hand Pointer
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FSOpenXRHandTracking|Hand Pointer")
	bool bUpdateHandPointer;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FSOpenXRHandTracking|Hand Pointer")
	USceneComponent* PointerContainer;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FSOpenXRHandTracking|Hand Pointer")
	float HandPointerAngleFromPalm;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FSOpenXRHandTracking|Hand Pointer")
	bool bHideHandPointerWhenNotTracked;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FSOpenXRHandTracking|Hand Pointer")
	float HandPointerLocationSpeed;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FSOpenXRHandTracking|Hand Pointer")
	float HandPointerRotationSpeed;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FSOpenXRHandTracking|Hand Pointer")
	int HandPointerDepth;

	// Events
	UPROPERTY(VisibleAnywhere, BlueprintAssignable)
	FHandTrackingEnabledDelegate HandTrackingEnableChanged;

	UFSInstancedHand();

	// Public & Blueprint functions
	UFUNCTION(BlueprintCallable, Category="FSOpenXRHandTracking|Blueprint")
	FTransform GetHandTransform() const;
	UFUNCTION(BlueprintCallable, Category="FSOpenXRHandTracking|Blueprint")
	bool UpdateHand(const FXRMotionControllerData& InData, const float DeltaTime);
	UFUNCTION(BlueprintCallable, Category="FSOpenXRHandTracking|Blueprint")
	static void GetDataFromSkeleton(UPoseableMeshComponent* Target, const bool bLeft, FXRMotionControllerData& OutData);
	UFUNCTION(BlueprintCallable, Category="FSOpenXRHandTracking|Blueprint")
	bool IsHandTracked() const;
	UFUNCTION(BlueprintCallable, Category="FSOpenXRHandTracking|Blueprint")
	bool IsPinching(const EFSOpenXRPinchFingers Finger) const;
	UFUNCTION(BlueprintCallable, Category="FSOpenXRHandTracking|Blueprint")
	void RegisterInputAction(const EFSOpenXRPinchFingers Finger, UInputAction* InInputAction);
	UFUNCTION(BlueprintCallable, Category="FSOpenXRHandTracking|Blueprint")
	void RegisterHandRay(USceneComponent* InRayContainer);
	UFUNCTION(BlueprintCallable, Category="FSOpenXRHandTracking|Blueprint")
	FRotator GetBoneRotation(const EHandKeypoint Keypoint) const;
	UFUNCTION(BlueprintCallable, Category="FSOpenXRHandTracking|Blueprint")
	FVector GetBoneLocation(const EHandKeypoint Keypoint) const;
	UFUNCTION(BlueprintCallable, Category="FSOpenXRHandTracking|Blueprint")
	FRotator GetBoneRelativeRotation(const EHandKeypoint Keypoint) const;
	
private:
	void RenderFinger(const FXRMotionControllerData& InData, const EHandKeypoint FingerStart,
	                  const EHandKeypoint FingerEnd) const;
	void OverrideInputWithAction(const UInputAction* InInputAction, const float Value) const;
	static int32 GetParentIndex(EHandKeypoint Keypoint);

	static uint8 GetOculusBone(EHandKeypoint Keypoint);
};
