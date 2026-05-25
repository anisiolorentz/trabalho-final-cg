## 4

PROMPT: Ajustar a peca amarela sobre a mesa para parecer um cilindro mais convincente: malha mais arredondada, sem pontas/facetamento forte nas bordas, orientada como cilindro/capsula alongada e com comprimento aproximadamente quatro a cinco vezes maior que o visual anterior. Atualizar colisao/AABB e posicionamento para manter a peca coerente sobre a mesa.

## 3

PROMPT: Corrigir o cubo geometrico do puzzle que fica sobre a mesa para que suas faces tenham winding externo e as normais calculadas apontem para fora. O objetivo e remover a aparencia de cubo invertido/visto por dentro, mantendo o comportamento de jogo e a iluminacao existentes.

## 2

PROMPT: Refatorar significativamente `src/main.cpp` sem alterar comportamento ou regras de negocio, reduzindo seu tamanho pelo menos pela metade e separando responsabilidades em modulos coesos. Extrair carregamento/renderizacao de cena, callbacks de entrada, overlay de debug/FPS, guia Bezier e logica de jogo/colisoes em arquivos proprios, mantendo o build atualizado para CMake e Makefile.

## 1

PROMPT: Ajustar o guia visual da curva Bezier para sair da mesa e apontar ate a moldura/saida no topo da parede, usando uma seta 3D larga em cinza claro/branco com ponta triangular/volumetrica inspirada em uma seta desenhada, sem atravessar a parede. A cauda deve parecer uma fumaca/faixa continua que acompanha a curva e some em degrade conforme a seta avanca por delta time, evitando aparencia de bolinhas, cilindros ou orbes enfileirados.
