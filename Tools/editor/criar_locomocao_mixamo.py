"""Cria BS_Ninja_Locomocao (cópia do BS_Idle_Walk_Run do template) com as animações do Mixamo
nas velocidades naturais delas, e troca as animações do ABP_Ninja.
Rodar com: python3 -I Tools/ue_exec.py -f Tools/editor/criar_locomocao_mixamo.py
"""
import unreal

ANIMS = "/Game/Ninja/Characters/Animations/Mixamo/"
BS_SRC = "/Game/Characters/Mannequins/Anims/Unarmed/BS_Idle_Walk_Run"
BS_DST = "/Game/Ninja/Characters/Animations/BS_Ninja_Locomocao"
ABP = "/Game/Ninja/Characters/Animations/ABP_Ninja"

IDLE = unreal.load_asset("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle")   # idle neutra (teste: Warrior Idle torce o tronco; para voltar: ANIMS + "A_Ninja_MX_Avulsa_Warrior_Idle")
W2 = {n: unreal.load_asset(ANIMS + "A_Ninja_MX_Avulsa_Walk2_" + n) for n in ("Forward", "Left", "Right", "Backward")}
R2 = {n: unreal.load_asset(ANIMS + "A_Ninja_MX_Avulsa_Run2_" + n) for n in ("Forward", "Left", "Right")}
# Cada amostra na velocidade natural da animação (deslocamento do root ÷ duração). Direção: 0 frente, -90 esquerda, 90 direita.
# Correndo para trás o corpo vira e corre de frente (não existe "correr de costas").
# Acima da velocidade de cada amostra o código acelera a animação (RunStrafe*Natural / AnimSpeedCurve no personagem).
EXTRA = [(0.0, 169.0, W2["Forward"]), (-90.0, 164.0, W2["Left"]), (90.0, 164.0, W2["Right"]),
         (180.0, 107.0, W2["Backward"]), (-180.0, 107.0, W2["Backward"]),
         (0.0, 560.0, R2["Forward"]), (-90.0, 428.0, R2["Left"]), (90.0, 342.0, R2["Right"])]
ROWS = [(0.0, IDLE)]   # linha completa (todas as direções) só para o parado

# --- Blend space: mesmo layout do template (direção x velocidade), refeito com 4 faixas ---
if not unreal.EditorAssetLibrary.does_asset_exist(BS_DST):
    unreal.EditorAssetLibrary.duplicate_asset(BS_SRC, BS_DST)
bs = unreal.load_asset(BS_DST)

params = list(bs.get_editor_property("blend_parameters"))
params[1].set_editor_property("max", 560.0)
params[1].set_editor_property("grid_num", 112)          # grade de 5 cm/s: as velocidades ficam exatas
for prm in params[:2]:
    prm.set_editor_property("snap_to_grid", False)
bs.set_editor_property("blend_parameters", params)

directions = sorted({round(s.get_editor_property("sample_value").x) for s in bs.get_editor_property("sample_data")})
samples = []
for speed, anim in ROWS:
    for d in directions:
        sample = unreal.BlendSample()
        sample.set_editor_property("animation", anim)
        sample.set_editor_property("sample_value", unreal.Vector(float(d), speed, 0.0))
        samples.append(sample)
for d, speed, anim in EXTRA:
    sample = unreal.BlendSample()
    sample.set_editor_property("animation", anim)
    sample.set_editor_property("sample_value", unreal.Vector(d, speed, 0.0))
    samples.append(sample)
bs.set_editor_property("sample_data", samples)
print("triangulação refeita:", unreal.NinjaEditorTools.rebuild_blend_space(bs))   # sem isso o blend space devolve pose parada
unreal.EditorAssetLibrary.save_asset(BS_DST, only_if_is_dirty=False)
print("blend space:", sorted({(round(s.get_editor_property("sample_value").y), s.get_editor_property("animation").get_name()) for s in bs.get_editor_property("sample_data")}), len(samples), "amostras")

# --- ABP_Ninja: troca as animações dos nós (exporta o nó como texto, troca o caminho, importa de volta) ---
BASE = ABP + ".ABP_Ninja:AnimGraph."
NODES = {
    "AnimGraphNode_StateMachine_0.Locomotion.AnimStateNode_1.Idle.AnimGraphNode_SequencePlayer_1": ("Sequence", IDLE),
    "AnimGraphNode_StateMachine_0.Locomotion.AnimStateNode_2.Walk / Run.AnimGraphNode_BlendSpacePlayer_0": ("BlendSpace", bs),
    "AnimGraphNode_StateMachine_1.Main States.AnimStateNode_1.Jump.AnimGraphNode_SequencePlayer_1": ("Sequence", unreal.load_asset(ANIMS + "A_Ninja_MX_Jumping_Up")),
    "AnimGraphNode_StateMachine_1.Main States.AnimStateNode_2.Fall Loop.AnimGraphNode_SequencePlayer_3": ("Sequence", unreal.load_asset(ANIMS + "A_Ninja_MX_Falling_Idle")),
}
import re
for path, (field, asset) in NODES.items():
    graph_node = unreal.find_object(None, BASE + path)
    node = graph_node.get_editor_property("node")
    cls = "AnimSequence" if field == "Sequence" else "BlendSpace"
    new_text, n = re.subn(field + r'="[^"]*"', '%s="/Script/Engine.%s\'%s\'"' % (field, cls, asset.get_path_name()), node.export_text(), count=1)
    assert n == 1, path
    node.import_text(new_text)
    graph_node.set_editor_property("node", node)
    print("trocado:", path.split(".")[-2], "->", re.search(field + r'="[^"]*"', graph_node.get_editor_property("node").export_text()).group(0).split("/")[-1])

abp = unreal.load_asset(ABP)
unreal.BlueprintEditorLibrary.compile_blueprint(abp)
print("ABP salvo:", unreal.EditorAssetLibrary.save_asset(ABP, only_if_is_dirty=False))
