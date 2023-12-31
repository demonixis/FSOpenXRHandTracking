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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHandTrackingEnabledDelegate, bool, bEnabled);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FSOPENXRHANDTRACKING_API UFSInstancedHand : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()

	bool bHandTracked;
	bool bPreviousHandTracked;

	UPROPERTY()
	TArray<FVector> BoneLocations;
	UPROPERTY()
	TArray<FRotator> BoneRotations;
	UPROPERTY()
	TArray<UInputAction*> InputActions;

public:
	// Settings
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FSOpenXRHandTracking|Settings")
	float BoneScale;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FSOpenXRHandTracking|Settings")
	bool bLeftHand;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FSOpenXRHandTracking|Settings")
	float PinchThreshold;

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
	bool UpdateHand(const FXRMotionControllerData& InData, const float DeltaTime);
	UFUNCTION(BlueprintCallable, Category="FSOpenXRHandTracking|Blueprint")
	bool IsHandTracked() const;
	UFUNCTION(BlueprintCallable, Category="FSOpenXRHandTracking|Blueprint")
	bool IsPinching(const EFSOpenXRPinchFingers Finger) const;
	UFUNCTION(BlueprintCallable, Category="FSOpenXRHandTracking|Blueprint")
	void RegisterInputAction(UInputAction* InInputAction, const EFSOpenXRPinchFingers Finger);
	UFUNCTION(BlueprintCallable, Category="FSOpenXRHandTracking|Blueprint")
	void RegisterHandRay(USceneComponent* InRayContainer);
	UFUNCTION(BlueprintCallable, Category="FSOpenXRHandTracking|Blueprint")
	FRotator GetBoneRotation(const EHandKeypoint Keypoint) const;
	
private:
	void RenderFinger(const FXRMotionControllerData& InData, const EHandKeypoint FingerStart,
	                  const EHandKeypoint FingerEnd) const;
	void OverrideInputWithAction(const UInputAction* InInputAction, const float Value) const;
};
