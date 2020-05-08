# QuestHands
An Unreal Engine 4 plugin for the new Oculus Quest hands tracking. To tied us over till Oculus's official release.

This plugin version requires version 16 of the Oculus Plugin so you need to use the Oculus branch of the Unreal Engine for this plugin to work.

The Oculus branch can be found here: https://github.com/Oculus-VR/UnrealEngine/tree/oculus-4.24.3-release-1.48.0-v16.0

For other versions of the plugin see the branches of this repository.

## Implementation Details
- Full hand tracking from API
- Hand bones setup
- Hand mesh deforming using bone positions
- Hand collision capsules

## Example Pawn
There is an example pawn setup with a number of features consistent with those found on the Oculus Dashboard hands such as:
- Interaction laser pointers
- Pinch cone markers with color and scaling based on pinch
- Widget Interaction Components
- Color changes based on System state
- Outlined hands
It also contains an example of using your own PoseableMeshComponent. The pawn can be found in the plugins content directory and is called BP_QuestHandsPawn

## Usage
Your projects Engine ini should contain a section called [/Script/AndroidRuntimeSettings.AndroidRuntimeSettings] with at least two additions. Here is an example:

```
[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]
+PackageForOculusMobile=Quest
bSupportQuestHandsTracking=True
```

Note **bSupportQuestHandsTracking** bool. If the QuestHands plugin is enabled this is true by default. Setting it to false will disable the important android manifest options which lets the app have hands tracking.

If you want a configuration which works well for the Quest you can use this projects configuration as a starting point.

The plugin contains a function library for querying the Oculus hand tracking API directly OR you can use the supplied QuestHands component to do all the work.

The component is called **QuestHandsComponent** and just has to be assigned to your pawn to function. 

By default it will create the PoseableMeshComponents for the hands and deform them and move them to coincide with the hand tracking data. If you would like to use custom hand meshes you can do so by either choosing different meshes in the mesh chooser under the QuestHandsComponent details panel OR adding two PoseableMeshComponent components parented to the QuestHandsComponent, giving them names and assigning those names in the QuestHandsComponent details panel. With the latter option you can control what is parented to the hands which can be useful for adding particle FX etc. The QuestHandsComponent will only look for already placed PoseableMeshComponents if the bool **CreateHandMeshComponents** is set to false.

For an example of a basic implementation into a pawn actor check out the example pawn blueprint under the plugins content directory called **BP_QuestHandsPawn**

To implement the plugin into your own project copy the folder QuestHands from this repos Plugins directory to your own project and activate the plugin.

![Alt text](/Screenshots/quest_hands_ue4.gif?raw=true "Oculus Quest Hand Tracking in UE4!")

## Future Enhancements
- Integrate the tracking hand bone positions as a blueprint animation graph node so it can be part of the animation composition. This will open the door to add extra additive enhancements such as finger IK which so far in the Unity projects has looked very promising.

## License
[MIT](https://choosealicense.com/licenses/mit/)
