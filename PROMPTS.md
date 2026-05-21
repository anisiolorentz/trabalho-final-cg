
## 2026-05-21 - Refatoracao HUD e ajustes de objeto segurado

PROMPT: Remover a coloracao azul ao focar/selecionar objetos, refatorar significativamente `main.cpp` separando responsabilidades em mais de um arquivo sem alterar regras de negocio, refinar novamente a maozinha do HUD para um estilo mais limpo, impedir que o objeto segurado atravesse o chao ou saia do plano ao olhar para baixo/lados, e ajustar o objeto segurado para ficar levemente mais longe, crescer mais ao olhar para cima e considerar tambem a distancia do jogador para aumentar a percepcao de escala.

## 2026-05-21 - Repegar pecas e reforcar escala por olhar

PROMPT: Depois de soltar uma peca geometrica no mundo, permitir pega-la novamente. Refinar o estilo visual da mao do HUD, deixar o objeto segurado mais distante da camera e tornar mais agressiva a mudanca de volume conforme o jogador olha para cima/baixo, usando a variacao do eixo Y da direcao da camera.

## 2026-05-21 - HUD de alvo e estados de interacao

PROMPT: A cada alteracao orientada, adicionar uma nova entrada em `PROMPTS.md`. Implementar um alvo fixo na tela: circulo no estado neutro, icone pequeno de mao aberta com texto "grab" quando a mira aponta para um objeto geometrico manipulavel, e mao fechada sem texto enquanto o jogador segura uma copia da peca. Ao soltar, voltar ao estado inicial conforme a mira.

## 2026-05-20 - Colisao com mesa e pecas soltas

PROMPT: Implementar colisao do jogador com a mesa e com os objetos geometricos soltos no mundo, manter as pecas originais da mesa como fontes reutilizaveis sem bloquear individualmente, afastar o ponto inicial do jogador, mover a mesa para um canto da sala e alterar a sala para um formato retangular em vez de quadrado.

## 2026-05-19 - Etapa 1: colisoes

PROMPT: Implementar a etapa 1 do plano: criar um modulo separado de colisoes (`include/collisions.h` e `src/collisions.cpp`), mover/centralizar testes simples de AABB e interseccao de mira, integrar no `main.cpp` para limitar o jogador dentro da sala e selecionar pecas pela mira, e atualizar o build para compilar o novo arquivo.

## 2026-05-19 - Analise de estado e plano incremental

PROMPT: Revisar `SPEC.md`, enunciado e implementacao atual para identificar pontos de negocio e tecnicos do puzzle 3D, apontar o que ja existe, o que esta parcial e o que falta, e montar um plano incremental para avancar o projeto em etapas.
