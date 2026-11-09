# Gōkakyū v2 — tutorial para o Codex continuar no Mac (M1, 8 GB)

Escrito em 09/11/2026 no fim do passe "trava + Zenkai" (feito no PC Windows, GTX 1650).
Leia também `CLAUDE.md` (regras do projeto) antes de mexer em qualquer coisa.

## 1. Estado atual

- Jutsu: `DA_Katon_Goukakyuu` (`/Game/HypeWorld/Jutsus/Katon/`) + projétil `BP_Katon_Goukakyuu`
  (C++ `Source/HypeWorld/Combat/NinjaJutsuProjectile.h/.cpp`).
- VFX v2: `/Game/HypeWorld/VFX/Katon/`
  - `Niagara/NS_Katon_Gokakyu_v2_Carga` (carga na boca + clarão, anel de choque e rajada da soltura)
  - `Niagara/NS_Katon_Gokakyu_v2_Voo` (labaredas, borda vermelha, cauda contínua, fumaça, fogo no chão, faíscas, halo)
  - `Niagara/NS_Katon_Gokakyu_v2_Impacto` (onda, estouro, chamas, fogo residual ~2 s, fumaça)
  - `Materiais/M_FireMaster` (um nó Custom HLSL) + instâncias `MI_Katon_*`
  - `Meshes/` (núcleo, onda de choque etc., vindas de `SourceAssets/VFX/Katon/Katon_Meshes.blend`)
- Linha do tempo (medida): cuspe (notify `Katon_Spit`) → carga na boca até **0,32 s** → no 1º quadro depois
  da soltura a bola já anda a ~2950 cm/s (0 ms de espera). Arrasto leve (perde ~30% em 1 s), cresce 10%,
  desvio suave. O dano continua no disparo.
- Custo (GTX 1650, passada Translucency do ProfileGPU): base 0,11 ms; voo pico 0,78 ms (soltura);
  impacto perto da câmera 2,11 ms (pior caso). Detalhes: `custo_gpu.txt`.
- Evidências (JPG) nesta pasta: `trava_camera_jogo_-0.05a+0.6s.jpg`, `trava_de_cima_cauda.jpg`,
  `trava_voo_close_vs_zenkai.jpg`, `trava_clarao_anel_saida.jpg`.
- Backups: `/Game/HypeWorld/VFX/Katon/Backup_v2b_0911/` (23 assets antes deste passe) +
  `backup_valores_v2b.json`; versões anteriores em `Backup_v2_0811` e
  `/Game/HypeWorld/Jutsus/Katon/VFX/Gokakyu/Backup_v1_0811`.

## 2. O que ainda falta (pendências reais)

1. **Ainda lê como "bola de energia" ao lado do Zenkai** (ver `trava_voo_close_vs_zenkai.jpg`): o núcleo + halo
   são claros e redondos demais; o contorno vermelho é fraco; o Zenkai tem uma COLUNA de chamas densa que se curva
   para cima/trás. Caminhos sugeridos: núcleo menor/escondido (`gokakyu_core_scale`), halo ainda mais fraco
   (`MI_Katon_Halo` Brilho), `BordaVermelha*` maiores e mais opacas atrás, cauda (`CaudaFogo`) mais alta
   (menos vento para baixo, mais `rise` Z) e mais densa perto da bola.
2. Muita chama baixa/no chão na câmera de perto (o `RastroChao` + a cauda): conferir pela câmera do jogo.
3. Impacto perto da câmera em 2,1 ms (no limite de ~2 ms).
4. A textura `T_Katon_Alongada_Dados` (flipbook 8x8 do fluido) é de baixa resolução: de perto fica "pipoca".
   Hoje está com **LOD bias 2** para amaciar. Se trocar a textura, rever isso.
5. Tentativa de "mip bias" no nó de flipbook do material falhou (o script imprime
   `AVISO: viés de mip não ligado`): é inofensivo.

## 3. Primeira vez no Mac (depois do pull)

O C++ mudou (`NinjaJutsuProjectile.cpp`), então precisa compilar **com o editor fechado** (no Mac não há
Live Coding). O jeito mais simples é o "Atualizar e Abrir" (faz pull, `git lfs pull`, compila e abre):

```bash
"/Users/luisfelipexavier/Desktop/Hype World/Atualizar e Abrir Editor.command"
```

Ou à mão, na pasta do projeto:

```bash
git pull --rebase && git lfs pull
"/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh" HypeWorldEditor Mac Development -project="$PWD/HypeWorld.uproject"
```

Os assets deste passe JÁ estão no Git (não precisa rodar os scripts de novo para ver o resultado).
Nada novo foi posto em `Tools/pendentes.txt`.

## 4. Como mexer no efeito (scripts que refazem tudo)

Todos rodam no editor aberto pelo Python Remote Execution:

```bash
python3 -I Tools/ue_exec.py -f Tools/editor/gokakyu_v2_2_material.py
```

Ordem e papel:

| Script | Faz |
|---|---|
| `Tools/editor/gokakyu_v2_1_importar.py` | importa as malhas FBX (`SourceAssets/VFX/Katon/FBX`), Nanite desligado |
| `Tools/editor/gokakyu_v2_2_material.py` | `M_FireMaster` + todas as `MI_Katon_*` (parâmetros de cor/brilho/erosão) |
| `Tools/editor/gokakyu_v2_3_niagara.py` | monta os 3 sistemas Niagara (emissores, tamanhos, taxas, vento) |
| `Tools/editor/gokakyu_v2_4_ligar.py` | liga tudo no DA e no BP (tempo, velocidade, tamanho, luz) e compila o BP |

