from audit import *
import shutil
assert (OUT/'twist_detached.json').exists()
assert (OUT/'BackupAntes').is_dir()
paths=['Content/Ninja/Characters/Animations/ABP_Ninja.uasset']
for p in paths:
    data=content('f091ba9',p);assert data
    (ROOT/p).write_bytes(data)
removed=['Source/HypeWorld/Ninja/NinjaBodyAnimInstance.cpp','Source/HypeWorld/Ninja/NinjaBodyAnimInstance.h','Content/HypeWorld/Animacoes/A_Pose_Prontidao_Base.uasset']
for p in removed:
    assert (OUT/'BackupAntes'/p).read_bytes()==(ROOT/p).read_bytes()
    (ROOT/p).unlink()
# Reconnect readiness only above spine_01, after the original battle idle, including at rest.
base=content('f091ba9','Source/HypeWorld/Ninja/NinjaAnimInstance.cpp').decode()
before='\t\tBattleIdle = Ninja->ActiveBattleIdle ? Ninja->ActiveBattleIdle.Get() : Ninja->BattleIdlePose;\n\t\tCombatPose = Ninja->ActiveBattleIdle ? Ninja->ActiveBattleIdle.Get() : Ninja->BattleIdlePose;'
after='\t\tUAnimSequenceBase* StyleIdle = Ninja->ActiveBattleIdle ? Ninja->ActiveBattleIdle.Get() : Ninja->BattleIdlePose;\n\t\tBattleIdle = StyleIdle;\n\t\tCombatPose = Ninja->CombatReadyPose ? Ninja->CombatReadyPose : StyleIdle;'
assert before in base
restored=base.replace(before,after)
restored=restored.replace('\t\tconst float MovingAlpha = FMath::SmoothStep(20.f, 160.f, float(Char->GetVelocity().Size2D()));\n\t\tconst float Target = bCombatRun ? MovingAlpha * (1.f - FMath::Clamp(GetSlotMontageGlobalWeight(FName("DefaultSlot")), 0.f, 1.f)) : 0.f;', '\t\t// A prontidão cobre somente o tronco, inclusive na guarda parada; as pernas mantêm a base do estilo.\n\t\tconst float Target = bCombatRun ? 1.f - FMath::Clamp(GetSlotMontageGlobalWeight(FName("DefaultSlot")), 0.f, 1.f) : 0.f;')
restored=restored.replace('\t\tApplyCombatReady(Output);\n\t\tApplyBattleIdle(Output);','\t\tApplyBattleIdle(Output);\n\t\tApplyCombatReady(Output);')
(ROOT/'Source/HypeWorld/Ninja/NinjaAnimInstance.cpp').write_text(restored,encoding='utf-8')
(OUT/'restored.json').write_text(json.dumps({'baseline':'f091ba9','restored':paths,'removed_with_backup':removed,'cpp_change':'Reconnect original CombatReadyPose for battle idle and combat trot; all other code from f091ba9.'},indent=2))
print('RESTORED',paths,'REMOVED_WITH_BACKUP',removed)
