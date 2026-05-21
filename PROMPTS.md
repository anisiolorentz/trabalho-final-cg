## 4

PROMPT: Ajustar a rampa ao ser pega para que ela fique sempre a frente do jogador, alinhada com a direcao da camera, facilitando posicionar a rampa como caminho do puzzle e corrigir movimento do jogador sobre a rampa para permitir subir pela inclinacao, melhorar precisao dos pulos e deixar o pulo menos "lunar", aumentando ligeiramente a gravidade/queda e ajustando a velocidade de impulso.

## 3

PROMPT: Permitir que o jogador pule e consiga andar sobre objetos geometricos soltos/empilhados, mantendo a ideia inicial do puzzle de construir um caminho com pecas. Implementar gravidade simples no jogador, suporte vertical sobre chao/mesa/pecas e ajuste de colisao para nao bloquear horizontalmente quando o jogador esta em cima de uma peca e corrigir pequeno afundamento visual do cubo ao soltar no chao; a peca deve repousar ligeiramente acima do plano, sem atravessar ou aparentar penetrar o piso.

## 2

PROMPT: Ajustar a escala perceptual do jogador/camera em relacao a mesa e ao espaco. A camera atual parece pequena demais, quase do tamanho da mesa; considerando uma pessoa normal, a altura dos olhos/jogador deve ficar aproximadamente o dobro da altura da mesa e corrigir colisao e gravidade das pecas soltas: objetos nao devem ficar flutuando acima do chao, devem encostar no plano, e ao empilhar uma peca sobre outra nao devem atravessar. Ajustar AABBs e logica de suporte para uma fisica simplificada mais estavel.

## 1

PROMPT: Ao pegar um objeto com `E`, ele deve aparecer inicialmente com o mesmo tamanho que tinha na mesa ou no mundo naquele instante, sem crescer imediatamente. Depois disso, conforme o jogador anda e oscila a camera no eixo Y, aplicar as transformacoes de escala/distancia previstas e implementar gravidade basica nos objetos geometricos ao solta-los, permitindo empilhar pecas e colocar uma em cima da outra com colisao simplificada, mantendo a ideia central de puzzle 3D descrita no `SPEC.md` e evitando fisica realista pesada.