**Regra importante:** depois de rodar o `_3_niagara`, rode SEMPRE o `_4_ligar`. O script 3 move a versão velha
para `/Game/_ToDelete/` e cria a nova no mesmo caminho; o DA continua apontando para a versão movida até o
`_4_ligar` religar (sintoma: "mudei e nada mudou no jogo").

Onde fica cada ajuste:

| Quero mudar | Onde |
|---|---|
| Momento da soltura | `gokakyu_hold_time` (0,32) no `_4_ligar` **e** `SOLTA` no `_3_niagara` (os dois juntos) |
| Velocidade / arrasto / desvio | `accel_time`, `launch_speed_scale`, `gokakyu_drag`, `gokakyu_wander`, `launch_bend` no `_4_ligar` |
| Tamanho da bola | `full_radius` (210), `gokakyu_size`, `gokakyu_core_scale` no `_4_ligar`; `R`/`V` no topo do `_3_niagara` |
| Clarão da saída | emissor `FlashSaida` (tamanho) e `MI_Katon_FlashSaida` (brilho); pico de luz `gokakyu_light_launch_peak` |
| Anel de choque | emissor `AnelSaida` + `MI_Katon_Onda` |
| Chamas na bola | emissores `Labaredas`, `LabaredasTras` + `MI_Katon_Labareda` |
| Contorno vermelho | `BordaVermelha`, `BordaVermelhaTras` + `MI_Katon_BordaVermelha` (#E0281C → #8A1A10) |
| Cauda | `CaudaFogo` (`spacing` = cm entre chamas, `curl`, `rise`) + `MI_Katon_CaudaFogo` (`Esfriar`) |
| Fumaça | `Fumaca` + `MI_Katon_Fumaca` (instância do material de fumaça do pack, translúcida) |
| Fogo no chão | `RastroChao` (voo, ~1 s) e `ChaoResidual`/`FogoResidual` (impacto, ~2 s) |
| Custo | taxas (`rate`), `spacing` e tamanhos dos emissores grandes; impacto: `Chamas`/`ChamasVermelhas` |

Não tocar (pedido do Felipe): notify, selos, combate, input e Post Process global. Paleta do miolo/corpo,
luz forte e bloom foram aprovados.

## 5. Testar (sempre pela câmera do jogo)

Com o editor aberto e o jogo em Play (o Felipe julga pela câmera de 3ª pessoa, não pela câmera de cima).
Ajustes de teste são variáveis no módulo `unreal`, que ficam guardadas entre chamadas: mande-as antes.

```bash
python3 -I Tools/ue_exec.py "import unreal; unreal._teste_gk_slomo=0.08; unreal._teste_gk_tempos=[round(0.27+0.05*i,2) for i in range(14)]"
python3 -I Tools/ue_exec.py -f Tools/editor/teste_goukakyuu.py
```

- `teste_goukakyuu.py`: sequência na câmera do jogo (tempos contados do nascimento do projétil; soltura = 0,32).
- `teste_gokakyu_v2_voo.py`: `unreal._teste_voo_modo = "cima"` (trajetória/cauda) ou `"close"` (acompanha a bola de lado).
- `teste_gokakyu_v2_custo.py`: ProfileGPU (base, carga, soltura, voo). `unreal._teste_custo_pitch = -25` faz a bola
  bater perto e mede o impacto. O resultado sai no log (`~/Library/Logs/Unreal Engine/HypeWorldEditor/HypeWorld.log`,
  procurar `GOKAKYU_CUSTO_MARCA` e a linha `Translucency`). No M1 os números NÃO são comparáveis com a GTX 1650.
- Fotos: comando `Shot` → `Saved/Screenshots/MacEditor/`. Evidência no Git vai como **JPG** (PNG em
  `Docs/Jutsus/Katon/` é ignorado pelo `.gitignore`).

Armadilhas já conhecidas:

- **Chakra:** cada disparo gasta 30 de 100; depois de ~3 testes, pare e reinicie o Play.
- **Mira:** os testes nivelam a mira (pitch -4) antes do disparo; sem isso a bola pode bater no chão cedo.
- **Editor em segundo plano** roda a ~3 FPS e estraga os testes: deixe a janela da Unreal na frente
  (e veja o item de `bThrottleCPUWhenNotForeground` no `CLAUDE.md`).
- **Callbacks de tick:** os testes usam `register_slate_post_tick_callback`; se um teste der erro sem
  desregistrar, ele roda todo quadro (log gigante, FPS cai). Os scripts atuais desregistram no erro; se o FPS
  despencar, reinicie o editor.
- **Mudança no BP pelo Python** só vale no Play depois de `compile_blueprint` (o `_4_ligar` já faz).
- **Flipbook no material:** com SubImageSize 8x8 a UV que chega é a do quadro (0–0,125). O `M_FireMaster`
  corrige com `UV = frac(UV * 8.0)` quando `UsarFB` = 1. Sem isso as chamas somem.
- **Mac 8 GB:** feche Blender/navegador pesados antes de abrir a Unreal; compile com o editor fechado.

## 6. Regras de entrega

- Nunca apagar asset: mover para `/Content/_ToDelete/` e avisar. Backup antes de alterar.
- Fechar a Unreal antes de qualquer `git pull`/checkout que mexa em `.uasset`.
- `git pull --rebase` antes de commitar/enviar; nunca sobrescrever commits de outra pessoa.
- Commit em português explicando o que mudou. Push só quando o Felipe pedir.
- Resposta final ao Felipe: 3 a 5 linhas (o que foi feito, prova, pendências, o que ele deveria entender).
