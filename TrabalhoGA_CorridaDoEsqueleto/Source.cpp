/* Trabalho GA - Processamento Gráfico
 *
 * Grupo: Ana Beatriz Stahl
 * 		  Emanuele Schlemmer Tomazzoni
 * 		  Luisa Becker dos Santos
 * 
 * Tema: Corrida do Esqueleto
 * 
 * Objetivo: O Esqueleto deve desviar e evitar dos 
 * obstáculos que estão caindo do topo da tela.
 * 
 * Professora: Rossana Baptista
 * 
*/

#include <iostream>
#include <string>
#include <assert.h>
#include <cstdlib> 
#include <ctime>
#include <vector>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// STB_IMAGE
#include <stb_image.h>

using namespace glm;

struct Sprite 
{
	GLuint VAO;
	GLuint texID;
	vec3 position;
	vec3 dimensions;
	float angle;
};


// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
int setupGeometry();
int loadTexture(string filePath, int &imgWidth, int &imgHeight);
void drawSprite(Sprite spr, GLuint shaderID);

// Dimensões da janela (pode ser alterado em tempo de execução)
//const GLuint WIDTH = 800, HEIGHT = 600;
const GLuint WIDTH = 2304, HEIGHT = 1296;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar* vertexShaderSource = "#version 400\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec2 texc;\n"
"uniform mat4 projection;\n"
"uniform mat4 model;\n"
"out vec2 texCoord;\n"
"void main()\n"
"{\n"
//...pode ter mais linhas de código aqui!
"gl_Position = projection * model * vec4(position.x, position.y, position.z, 1.0);\n"
"texCoord = vec2(texc.s, 1.0 - texc.t);\n"
"}\0";

//Códifo fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar* fragmentShaderSource = "#version 400\n"
"in vec2 texCoord;\n"
"uniform sampler2D texBuffer;\n"
"out vec4 color;\n"
"void main()\n"
"{\n"
"color = texture(texBuffer, texCoord);\n"
"}\n\0";

enum directions { NONE, LEFT, RIGHT};
int dir = NONE;
float vel = 1.0;

float animationTime = 0.0f;
float frameDuration = 0.1f;
int currentFrame = 0; 

// Estrutura para objetos caindo (moedas e obstáculos)
struct FallingObject
{
	Sprite sprite;
	bool isCoin; // Verifica se é moeda ou obstáculo
	float fallSpeed; // Velocidade de queda
	int currentFrame; // Frame atual para animação de moedas
	bool isVisible; // Indica se é visivel ou não
};

// Vetores para armazenas objetos
std::vector<FallingObject> fallingObjects;

// Gerar posição aleatória na tela
float objectPosition(float maxWidth)
{
	return static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / maxWidth));
}

// Inicializar moedas e obstáculos
void initFallingObjects(int numObjects, GLuint coinTexture, GLuint obstacleTexture, float maxY) {
    for (int i = 0; i < numObjects; ++i) {
        FallingObject obj;
        obj.isCoin = (i % 2 == 0);  // Alterna entre moedas e obstáculos
        obj.fallSpeed = 100.0f + (rand() % 200);  // Velocidade de queda aleatória
        obj.currentFrame = 0;  // Inicia o frame atual como 0
		obj.isVisible = true;
        
        obj.sprite.VAO = setupGeometry();
        obj.sprite.texID = obj.isCoin ? coinTexture : obstacleTexture;
        obj.sprite.dimensions = vec3(50.0f, 50.0f, 0.0f);  // Tamanho genérico para moedas e obstáculos
        obj.sprite.position = vec3(objectPosition(WIDTH), maxY, 0.0f);  // Inicia no topo da tela

        fallingObjects.push_back(obj);
    }
}

// Colisão simples (bounding box)
bool checkCollision(Sprite& character, FallingObject& obj) {
    return (character.position.x < obj.sprite.position.x + obj.sprite.dimensions.x &&
            character.position.x + character.dimensions.x > obj.sprite.position.x &&
            character.position.y < obj.sprite.position.y + obj.sprite.dimensions.y &&
            character.position.y + character.dimensions.y > obj.sprite.position.y);
}

