"""Aplica os ícones Katon e o layout compacto da hotbar. Executar no Unreal Editor."""
from pathlib import Path
import unreal

project = Path(unreal.Paths.project_dir()).resolve()
art = project / "SourceAssets" / "UI" / "Hotbar"
material_script = project / "Tools" / "editor" / "criar_material_slot_anel.py"
exec(compile(material_script.read_text(), str(material_script), "exec"), {})

katon = "/Game/HypeWorld/Jutsus/Katon"
tools = unreal.AssetToolsHelpers.get_asset_tools()
packages = []
for name, folder in (("Goukakyuu", ""), ("Endan", "/Endan"), ("Hosenka", "/Hosenka")):
    texture_name = f"T_JS_{name}_Paint"
    task = unreal.AssetImportTask()
    task.filename = str(art / f"{texture_name}.png")
    task.destination_path = f"{katon}{folder}/UI"
    task.destination_name = texture_name
    task.automated = True
    task.replace_existing = True
    tools.import_asset_tasks([task])
    imported = task.get_objects()
    if len(imported) != 1 or not isinstance(imported[0], unreal.Texture2D):
        raise RuntimeError(f"Falha ao importar {texture_name}")
    texture = imported[0]
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
    texture.set_editor_property("srgb", True)
    asset = unreal.load_asset(f"{katon}{folder}/DA_Katon_{name}")
    asset.set_editor_property("icon", texture)
    packages.extend((texture.get_outermost(), asset.get_outermost()))

bp = unreal.load_asset("/Game/Ninja/UI/WBP_NinjaHUD")
cdo = unreal.get_default_object(bp.generated_class())
cdo.modify()
cdo.set_editor_property("jutsu_slot_size", 54.0)
cdo.set_editor_property("jutsu_slot_gap", 10.0)
cdo.set_editor_property("jutsu_hidden_offset", 80.0)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
packages.append(bp.get_outermost())
if not unreal.EditorLoadingAndSavingUtils.save_packages(packages, False):
    raise RuntimeError("Não foi possível salvar os ícones e o HUD")
print("HOTBAR: aro, três ícones Katon e layout compacto salvos")
