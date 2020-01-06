# QuestHands
An Unreal Engine 4 plugin for the new Oculus Quest hands tracking. To tied us over till Oculus's official release.

This plugin requires version 12 of the Oculus Plugin so you need to use the Oculus branch of the Unreal Engine for this plugin to work.

The Oculus branch can be found here: https://github.com/Oculus-VR/UnrealEngine/tree/oculus-4.24.1-release-1.44.0-v12.0

## Implementation Details
- Full hand tracking from API
- Hand bones setup
- Hand mesh deforming using bone positions
- Hand collision capsules

## Usage
The plugin contains a function library for querying the Oculus hand tracking API directly OR you can use the supplied QuestHands component to do all the work.

The component is called **QuestHandsComponent** and just has to be assigned to your pawn to function. 

By default it will create the PoseableMeshComponents for the hands and deform them and move them to coincide with the hand tracking data. If you would like to use custom hand meshes you can do so by either choosing different meshes in the mesh chooser under the QuestHandsComponent details panel OR adding two PoseableMeshComponent components parented to the QuestHandsComponent, giving them names and assigning those names in the QuestHandsComponent details panel. With the latter option you can control what is parented to the hands which can be useful for adding particle FX etc. The QuestHandsComponent will only look for already placed PoseableMeshComponents if the bool **CreateHandMeshComponents** is set to false.

For an example of a basic implementation into a pawn actor check out the example pawn blueprint under the plugins content directory called **BP_QuestHandsPawn**

![Alt text](/Screenshots/quest_hands_ue4.gif?raw=true "Oculus Quest Hand Tracking in UE4!")
