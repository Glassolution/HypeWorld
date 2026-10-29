# Katon — corpo de chamas

Os três flipbooks aprovados vêm do Niagara Baker, com 64 quadros em atlas 2048×2048. Os dados usados no jogo ficam em `/Game/HypeWorld/Jutsus/Katon/VFX/Flipbooks`; as simulações e o mapa de autoria ficam em `Fluidos`, excluído do cook. Nenhum solver de fluidos roda durante a partida.

Cada atlas RGBA8 guarda temperatura em R, deslocamento UV por quadro em G/B e opacidade de densidade em A. Os vetores têm sinal preservado e eixo vertical corrigido; a advecção reduz o erro entre quadros em 32–42% frente à amostra sem movimento. O gradiente é o Curve Atlas `CA_Katon_Temperatura`, editável com `C_Katon_Temperatura`.

O sistema `NS_Katon_FogoVivo` reúne Chamas (90/s, 0,30–0,42 s), Linguas (28/s, 0,30–0,45 s), Cauda (30/s, 0,30–0,48 s), Fumaca (12/s, 0,40–0,65 s), Nucleo (10/s, 0,25–0,40 s) e Brasas (38/s, 0,40–0,75 s). Média em regime: ~55 chamas + 3 núcleos + 6 fumaças; ~22 brasas pequenas. Todos usam GPU, curl noise e limites fixos de volume. O volume principal é oval e local; cauda, fumaça e brasas persistem em espaço do mundo.

As chamas usam temperatura, envelhecimento, erosão por parâmetro dinâmico, crescimento/encolhimento, rotação aleatória, ondulação UV e interpolação com os vetores gravados. Para limitar a sobreposição, Chamas e Linguas usam recorte com profundidade e máscara barata de densidade; a cauda e a fumaça mantêm transparência com Depth Fade. O núcleo é aditivo. A composição das camadas transparentes acontece antes do DOF; os vetores geométricos do renderer são desativados, sem desativar os vetores do flipbook. O núcleo usa poucos sprites aditivos. A geometria de recorte tem oito vértices e é calculada sobre a união das silhuetas. Acima de 18 m, a emissão cai para 55%; o renderer é limitado a 100 m.

As malhas de corpo ficam invisíveis durante o voo. A distorção usa uma amostra deslocada da cor da cena, evitando o passe global caro de refração. As malhas legadas continuam apenas na onda de impacto. Saída pela boca, selo, slot, dano, custo, cooldown e mensagens replicadas são preservados.

A gravação usa 60 FPS de simulação fixa para capturar a animação; a medição usa tempo real sem limite de FPS, sem captura simultânea, Alto (sg=2), 1920×1080 e resolução 100%. Há duas repetições perto, um bloco longe e um bloco com fundo opaco simples constante em ambos os estados, para não creditar vegetação encoberta como economia do efeito. Os CSVs e resultados acompanham a entrega.
