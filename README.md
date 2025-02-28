# floorpln
Trabalho de Implementação do Problema Floorplan Design, desenvolvido para o Curso de Mestrado/Doutorado em Engenharia de Sistemas e Computação da COPPE/UFRJ entre 1998-2002.

Disciplina: Programação Concorrente
Professores: 
- Inês de Castro Dutra (https://sigarra.up.pt/fcup/pt/func_geral.formview?p_codigo=427256);

Autor(100%): Luiz Marcio Faria de Aquino Viana, Pós-D.Sc.

Prazo de dezenvolvimento dos programas: 15 dias

Aplicação: Trabalho de Implementação do Problema Floorplan Design.

1. OBJETIVO: 

Este trabalho de implementação tem como objetivo complementar o curso de programação concorrente, introduzindo conhecimentos práticos de programação paralela através do desenvolvimento de cinco versões paralelas do problema do Floorplan Design.

Cada implementação utiliza recursos de software e hardwares diferentes, sendo três delas desenvolvidas para máquinas de memória compartilhada centralizada utilizando POSIX Threads, IPC Share Memory e a linguagem SR, e duas para sistemas de memória compartilhada distribuída utilizando PVM e ThreadMarks.

2. DESCRIÇÃO DO PROBLEMA: "FLOORPLAN DESIGN"

O processo de confecção de circuito integrado atravessa várias etapas. Entre elas está a fase de lay out onde os componentes são posicionados dentro da pastilha. 

A fase de lay out é dividida em três partes. Na primeira parte são construídos conjuntos de blocos retangulares indivisíveis denominados células onde cada conjunto representa diferentes instâncias de implementação de um dos componente do circuito. 

Em seguida são estabelecidas as posições relativas de cada componente informando as conexões existentes entre eles. Na terceira fase são analisadas as possíveis combinações de células até encontrar a combinação de instâncias que requer área mínima de pastilha.

O problema do Floorplan Design esta relacionado ao último passo do processo de lay out de circuitos integrados e consiste na determinação das combinações de instâncias de cada componente que minimizam à área de pastilha. Para isso é necessário o conhecimento do conjunto de instâncias de cada componente e a posição relativa entre eles. 

3. DIRETÓRIOS:
- PVM - Ref: https://www.netlib.org/pvm3/
- SEQ_NORM e SEQ_OPT - Programa sequencial para solução do problema.
- SHMEM - Ref: https://en.wikipedia.org/wiki/SHMEM
- SR_DIST e SR_SHM - Ref: https://en.wikipedia.org/wiki/SR_(programming_language)
- THREADS - Ref: https://www.cs.cmu.edu/afs/cs/academic/class/15492-f07/www/pthreads.html
- TMK - Software DSM (Distributed Shared Memory) - Ref: https://en.wikipedia.org/wiki/Distributed_shared_memory

# CONTATO

CONTACT ME, IF YOU NEED HELP OR HAVE ANY QUESTIONS ABOUT THIS ACADEMIC WORK!

Luiz Marcio Faria de Aquino Viana,Pós-D.Sc.
E-mail: luiz.marcio.viana@gmail.com
Phone/WhatsApp: +55-21-99983-7207