// Estados da moeda e obstáculos
const int COIN_FRAMES = 9;
const int OBSTACLE_FRAMES = 8;

// Dividir a imagem da moeda em 9 frames e fazer a animação
void animateCoin(Sprite& coinSprite, FallingObject& obj, float deltaTime, float& timeSinceLastFrame) {
    const float frameDuration = 0.1f;  // Tempo entre troca de frames

    timeSinceLastFrame += deltaTime;

    if (timeSinceLastFrame >= frameDuration) {
        obj.currentFrame = (obj.currentFrame + 1) % COIN_FRAMES;  // Próximo frame
        timeSinceLastFrame = 0.0f;
    }

    // Calcula a coordenada correta para a moeda dentro da imagem
    float texWidth = 1.0f / COIN_FRAMES;
    float texOffset = obj.currentFrame * texWidth;
    
    glBindTexture(GL_TEXTURE_2D, coinSprite.texID);
    // Atualiza a textura da moeda para o frame atual
    glTexCoord2f(texOffset, 0.0f);
    glTexCoord2f(texOffset + texWidth, 0.0f);
    glTexCoord2f(texOffset + texWidth, 1.0f);
    glTexCoord2f(texOffset, 1.0f);
}

// Selecionar um obstáculo da imagem
void selectObstacle(Sprite& obstacleSprite) {
    int randomObstacle = rand() % OBSTACLE_FRAMES;  // Escolhe um obstáculo aleatoriamente

    // Calcula a coordenada correta para o obstáculo dentro da imagem
    float texWidth = 1.0f / OBSTACLE_FRAMES;
    float texOffset = randomObstacle * texWidth;

    glBindTexture(GL_TEXTURE_2D, obstacleSprite.texID);
    // Atualiza a textura do obstáculo para o frame selecionado
    glTexCoord2f(texOffset, 0.0f);
    glTexCoord2f(texOffset + texWidth, 0.0f);
    glTexCoord2f(texOffset + texWidth, 1.0f);
    glTexCoord2f(texOffset, 1.0f);
}

