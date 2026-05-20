ao pegar/soltar a peça com E o objeto está muito grande, e soltando com E a cor azul ainda persiste no objeto, cada um deve ter um cor e quando soltar deve se manter na cor original e não da cor de seleção C, circulo amarelo, cubo verde, triangulo vermelho, ao pegar com E o tamanho deve ser muito menor, atualmente está ocupando boa parte da cena

Dado os requisitos, agora as peças da mesa precisam funcionar como fontes reutilizáveis.

Comportamento novo:

C seleciona uma peça da mesa.
E cria uma cópia segurada daquela peça.
A peça original continua na mesa.
A cópia acompanha a câmera e muda de escala.
E solta a cópia no mundo.
A cópia solta fica fixa e deixa de ser selecionável por enquanto.

implemente funcionalidade do objeto crescer conforme oscilação da camera no eixo y, olhando para cima aumenta, para baixo diminui

o "pegar" objeto com E deixe ele mais longe ao centro na cena para ficar mais visivel a visualização do objeto aumentando, a seleção com C, altere também para ser possível selecionar como um "alvo" como se fosse uma mira no centro da tela, onde ao "mirar" /olhar para o objeto específico na devida marcada e apertar E, pegue o objeto