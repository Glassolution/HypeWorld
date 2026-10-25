"""Apaga os bonecos temporários da folha de quadros (os que estão no céu, z > 15000)."""
import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
gone = [a for a in eas.get_all_level_actors() if a.get_actor_location().z > 15000 and isinstance(a, (unreal.SkeletalMeshActor, unreal.TextRenderActor))]
for a in gone:
    eas.destroy_actor(a)
print("removidos:", len(gone))
