## 5

PROMPT: Consolidar o jogo como puzzle 3D completo: detectar vitoria quando o jogador alcanca e pisa na varanda/plataforma elevada, exibir comemoracao simples no HUD com confetes, estrelas e mensagem de parabens, mostrar contagem regressiva de reinicio apos 2 segundos e resetar estado inicial de camera, pecas, selecao, fisica e animacoes. Manter controles de interacao com `E` para pegar/soltar confirmando e `Q` para cancelar manipulacao, descartando copias da mesa ou restaurando pecas ja colocadas.

## 4

PROMPT: Refinar interacao, escala e fisica das pecas geometricas: permitir pegar novamente objetos soltos, segurar objetos a frente da camera com crescimento controlado pelo eixo Y sem atravessar teto/chao/paredes, aplicar gravidade basica ao soltar, permitir empilhamento, impedir tremores em repouso e bloquear soltura/apoio de objetos sobre a varanda. A rampa deve preservar orientacao dinamica ao ser segurada e acompanhar o yaw da camera para apontar para a parede desejada.

## 3

PROMPT: Implementar e estabilizar locomocao, pulo e colisoes do jogador: colisao com sala, mesa, paredes, objetos e varanda; suporte vertical para subir em cubos/cilindros quando o pulo alcanca; rampa caminhavel pela face inclinada sem teleporte; colisao lateral da rampa deve bloquear como obstaculo quando o jogador encosta pelos lados, mas permitir entrada normal pela base com tolerancia suficiente para o corpo do jogador.

## 2

PROMPT: Construir a cena final do puzzle em uma sala interna retangular e alta, com mesa distante em um canto, plataforma/varanda elevada, porta texturizada no topo, muretas laterais, teto cinza, luminarias distribuidas, sombras simplificadas e texturas coerentes em chao, paredes, mesa, porta e concreto da varanda. Reorganizar assets em `data/assets`, removendo duplicatas e arquivos nao usados, mantendo objetos em diretorios proprios com OBJ/FBX/textures quando aplicavel.

## 1

PROMPT: Evoluir a base tecnica do projeto conforme requisitos academicos: modularizar `main.cpp`, centralizar AABB/colisoes, criar estrutura de objetos do jogo, implementar camera FPS e modo alternativo, selecao por mira com HUD de circulo/mao/grab, instancias, texturas, iluminacao, sombras, animacoes por delta time, guia visual por curva Bezier cubica com seta/faixa de fumaca e moldura pulsante de saida sempre visivel.
