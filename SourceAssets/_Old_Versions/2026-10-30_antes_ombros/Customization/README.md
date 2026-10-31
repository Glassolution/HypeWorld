# Cabelos e olhos — fontes para Windows e Mac

`Hair/` contém todos os 46 GLBs e o ZIP encontrados em `Downloads/personagem para customizacao/Cabelos`, sem modificar os arquivos originais. `Eyes/` contém as três íris extraídas: Kakashi, Minato e Sasuke. Os assets Unreal já existentes ficam em `Content/HypeWorld/Characters/Parts/Hair` e `Eyes`.

No Mac, com o Git LFS instalado, execute na pasta do projeto:

```sh
git pull origin main
git lfs pull
```

Abra `HypeWorld.uproject` com a mesma versão do Unreal usada no projeto (5.8.4). Os arquivos de `SourceAssets` são fontes de arte; os `.uasset` de `Content` são os assets do editor. Os GLBs preservam seus materiais e texturas incorporados. O ZIP do Naruto contém o FBX e as texturas, incluindo `textures/eyes.png`.

O manifesto registra nomes, tamanhos e SHA-256 de todos os arquivos. `zabusa (1).glb` e `zabusa.glb` são cópias idênticas, ambas preservadas. Alguns GLBs da pasta de cabelos incluem personagens completos e acessórios; os nomes originais foram mantidos para não descartar conteúdo do download.
