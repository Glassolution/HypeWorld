"""Importa os modelos da floresta (Desktop/Hype World/Floresta/FBX, feitos por Tools/floresta/gerar_assets_floresta.py) em
/Game/Ninja/Ambiente/Floresta/Meshes, liga os materiais (criar_materiais_floresta.py) pelo nome do slot e configura:
- LODs automáticos (árvores, arbustos e pedras: 3 níveis; grama e flores: 2) — mais leve de longe
- colisão: árvores e pedras = a própria malha (dá para subir/pular nelas); grama, flores e arbustos = sem colisão
Rodar com ue_exec -f, editor fora do Play.
"""
import os
import unreal

ORIGEM = "/Users/luisfelipexavier/Desktop/Hype World/Floresta/FBX"
DESTINO = "/Game/Ninja/Ambiente/Floresta/Meshes"
MATS = "/Game/Ninja/Ambiente/Floresta/Materiais"
tools = unreal.AssetToolsHelpers.get_asset_tools()
sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
eal = unreal.EditorAssetLibrary

SEM_COLISAO = ("SM_Grama", "SM_Flor", "SM_Arbusto")
LODS_3 = ("SM_Arvore", "SM_Pedra", "SM_Arbusto")

for arq in sorted(os.listdir(ORIGEM)):
    if not arq.endswith(".fbx"):
        continue
    nome = arq[:-4]
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", False)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("import_animations", False)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    sm = ui.get_editor_property("static_mesh_import_data")
    sm.set_editor_property("combine_meshes", True)
    sm.set_editor_property("auto_generate_collision", False)
    sm.set_editor_property("vertex_color_import_option", unreal.VertexColorImportOption.REPLACE)
    sm.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    sm.set_editor_property("generate_lightmap_u_vs", False)
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", "%s/%s" % (ORIGEM, arq))
    t.set_editor_property("destination_path", DESTINO)
    t.set_editor_property("destination_name", nome)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", False)
    t.set_editor_property("options", ui)
    tools.import_asset_tasks([t])
    mesh = unreal.load_asset("%s/%s" % (DESTINO, nome))
    if not mesh:
        print("FALHOU", nome)
        continue
    # materiais pelo nome do slot (o slot vem com o nome do material do Blender)
    mats = mesh.get_editor_property("static_materials")
    for i, m in enumerate(mats):
        slot = str(m.get_editor_property("material_slot_name")).split(".")[0]
        mat = unreal.load_asset("%s/%s" % (MATS, slot))
        if mat:
            mesh.set_material(i, mat)
        else:
            print("sem material para o slot", slot, "em", nome)
    # LODs
    if nome.startswith(LODS_3):
        nivel = [(1.0, 1.0), (0.45, 0.35), (0.18, 0.12)]
    else:
        nivel = [(1.0, 1.0), (0.5, 0.15)]
    opcoes = unreal.StaticMeshReductionOptions()
    opcoes.set_editor_property("reduction_settings", [unreal.StaticMeshReductionSettings(percent_triangles=p, screen_size=s) for p, s in nivel])
    opcoes.set_editor_property("auto_compute_lod_screen_size", False)
    sme.set_lods(mesh, opcoes)
    # colisão
    body = mesh.get_editor_property("body_setup")
    if nome.startswith(SEM_COLISAO):
        if body:
            body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)
        sme.remove_collisions(mesh)
    elif body:
        body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    eal.save_loaded_asset(mesh, False)
    b = mesh.get_bounding_box()
    print("OK", nome, "LODs", sme.get_lod_count(mesh), "tam (m)", [round(v / 100.0, 1) for v in (b.max.x - b.min.x, b.max.y - b.min.y, b.max.z - b.min.z)],
          "base z", round(b.min.z, 1))