// Função MAIN
int main()
{
	// Inicializando a seed para números aleatórios
    srand(static_cast<unsigned>(time(0)));
	
	// Inicialização da GLFW
	glfwInit();

	// Criação da janela GLFW
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Corrida do Esqueleto!", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);

	// GLAD: carrega todos os ponteiros d funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
	}

	// Obtendo as informações de versão
	const GLubyte* renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte* version = glGetString(GL_VERSION); /* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);


	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();


	//Sprite do fundo da cena
	Sprite background, character;
	// Gerando um buffer simples, com a geometria de um triângulo
	background.VAO = setupGeometry();
	//Carregando uma textura (recebendo seu ID)
	int imgWidth, imgHeight;

	//ADICIONEI
	GLuint backgroundTexture;
	backgroundTexture = loadTexture("../Sprites/background.png", imgWidth, imgHeight);
	background.texID = backgroundTexture;

	// Aqui, as dimensões do background devem ser definidas usando o tamanho real da imagem
	background.dimensions = vec3((float)imgWidth, (float)imgHeight, 1.0);  // Usando as dimensões reais da textura
	
	character.VAO = setupGeometry();
	character.texID = loadTexture("../TrabalhoGA_ CorridaDoEsqueleto/Sprites/centro.png",imgWidth,imgHeight);
	GLuint textureFrames[4];  // <-- Novas texturas para animação
	textureFrames[0] = loadTexture("../Sprites/centro.png", imgWidth, imgHeight);  // primeira textura
	textureFrames[1] = loadTexture("../Sprites/esquerda.png", imgWidth, imgHeight); // segunda textura
	textureFrames[2] = loadTexture("../Sprites/centro.png", imgWidth, imgHeight);  // primeira textura
	textureFrames[3] = loadTexture("../Sprites/direita.png", imgWidth, imgHeight);  // terceira textura
	
	character.VAO = setupGeometry();
	character.texID = textureFrames[0];
	character.dimensions = vec3(imgWidth*0.5, imgHeight*0.5, 0.25);
	character.position = vec3(50.0, 40.0, 0.0);

	// Inicializa os objetos que vão cair (moedas e obstáculos)
    GLuint coinTexture = loadTexture("../Sprites/coins.png", imgWidth, imgHeight);  // Textura das moedas
    GLuint obstacleTexture = loadTexture("../Sprites/obstacle.png", imgWidth, imgHeight);  // Textura dos obstáculos
    initFallingObjects(5, coinTexture, obstacleTexture, HEIGHT);

	//Controle de frames
	float frameDuration = 0.2f;  // Tempo entre as trocas de frames
	float timeSinceLastFrame = 0.0f;  // Tempo acumulado desde a última troca
	int currentFrame = 0;  // Índice do frame atual


	glUseProgram(shaderID);

	// Enviando a cor desejada (vec4) para o fragment shader
	// Utilizamos a variáveis do tipo uniform em GLSL para armazenar esse tipo de info
	// que não está nos buffers
	glUniform1i(glGetUniformLocation(shaderID, "texBuffer"), 0);


	// Matriz de projeção ortográfica
	//mat4 projection = ortho(0.0f,800.0f,0.0f,600.0f,-1.0f,1.0f);
	mat4 projection = ortho(0.0f, 2304.0f, 0.0f, 1296.0f, -1.0f, 1.0f);

	glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

	//Ativando o primeiro buffer de textura da OpenGL
	glActiveTexture(GL_TEXTURE0);

	//Habilitar a transparência
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//Habilitar o teste de profundidade
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);

	float lastFrameTime = glfwGetTime();  // Inicia o tempo anterior
	
	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{
    	float currentTime = glfwGetTime();
    	float deltaTime = currentTime - lastFrameTime;  // Tempo desde o último frame
    	lastFrameTime = currentTime;

	// Atualiza animação do personagem ao se mover
		if (dir != NONE)
		{
			animationTime += deltaTime;

			if (animationTime >= frameDuration)
			{
				// Alterna texturas
				currentFrame = (currentFrame + 1) % 4;
				character.texID = textureFrames[currentFrame]; // Atualiza a textura do personagem

				// Reseta tempo de animação
				animationTime = 0.0f;
			}
		} else {
			// Se não mover, personagem volta para "centro"
			character.texID = textureFrames[0];
		}

    // Limpa o buffer de cor
    	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Desenha o primeiro background na posição atual (offset)
    	background.position = vec3(0.0, 0.0, 0.0);  // Posiciona no canto inferior esquerdo inicialmente
    	drawSprite(background, shaderID);

	// Atualiza queda de objetos
	for (auto& obj : fallingObjects)
	{
		obj.sprite.position.y -= obj.fallSpeed * deltaTime; // Objeto cai
		if (obj.sprite.position.y < 0.0f)
		{
			obj.sprite.position = vec3(objectPosition(WIDTH), HEIGHT, 0.0f);

			if(!obj.isCoin)
			{
				selectObstacle(obj.sprite); // Seleciona um novo obstáculo
			}
			else
			{
				// seta a visibilidade para true novamente
        		obj.isVisible = true;
			}
		}

		if (obj.isCoin)
		{
			animateCoin(obj.sprite, obj, deltaTime, timeSinceLastFrame);  // Anima a moeda
		}

		if (obj.isVisible)
		{
			drawSprite(obj.sprite, shaderID);
		}
		
		// Verifica colisão com moedas
		if (obj.isCoin && checkCollision(character, obj) && obj.isVisible)
		{
			obj.isVisible = false;
		}

		// Verifica colisão com lapides
		if (!obj.isCoin && checkCollision(character, obj))
		{
			std::cout << "GAME OVER!" << std::endl;
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	}

    // Checa se houveram eventos de input e chama as funções de callback
		glfwPollEvents();

		// Atualiza posição do personagem
		if (dir == LEFT) character.position.x -= vel;
		if (dir == RIGHT) character.position.x += vel;
		drawSprite(character,shaderID); // Desenha personagem

		// Troca os buffers da tela
		glfwSwapBuffers(window);
	}
	// Pede pra OpenGL desalocar os buffers
	//glDeleteVertexArrays(1, background.VAO);
	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_A || key == GLFW_KEY_LEFT)
	{
		dir = LEFT;
	}
	else if (key == GLFW_KEY_D || key == GLFW_KEY_RIGHT)
	{
		dir = RIGHT;
	}

	if (action == GLFW_RELEASE)
	{
		dir = NONE;
	}
}

//Esta função está basntante hardcoded - objetivo é compilar e "buildar" um programa de
// shader simples e único neste exemplo de código
// O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
// fragmentShader source no iniçio deste arquivo
// A função retorna o identificador do programa de shader
int setupShader()
{
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Checando erros de compilação (exibição via log no terminal)
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// Checando erros de compilação (exibição via log no terminal)
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Linkando os shaders e criando o identificador do programa de shader
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Checando por erros de linkagem
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

// Esta função está bastante harcoded - objetivo é criar os buffers que armazenam a 
// geometria de um triângulo
// Apenas atributo coordenada nos vértices
// 1 VBO com as coordenadas, VAO com apenas 1 ponteiro para atributo
// A função retorna o identificador do VAO
int setupGeometry()
{
	// Aqui setamos as coordenadas x, y e z do triângulo e as armazenamos de forma
	// sequencial, já visando mandar para o VBO (Vertex Buffer Objects)
	// Cada atributo do vértice (coordenada, cores, coordenadas de textura, normal, etc)
	// Pode ser arazenado em um VBO único ou em VBOs separados
	GLfloat vertices[] = {
        // x    y    z     s     t
         0.0,  0.0, 0.0,  0.0,  0.0, // canto inferior esquerdo
         1.0,  0.0, 0.0,  1.0,  0.0, // canto inferior direito
         0.0,  1.0, 0.0,  0.0,  1.0, // canto superior esquerdo
         1.0,  1.0, 0.0,  1.0,  1.0  // canto superior direito
    };


	GLuint VBO, VAO;
	//Geração do identificador do VBO
	glGenBuffers(1, &VBO);
	//Faz a conexão (vincula) do buffer como um buffer de array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//Envia os dados do array de floats para o buffer da OpenGl
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);
	// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
	// e os ponteiros para os atributos 
	glBindVertexArray(VAO);
	//Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando: 
	// Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
	// Numero de valores que o atributo tem (por ex, 3 coordenadas xyz) 
	// Tipo do dado
	// Se está normalizado (entre zero e um)
	// Tamanho em bytes 
	// Deslocamento a partir do byte zero 

	//Atributo 0 - Posição - x, y, z
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	//Atributo 1 - Coordenadas de textura - s, t
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice 
	// atualmente vinculado - para que depois possamos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0); 

	// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
	glBindVertexArray(0); 

	return VAO;
}

int loadTexture(string filePath, int &imgWidth, int &imgHeight)
{
	GLuint texID;

	// Gera o identificador da textura na memória 
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	//Configurando o wrapping da textura
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	//Configurando o filtering de minificação e magnificação da textura
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//Carregamento da imagem da textura
	int nrChannels;
	unsigned char *data = stbi_load(filePath.c_str(), &imgWidth, &imgHeight, &nrChannels, 0);

	if (data)
	{
    	if (nrChannels == 3) //jpg, bmp
    	{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imgWidth, imgHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    	}
    	else //png
    	{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    	}
    	glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
     	std::cout << "Failed to load texture" << filePath << std::endl;
	}
	
	return texID;
}

void drawSprite(Sprite spr, GLuint shaderID)
{
	glBindVertexArray(spr.VAO); //Conectando ao buffer de geometria

	glBindTexture(GL_TEXTURE_2D, spr.texID); //conectando o buffer de textura

	//Matriz de modelo - Tranformações na geometria, nos objetos
	mat4 model = mat4(1); //matriz identidade
	//Translação
	model = translate(model,spr.position);
	//Rotação
	model = rotate (model,radians(spr.angle), vec3(0.0,0.0,1.0));
	//Escala
	model = scale(model,spr.dimensions);
	//Enviar para o shader
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

	// Chamada de desenho - drawcall
	// Poligono Preenchido - GL_TRIANGLES
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0); //Desconectando o buffer de geometria

}