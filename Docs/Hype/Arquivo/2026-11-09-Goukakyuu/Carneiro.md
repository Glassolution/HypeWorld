---
status: pose-estatica-em-revisao-nao-validada
atualizado: 2026-11-09
---
# Carneiro — pose parada antes de integrar

> A revisão no Blender melhorou punhos e cintura numa cópia autorizada; o encaixe dos dedos ainda não passou.
> Não criar transições, alterar ficha ou salvar/importar na Unreal antes de a pose estática ficar limpa.

**Alvo:** partir do Tigre aprovado. Mãos em bloco vertical no zíper, palmas/bordas internas em contato; antebraços horizontais, cotovelos dobrados, punhos acima do cinto. Indicador e médio de cada mão retos e juntos, ~8° para dentro, pontas centrais um pouco mais altas; anelar/mínimo dobrados e entrelaçados abaixo; polegares recolhidos junto deles. Conferir frente com cotovelos, perfil, cima e closes: contato, dedos, punhos suaves e ausência de atravessamentos.

**Estado real:** revisão 63 preserva punhos/cintura e melhora os polegares por oposição, mas o encaixe ainda não passou em perfil/cima. Usuário escolheu manter a malha e usar só pose/corretivos; não remodelar falanges. Nenhuma exceção aprovada. Cintura da cópia melhorou no Blender, mas não foi testada em Play/idle novo. Não declarar o Carneiro concluído nem a sequência de seis integrada.

**Diagnóstico:** manga não segue a mão (peso `hand` = 0 na amostra), segue antebraço/twists; não houve repintura dessas regiões. Punho antigo continuava ruim sem corretivos: voltar à orientação do Tigre foi mais eficaz. Cintura tinha descontinuidade de pesos pelve/coluna/coxa; usuário autorizou corrigir em uma cópia. Aresta de diagnóstico: 8,487 → 0,835 cm. Mãos/braços de Tigre, Serpente e Javali: diferença máxima 0 cm, novos corretivos em zero.

**Retomada WS:** `Referencias/Katon/Carneiro/Diagnostico-2026-11-09/Estado.json` aponta o checkpoint, pose, valores e script repetível. Ler só esse estado e `Relatorio.md` conforme necessário. Checkpoint: `Personagem - Carneiro revisao estatica.blend`; capturas atuais prefixo `63`. Não retomar de tentativas intermediárias nem da pose antiga de 08/11. Biblioteca da revisão é rascunho; bibliotecas aprovadas intactas. Chaves experimentais reprovadas ficam em zero e não devem ser exportadas.

**Preservação:** fonte e backup SHA256 `13813c48eec159b20028e59af289ce830ed79de332b61d6100554d712c32b510`; hashes/regressões na mesma pasta. A nota anterior, incluindo o erro antigo do corretivo de 1,5346 m e arquivos de 08/11, foi preservada em `Hype/Arquivo/2026-11-09-Carneiro/Carneiro antes do diagnostico.md`. Não elevar limites para esconder deformação.

Depois da pose validada: entrada, Serpente→Carneiro→Macaco, transições lentas, ficha completa de seis selos, Play e nova medição, sem mudar ritmo nem marcadores. Ordem e medições em [[Katon]].

Relacionadas: [[Rig]], [[Blender]], [[Selos/Tigre]], [[Selos/Serpente]], [[Animacoes]], [[Erros]].
