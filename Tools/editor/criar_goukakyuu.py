"""Gera o material do novo Gōkakyū e configura a ficha existente do slot 1."""
import unreal

TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
FOLDER = "/Game/HypeWorld/Jutsus/Katon"
NAME = "M_Goukakyuu_FogoEmissivo"

material = unreal.load_asset(f"{FOLDER}/{NAME}") if unreal.EditorAssetLibrary.does_asset_exist(
    f"{FOLDER}/{NAME}"
) else TOOLS.create_asset(NAME, FOLDER, unreal.Material, unreal.MaterialFactoryNew())

MEL.delete_all_material_expressions(material)
material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property("two_sided", False)

def node(cls, x, y, **properties):
    expression = MEL.create_material_expression(material, cls, x, y)
    for key, value in properties.items():
        expression.set_editor_property(key, value)
    return expression

def link(source, destination, name):
    assert MEL.connect_material_expressions(source, "", destination, name), name

uv = node(unreal.MaterialExpressionTextureCoordinate, -700, -200)
rim = node(unreal.MaterialExpressionFresnel, -700, -60, exponent=1.0, base_reflect_fraction=0.0)
parameters = [uv, rim]
names = ["UV", "Rim"]
for i, (name, default) in enumerate(
    [("Heat", 1.0), ("Age", 0.0), ("Seed", 0.0)]
):
    parameters.append(node(unreal.MaterialExpressionScalarParameter, -700, 70 + i * 100,
                           parameter_name=name, default_value=default))
    names.append(name)

fire = node(unreal.MaterialExpressionCustom, -350, 0,
            output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3,
            description="Fogo do Gōkakyū | branco, amarelo, laranja, vermelho")
inputs = []
for name in names:
    port = unreal.CustomInput()
    port.set_editor_property("input_name", name)
    inputs.append(port)
fire.set_editor_property("inputs", inputs)
fire.set_editor_property("code", """
struct FireNoise
{
    float hash(float3 p)
    {
        p = frac(p * 0.3183099 + 0.1) * 17.0;
        return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
    }
    float smoothNoise(float3 p)
    {
        float3 i = floor(p);
        float3 t = frac(p);
        t *= t * (3.0 - 2.0 * t);
        float4 low = float4(hash(i), hash(i + float3(1,0,0)), hash(i + float3(0,1,0)), hash(i + float3(1,1,0)));
        float4 high = float4(hash(i + float3(0,0,1)), hash(i + float3(1,0,1)), hash(i + float3(0,1,1)), hash(i + 1));
        float4 z = lerp(low, high, t.z);
        return lerp(lerp(z.x, z.y, t.x), lerp(z.z, z.w, t.x), t.y);
    }
};
FireNoise n;
float face = saturate(1.0 - Rim);
float phi = UV.x * 6.283185;
float theta = UV.y * 3.141593;
float3 direction = float3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
float3 p = direction * float3(2.7, 4.1, 3.0) + float3(Age * 3.8, Seed, -Age * 1.6);
float warp = n.smoothNoise(p * 0.69);
float broad = n.smoothNoise(p + warp * 1.45);
float curls = n.smoothNoise(p * 2.25 - float3(Age * 1.1, 0.0, 0.0));
float flame = broad * 0.72 + curls * 0.28;
float temperature = saturate(flame * 0.92 + face * 0.44 - 0.16);
float3 red = float3(0.75, 0.014, 0.002);
float3 orange = float3(2.8, 0.40, 0.006);
float3 gold = float3(4.5, 2.2, 0.075);
float3 whiteGold = float3(8.0, 6.2, 2.3);
float3 color = lerp(red, orange, smoothstep(0.0, 0.40, temperature));
color = lerp(color, gold, smoothstep(0.33, 0.65, temperature));
color = lerp(color, whiteGold, smoothstep(0.59, 0.87, temperature));
return color * Heat;
""")
for source, name in zip(parameters, names):
    link(source, fire, name)

assert MEL.connect_material_property(fire, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(material)

jutsu = unreal.load_asset(f"{FOLDER}/DA_Katon_Goukakyuu")
pack = unreal.load_asset("/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Floor_01_Smoke")
assert jutsu and pack
assert unreal.EditorAssetLibrary.save_loaded_asset(material, False)
jutsu.set_editor_property("allow_air_cast", True)
jutsu.set_editor_property("goukakyuu_material", material)
jutsu.set_editor_property("goukakyuu_impact_system", pack)
print(f"GOUKAKYUU_CONFIG: seals={jutsu.get_editor_property('seals')} "
      f"speed={jutsu.get_editor_property('projectile_speed')} "
      f"range={jutsu.get_editor_property('projectile_range')}")
assert unreal.EditorAssetLibrary.save_loaded_asset(jutsu, False)
print("GOUKAKYUU_OK")
