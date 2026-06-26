# Computação Gráfica e Visualização I (INF01047) - INF/UFRGS

Este repositório contém o código base para o trabalho final. O enunciado completo do trabalho final está no Moodle:

https://moodle.ufrgs.br/mod/assign/view.php?id=6018620

## A aplicação

Desenvolvemos um jogo de puzzle 3D em primeira pessoa que se passa em uma sala
interna fechada, inspirado no vídeo de referência do jogo Superliminal. O
jogador explora o ambiente em primeira pessoa e pode interagir com três peças
geométricas que ficam em uma mesa (um cubo, um cilindro e uma rampa).
Elas podem ser seguradas, movidas, rotacionadas (sobre o eixo Y) e soltadas para construir
um caminho até uma plataforma elevada com uma porta de saída, completando o puzzle. A cena
inclui malhas poligonais complexas (mesa e porta de castelo), diversas
instâncias de objetos (peças do puzzle e luminárias do teto), iluminação por
modelo de Blinn-Phong (componentes ambiente, difusa e especular) e mapeamento de
texturas em todos os objetos. A movimentação do jogador, das peças, a
seta-guia animada ao longo de uma curva de Bézier cúbica que aponta para a
saída, são calculadas com base no tempo entre quadros (Δt), garantindo
velocidade consistente independente da taxa de quadros. Também possui dois tipos de
câmera (livre em primeira pessoa e look-at), testes de intersecção para colisões 
(jogador, paredes, mesa e peças) e para seleção das peças (ray-casting a partir da mira). 
Os objetos segurados crescem de acordo com a perspectiva, de acordo com o ângulo entre a mira e o chão.

## Contribuições da dupla

Anisio ficou responsável pela estruturação da sala, sistema de
câmeras (livre e look-at), mapeamento das texturas das peças, sistema de sombras, implementação
da movimentação do jogador e da rotação dos objetos. E Antonio pela mecânica de interação com as peças (pegar/mover/soltar/redimensionamento), testes de intersecção e colisões, animação por curva de Bézier e iluminação. 
A parte de integração e testes finais foram realizadas em conjunto pelos dois.

## Uso de agentes de IA

Utilizamos agentes de IA durante o desenvolvimento. Anisio utilizou o
Claude Code (Anthropic) e Antonio utilizou o Codex (OpenAI). Essas ferramentas foram empregadas
para explicação e compreensão de problemas, geração de de código, além de esclarecimento de conceitos de computação gráfica e do pipeline OpenGL. 
Análise crítica:
Consideramos as ferramentas muito úteis para acelerar tarefas mecânicas. Por outro lado, as
ferramentas nem sempre auxiliaram adequadamente: em alguns momentos
traziam soluções parciais em vez de atacar a causa raiz (foi necessário
insistir para chegar na solução definitiva em casos para corrigir o esticamente das texturas, por exemplo), e casos em que o agente perdeu tempo investigando hipóteses erradas porque dependia de testes do jogo em execução, exigindo verificação humana constante, recompilação e
execução real da aplicação para validar cada mudança. A conclusão é de que os agentes estão muito bons, mas ainda não substituem o entendimento do código nem os testes manuais.

## Manual de utilização

| **W A S D**         ->  Movimentar o jogador (câmera livre)                        
| **Mouse**           ->  Olhar ao redor (orbitar o alvo no modo look-at)            
| **Espaço**          ->  Pular                                                       
| **E**               ->  Pegar / soltar a peça que está sob a mira                  
| **Q**               ->  Cancelar — devolve a peça segurada à posição de origem    
| **R** (segurar)     ->  Girar a peça segurada (eixo Y / yaw)                         
| **V**               ->  Alternar câmera: livre (1ª pessoa) ↔ look-at orbital       
| **Roda do mouse**   ->  Zoom (aproximar/afastar) no modo look-at                         
| **H**               ->  Mostrar/ocultar o texto informativo na tela (HUD)          
| **L**               ->  Recarregar os shaders em tempo de execução                 
| **ESC**             ->  Sair da aplicação                                           

**Objetivo:** organizar as peças (empilhando/posicionando o cubo, o cilindro e a
rampa) para formar um caminho que permita subir até a plataforma elevada e
alcançar a porta de saída.

## Compilação e execução

O executável chama-se `main` e deve ser executado a partir do seu diretório de
saída (`bin/<plataforma>/`), pois os caminhos dos assets são relativos
(`../../data/...`). Os alvos `run` dos Makefiles já fazem isso automaticamente.

### Linux
Requer `cmake`, um compilador C++ e as bibliotecas de desenvolvimento de OpenGL/
GLFW (a GLFW é compilada pelo próprio projeto). Na raiz do repositório:
```bash
make            # configura e compila via CMake (presets default-config/default-build)
make run        # executa a partir de bin/Linux/