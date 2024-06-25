# OpenXR Hand Tracking for Unreal Engine 5

This is a plugin that allows you to use hand tracking using the OpenXR plugin and the Hand Tracking extension.
This plugin was tested on Meta Quest 3, but must work on any other device that supports the Hand Tracking extension.

## Requirements
- Current Unreal Engine version: 5.4.x

## Features
- Basic hand rendering with many options
- Pinch detection (optional)
- Enhanced Input Support (optional)
- Hand Ray support (optional)
- MetaXR bridge (Experimental)

## Getting started
You've to add an `UFSInstancedHand` component parented to your `VROrigin` component and select weither it's a `left` or `right` hand. The `UFSInstancedHand` component inherits from `UInstancedStaticMeshComponent`, that means the hand is rendered into one draw call. You can assign a material for each hand, or keep the default material. You can use this plugin in a C++ and Blueprint projects, all function are exposed to Blueprint.
The final step is to call the `UpdateHand(const FXRMotionControllerData& InData, const float DeltaTime)` function in the `Tick` function. 

### MetaXR Support
When MetaXR plugin is enabled, Hand Tracking data are not valid and the plugin can't work. That's why there is a function called `GetDataFromSkeleton(UPoseableMeshComponent* Target, const bool bLeft, FXRMotionControllerData& OutData)`, that allows you to retrieve a valid `FXRMotionControllerData`.

You've to add the OculusXRHands to your Pawn, then pass each hand to the function. You can use this function in both Blueprint and C++, here is a C++ example:

```cpp
FXRMotionControllerData HandData;

UHeadMountedDisplayFunctionLibrary::GetMotionControllerData(this, EControllerHand::Left, HandData);

#if WITH_METAXR
UFSInstancedHand::GetDataFromSkeleton(LeftHandSkeleton, true, HandData);
#endif

LeftHandTracking->UpdateHand(HandData, DeltaSeconds);
```

Finally, you've to enable MetaXR support by passing the `bMetaXREnabled` private var to `true` in `FSOpenXRHandTracking.Build.cs`.

### Hand rendering
 Here are the supported rendering modes:
- `EFSOpenXRHandRendering::InstancedMesh` displays a mesh on each bone
- `EFSOpenXRHandRendering::Wireframe` displays wireframe
- `EFSOpenXRHandRendering::Both` (default), displays both meshes and wireframe

#### Basic settings
| Parameter | Description | Default |
|-----------|-------------|---------|
| `BoneScale` | Size of a rendered bone | `0.015f` |
| `bLeftHand` | Set to `true` for the left hand | `false` |
| `PinchThreshold` | Pinch detection threshold | `1.5f` |
| `bHideHand` | Hide 3D hands but keeps the logic working (pinch/gestures) | `False` |
| `bOnlyDisplayTups` | Only display tip bones | `False` |
| `bComputeRelativeRotations` | Compute relative rotations, for use with gesture recognizer for instance | `False` |

#### Wireframe settings
| Parameter | Description | Default |
|-----------|-------------|---------|
| `HandRendering` | Hand rendering mode | `Both` |
| `WireframeColor` | Wireframe color | `FColor::Blue` |
| `WireframeThickness` | Wireframe Thickness | `0.5f ` |
| `bRenderWireframePalm` | Render palm wireframe | `false` |
| `bRenderWireframeBones` | Render wireframe bones | `false` |
| `HandPointerDepth` | Pointer Depth when rendering wireframe | `1.0f` |

### Pinch detection & Enhanced Input System
You can check using the `IsPinching(const EFSOpenXRPinchFingers Finger)` function if a finger is pinching or not.
It's also possible to use the Enhanced Input System to trigger an `UInputAction` during a finger pinch. Check the `RegisterInputAction(UInputAction* InInputAction, const EFSOpenXRPinchFingers Finger)` function. You can control the pinch detection threshold using the `PinchThreshold` parameter.

### Hand Ray follower
You can setup a hand ray (for instance a scaled cylinder and a `UWidgetInteractionComponent`). The system will move the hand ray at the correct location and rotation, adding an angle to the ray and some lag. The lag is required because the hand is constantly moving and you can't interact easily with UI elements without that. To enable this feature, you've to first register a `USceneComponent` node, that will be moved using the `RegisterHandRay(USceneComponent* InRayContainer)`

#### Hand Ray settings
| Parameter | Description | Default |
|-----------|-------------|---------|
| `HandPointerAngleFromPalm` | The angle applied from the palm to the Ray | `-45.0f` |
| `bHideHandPointerWhenNotTracked` | Hide hand ray if hands are not tracked | `false` |
| `HandPointerLocationSpeed` | Ray movement speed | `8.0f` |
| `HandPointerRotationSpeed` | Ray rotation speed | `2.0f` |

## What's planned
- Basic Gesture detector
- Enhanced Input System support for the gesture detector
- Skeletal mesh support

## Contribution
Feel free to fork and contribute :)

## License
This project is released under the MIT license. Please read the `LICENSE.md` file for more informations.
