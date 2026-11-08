"""Gōkakyū (07/11, 6ª especificação): o fogo NASCE NA BOCA no CUSPE e se projeta para a FRENTE (sopro), sem aura.

- Notify "Katon_Spit" (classe C++ UAnimNotify_KatonSpit) no sopro A_Katon_Sopro_Sasuke, no instante em que a cabeça
  começa a ir para a frente (medido pelo osso bone_head: recua 0-0,2 s, projeta 0,275-0,4 s). É ele que solta o jutsu:
  efeito, luz e projétil nascem só aí (o NinjaJutsuRelease da mesma animação passa a ser ignorado). Clique = só selos.
- NS_Katon_Gokakyu_Sopro (vira a "saída" do DA: nasce parado na boca, X = mira). Tempos a partir do cuspe:
    0-0,1     Ignicao: ponto branco-amarelo forte na boca
    0,06-0,35 Sopro + Linguas: chamas num cone de ~28° para a frente, freando (drag) e formando a bola oval na frente
              Feixes: 3 raios curtos para a frente (0,12-0,3); Brasas; Halo aditivo na frente
  Tudo no hemisfério da frente: nada em volta ou atrás do corpo.
- O projétil fica preso à frente da boca por GokakyuHoldTime (0,35 s, sem colisão/dano) e então dispara como cometa
  (jato/cauda de antes).
Rodar depois do gokakyu_4_jato.py e do gokakyu_3_ligar.py. Editor aberto, fora do Play.
"""
import time
import unreal

EAL = unreal.EditorAssetLibrary
NT = unreal.NinjaEditorTools
AL = unreal.AnimationLibrary
FOLDER = "/Game/HypeWorld/Jutsus/Katon/VFX/Gokakyu"
DA = "/Game/HypeWorld/Jutsus/Katon/DA_Katon_Goukakyuu"
BP = "/Game/HypeWorld/Jutsus/Katon/BP_Katon_Goukakyuu"
T_FOUNTAIN = "/Niagara/DefaultAssets/Templates/Emitters/Fountain"
SELF, ONCE = "NewEnumerator1", "NewEnumerator2"
CUSPE = 0.27        # s no sopro (bone_head começa a ir para a frente em ~0,275 s)
HOLD = 0.35         # s do cuspe até a bola disparar

# ---------------------------------------------------------------- notify Katon_Spit na animação do sopro
da = unreal.load_asset(DA)
anim = da.get_editor_property("body_cast_animation")
spit_cls = unreal.AnimNotify_KatonSpit.static_class()
ja = [ev for ev in AL.get_animation_notify_events(anim) if str(ev.get_editor_property("notify_name")) == "Katon_Spit"]
if not ja:
    tracks = AL.get_animation_notify_track_names(anim)
    track = tracks[0] if tracks else "1"
    if not tracks:
        AL.add_animation_notify_track(anim, track)
    assert AL.add_animation_notify_event(anim, track, CUSPE, spit_cls), "notify"
    EAL.save_loaded_asset(anim, False)
for ev in AL.get_animation_notify_events(anim):
    print("notify", ev.get_editor_property("notify_name"), AL.get_anim_notify_event_trigger_time(ev))

# ---------------------------------------------------------------- Niagara do sopro
MI = lambda n: f"{FOLDER}/{n}.{n}"


def const(s, em, param, value, required=True):
    v = value if isinstance(value, str) else ",".join(map(str, value)) if isinstance(value, (tuple, list)) else str(value)
    n = NT.set_niagara_constant(s, em, param, v)
    if n <= 0:
        assert not required, f"{em}.{param}: {n}"
        print("  (sem constante)", em, param)


def rend(s, em, prop, value):
    assert NT.set_niagara_renderer_property(s, em, 0, prop, value), f"{em}.{prop}"


# axis (1,0,0) = direção do disparo; offset/raio pequenos na boca; cones <= ~30°
CFG = {
    "Ignicao": dict(fase=(0.0, 0.1), rate=70, life=(0.1, 0.15), size=(90, 150), speed=(0, 1), cone=0, axis=(1, 0, 0),
                    radius=6, offset=(15, 0, 0), rise=(0, 0, 0), drag=0, mat=MI("MI_FireJato_Miolo"), sort=6),
    # (1ª versão com MI_FireJato_Cabeca + miolo 140-240 + aura de centro branco = bola quase toda branca)
    # (260/s com 180-340 cm = +4 ms de transparência na câmera do jogo; 140/s = ~metade)
    "Sopro": dict(fase=(0.06, 0.29), rate=105, life=(0.28, 0.42), size=(150, 290), speed=(500, 1100), cone=28,
                  axis=(1, 0, 0), radius=15, offset=(20, 0, 0), rise=(0, 0, 120), drag=4.0, mat=MI("MI_FireJato_Cauda"),
                  sort=2),
    "Linguas": dict(fase=(0.1, 0.25), rate=35, life=(0.25, 0.4), size=(120, 220), speed=(700, 1300), cone=30,
                    axis=(1, 0, 0), radius=15, offset=(20, 0, 0), rise=(0, 0, 80), drag=3.0, stretch=(0.5, 1.6),
                    mat=MI("MI_FireJato_Lingua"), sort=3),
    "Miolo": dict(fase=(0.06, 0.29), rate=25, life=(0.12, 0.2), size=(90, 160), speed=(250, 450), cone=10,
                  axis=(1, 0, 0), radius=10, offset=(30, 0, 0), rise=(0, 0, 0), drag=4.0, mat=MI("MI_FireJato_Miolo"),
                  sort=5),
    "Feixes": dict(fase=(0.12, 0.18), rate=17, life=(0.15, 0.22), size=(1, 1), sprite=((40, 700), (70, 1100)),
                   speed=(30, 60), cone=4, axis=(1, 0, 0), radius=25, offset=(450, 0, 0), rise=(0, 0, 0), drag=10.0,
                   mat=MI("MI_FireJato_Feixe"), sort=4),
    "Brasas": dict(fase=(0.05, 0.3), rate=60, life=(0.4, 0.7), size=(6, 14), speed=(400, 900), cone=30, axis=(1, 0, 0.1),
                   radius=10, offset=(20, 0, 0), rise=(0, 0, 200), drag=1.5, mat=MI("MI_FireJato_Brasa"), sort=3),
    # (halo próprio tirado: o HaloCabeca do jato já envolve a bola; os dois juntos passavam de 2 ms)
}

