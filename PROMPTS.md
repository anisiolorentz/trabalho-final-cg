transformar essas peças desenhadas “na mão” em uma lista GameObject, ainda sem interação.

implementar seleção de peça com teclado, por exemplo E seleciona a peça mais próxima/central, e enquanto selecionada ela ganha um destaque visual ou começa a acompanhar uma posição fixa à frente da câmera.

implementar “pegar/soltar” ainda em versão simples e visual. A peça selecionada vai acompanhar um ponto à frente da câmera enquanto estiver sendo segurada, e a escala vai variar com o ângulo vertical da câmera; ao soltar, ela fica fixa na cena.

ao pegar/soltar a peça com E o objeto está muito grande, e soltando com E a cor azul ainda persiste no objeto, cada um deve ter um cor e quando soltar deve se manter na cor original e não da cor de seleção C, circulo amarelo, cubo verde, triangulo vermelho, ao pegar com E o tamanho deve ser menor e mais proporcional, atualmente está ocupando boa parte da cena

 seguir com uma etapa que deixa a mecânica mais próxima do puzzle: ao soltar uma peça, ela fica fixa no mundo, e a peça original reaparece na mesa para poder ser usada de novo. Assim começamos a ter “instâncias” e o jogador pode ir construindo o caminho peça por peça, sem acabar com só três objetos.