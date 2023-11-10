# OpenXR Hand Tracking for Unreal Engine 5

This is a plugin that allows you to use hand tracking using the OpenXR plugin and the Hand Tracking extension.
This plugin was tested on Meta Quest 3, but must work on any other device that supports the Hand Tracking extension.

## Features
- Basic hand rendering with many options
- Pinch detection
- Enhanced Input Support (optional)
- Hand Ray support (optional)

## Getting started
You've to add an `UFSInstancedHand` component parented to your `VROrigin` component and select weither it's a `left` or `right` hand. The `UFSInstancedHand` component inherits from `UInstancedStaticMeshComponent`, that means the hand is rendered into one draw call. You can assign a material for each hand, or keep the default material. You can use this plugin in a C++ and Blueprint projects, all function are exposed to Blueprint.
The final step is to call the `UpdateHand(const FXRMotionControllerData& InData, const float DeltaTime)` function in the `Tick` function. 

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

#### Wireframe settings
| Parameter | Description | Default |
|-----------|-------------|---------|
| `WireframeColor` | Wireframe color | `FColor::Blue` |
| `WireframeThickness` | Wireframe Thickness | `0.5f ` |
| `bRenderWireframePalm` | Render palm wireframe | `false` |
| `bRenderWireframeBones` | Render wireframe bones | `false` |

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