name = "NS_Katon_Gokakyu_Sopro"
path = f"{FOLDER}/{name}"
if EAL.does_asset_exist(path):
    dest = f"/Game/_ToDelete/{name}_{int(time.time())}"
    assert EAL.rename_asset(path, dest), dest
    print("versão anterior movida para", dest)
s = NT.create_niagara_system(FOLDER, name, [unreal.load_asset(T_FOUNTAIN)] * len(CFG), list(CFG))
assert s, name
for em, c in CFG.items():
    assert NT.set_niagara_emitter_property(s, em, "bLocalSpace", "True"), em
    assert NT.set_niagara_module_pin(s, em, "EmitterState", "Life Cycle Mode", SELF), em
    assert NT.set_niagara_module_pin(s, em, "EmitterState", "Loop Behavior", ONCE), em
    if c["fase"][0] > 0:
        assert NT.set_niagara_module_pin(s, em, "EmitterState", "UseLoopDelay", "true"), em
    assert NT.set_niagara_module_pin(s, em, "InitializeParticle", "UsePositionOffset", "true"), em
    if c.get("stretch") or c.get("sprite"):
        assert NT.set_niagara_module_pin(s, em, "InitializeParticle", "Sprite Size Mode", "NewEnumerator2"), em
NT.compile_niagara_system(s, False, unreal.Vector(0, 0, 0))
for em, c in CFG.items():
    inicio, dur = c["fase"]
    const(s, em, "EmitterState.Loop Duration", dur)
    if inicio > 0:
        const(s, em, "EmitterState.Loop Delay", inicio)
    const(s, em, "SpawnRate.SpawnRate", c["rate"])
    const(s, em, "InitializeParticle.Lifetime Min", c["life"][0])
    const(s, em, "InitializeParticle.Lifetime Max", c["life"][1])
    if c.get("sprite"):
        const(s, em, "InitializeParticle.Sprite Size Min", c["sprite"][0])
        const(s, em, "InitializeParticle.Sprite Size Max", c["sprite"][1])
        rend(s, em, "Alignment", "VelocityAligned")
    elif c.get("stretch"):
        w, l = c["stretch"]
        const(s, em, "InitializeParticle.Sprite Size Min", (c["size"][0] * w, c["size"][0] * l))
        const(s, em, "InitializeParticle.Sprite Size Max", (c["size"][1] * w, c["size"][1] * l))
        rend(s, em, "Alignment", "VelocityAligned")
    else:
        const(s, em, "InitializeParticle.Uniform Sprite Size Min", c["size"][0])
        const(s, em, "InitializeParticle.Uniform Sprite Size Max", c["size"][1])
    aligned = bool(c.get("sprite") or c.get("stretch"))
    const(s, em, "InitializeParticle.Sprite Rotation Angle Min", 0 if aligned else -40)
    const(s, em, "InitializeParticle.Sprite Rotation Angle Max", 0 if aligned else 40)
    const(s, em, "InitializeParticle.Position Offset", c["offset"])
    const(s, em, "RandomRangeFloat.Minimum", c["speed"][0])
    const(s, em, "RandomRangeFloat.Maximum", c["speed"][1])
    const(s, em, "AddVelocity.Cone Axis", c["axis"])
    const(s, em, "AddVelocity.Cone Angle", c["cone"])
    const(s, em, "ShapeLocation.Sphere Radius", c["radius"])
    const(s, em, "GravityForce.Gravity", c["rise"])
    const(s, em, "Drag.Drag", c["drag"])
    rend(s, em, "Material", c["mat"])
    rend(s, em, "SortMode", "ViewDepth")
    rend(s, em, "SortOrderHint", str(c["sort"]))
assert NT.compile_niagara_system(s, True, unreal.Vector(1500, 1500, 1000)), name
EAL.save_loaded_asset(s, False)
print("OK", path)

# ---------------------------------------------------------------- DA e BP: sem carga/aura; sopro como saída; bola presa
da.set_editor_property("gokakyu_charge_system", None)          # aura em volta do corpo: removida
da.set_editor_property("goukakyuu_exit_system", s)
EAL.save_loaded_asset(da, False)
bp = unreal.load_asset(BP)
cdo = unreal.get_default_object(bp.generated_class())
cdo.set_editor_property("anticipation_light", 0.0)             # nada durante os selos
cdo.set_editor_property("gokakyu_hold_time", HOLD)
cdo.set_editor_property("gokakyu_hold_forward", 1.0)
cdo.set_editor_property("goukakyuu_exit_hold_time", 0.3)        # luz forte no chão à frente durante o sopro
cdo.set_editor_property("goukakyuu_exit_fade_time", 0.6)
cdo.set_editor_property("goukakyuu_exit_light_intensity", 35.0)   # 60 amarelava a árvore perto
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
EAL.save_loaded_asset(bp, False)
print("GOKAKYU_SOPRO_OK cuspe=%.2f hold=%.2f" % (CUSPE, HOLD))
