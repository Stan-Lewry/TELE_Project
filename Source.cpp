#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>

#define screenW 1280
#define screenH 720

SDL_Renderer* rend;
SDL_Window* window;
SDL_Event evt;
TTF_Font* mainFont;
TTF_Font* headerFont;

bool gRunning;
bool executing;
bool animating;

std::string inputText = "Some Text";

const int maxLines = 25;
std::string inputBlock[maxLines];
int currentLine = 0;

struct block{
	int r, g, b;
	int sortValue;
	int screenX, screenY;
	int target;
};

enum Operator{SWAP};

struct Instruction{
	Operator op;
	int index1;
	int index2;
};

enum ErrorCode{EC_SUCCESS, EC_BAD_OPCODE, EC_OUT_OF_BOUNDS, EC_TOO_LONG, EC_TOO_SHORT};

struct ParseResult{
	ErrorCode errorCode;
	int lineNo;
};

std::vector<Instruction> instructionStack;

block userSequence[5];


block targetSequence[5];

enum ButtonType{RUN, CLEAR};

class GUIObject{
public:
	GUIObject(int x, int y, int w, int h, SDL_Color color){
		this->x = x;
		this->y = y;
		this->w = w;
		this->h = h;
		this->color = color;
	}
	~GUIObject(){}

	int getX(){return x;}
	int getY(){return y;}
	int getW(){return w;}
	int getH(){return h;}
	SDL_Color getColor(){return color;}

protected:
	int x;
	int y;
	int w;
	int h;
	SDL_Color color;

};

class Container : public GUIObject{
public:
	Container(int x, int y, int w, int h, SDL_Color color) :GUIObject(x, y, w, h, color){
		contentsSize = 0;
		hidden = false;
	}
	~Container(){}

	int getContentsSize(){return contentsSize;}

	void addContents(Container* newObject){
		contentsSize += 1;
		contents.push_back(newObject);
	}
	
	Container* getContents(int index){
		return contents.at(index);
	}

	void setHidden(bool hidden){this->hidden = hidden;}
	bool isHidden(){return hidden;}

private:
	std::vector<Container*> contents;
	int contentsSize;
	bool hidden;
};

class Button : public GUIObject{
public:
	Button(int x, int y, int w, int h, SDL_Color color, bool shown, ButtonType buttonType)
		: GUIObject(x, y, w, h, color){
			this->buttonType = buttonType;
			this->shown = shown;
	}

	ButtonType getButtonType(){return buttonType;}
	bool isShown(){return shown;}
	void setShown(bool shown){this->shown = shown;}

	bool clickedWithin(int mouseX, int mouseY){
		if(mouseX >= x && mouseX < x + w){
			if(mouseY >= y && mouseY < y + h){
				std::cout << "Button Clicked!" << std::endl;
				return true;
			}
			return false;
		}
		return false;
	}

private:
	ButtonType buttonType;
	bool shown;
};


void swapBlocks(int block1, int block2);

std::vector<Button*> buttonList;

bool initSDL(){
	SDL_Init(SDL_INIT_EVERYTHING);

	TTF_Init();
	IMG_Init(IMG_INIT_PNG);
	//SDL_ShowCursor(SDL_DISABLE);
	window = SDL_CreateWindow("Technology Enhanced Learning Environment Prototype", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenW, screenH, SDL_WINDOW_SHOWN);
	if (window != NULL){
		printf("Window Initialized\n");
		rend = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED);
		if (rend != NULL){
			printf("Renderer Initialized\n");
			mainFont = TTF_OpenFont("Fonts/half_bold_pixel.ttf",24);
			headerFont = TTF_OpenFont("Fonts/half_bold_pixel.ttf", 32);
			SDL_StartTextInput();
			return true;
		}
		else{
			return false;
		}
	}
	else{
		return false;
	}
}

void initSequences(){
	
	userSequence[0] = {255, 255, 0, 4, 0, 0, 0};
	userSequence[1] = {0, 0, 255, 3, 0, 0, 0};
	userSequence[2] = {0, 255, 0, 2, 0, 0, 0};
	userSequence[3] = {255, 0, 0, 1, 0, 0, 0};
	userSequence[4] = {0, 255, 255, 5, 0, 0, 0};

	SDL_Rect tRect = {48, (screenH / 4) - 40, 80, 80};

	for(int i = 0; i < 5; i++){
		userSequence[i].screenX = tRect.x;
		userSequence[i].screenY = tRect.y;
		tRect.x += 112;
	}

	targetSequence[0] = {255, 0, 0, 1, 0, 0, 0};
	targetSequence[1] = {0, 255, 0, 2, 0, 0, 0};
	targetSequence[2] = {0, 0, 255, 3, 0, 0, 0};
	targetSequence[3] = {255, 255, 0, 4, 0, 0, 0};
	targetSequence[4] = {0, 255, 255, 5, 0, 0, 0};

	//tRect = {48, (screenH / 4) - 40, 80, 80};

	tRect.y = ((screenH / 4) - 40) + (screenH / 2);
	tRect.x = 48;

	for(int i = 0; i < 5; i++){
		targetSequence[i].screenX = tRect.x;
		targetSequence[i].screenY = tRect.y;
		tRect.x += 112;
	}
}

void initButtons(){
	SDL_Color c = {255, 255, 255, 1};
	Button* runButton = new Button(screenW - 280, screenH - 80, 120, 60, c, true, RUN);
	buttonList.push_back(runButton);

	Button* clearButton = new Button(screenW - 400 - 80, screenH - 80, 120, 60, c, true, CLEAR);
	buttonList.push_back(clearButton);
}

void clearAllText(){
	for(int i = 0 ; i < maxLines; i++){
		inputBlock[i] = "";
		currentLine = 0;
	}
}

/*
void parseInstructions(){
	// read the text block from back to front forming "instructions" and inserting them into the instruction list
	for(int i = maxLines - 1; i > -1; i--){
		int lineLength = inputBlock[i].size();
		const char* line = inputBlock[i].c_str();
		//std::cout << line << std::endl;

		char a;
		char* word;
		int iter = 0;
		a = line[iter];
		while(a != ' ' && iter < lineLength){
			//a = line[iter];
			word += a;
			iter ++;
			a = line[iter];
		}
		std::cout << word << std::endl;
	}
}
*/

/*
void parseInstructions(){
	for(int i = maxLines - 1; i > -1; i--){
		int lineLength = inputBlock[i].size();

		if(lineLength > 0){
			const char* line = inputBlock[i].c_str();

			char* a = line[0];
			char* word = "";
			int iter = 0;
			//std::cout << a << std::endl;
			
			while(a != ' ' && iter < lineLength){
				word += *a;
				iter += 1;
				a = line[iter];
			}
			std::cout << word << std::endl;
		}
	}
}
*/

ParseResult parseInstructions(){

	ParseResult pr;

	for(int i = maxLines - 1; i > -1; i--){
		std::string str = inputBlock[i];
		std::string buf;
		std::stringstream ss(str);

		std::vector<std::string> tokens;

		while(ss >> buf){
			tokens.push_back(buf);
		}

		if(tokens.size() > 0){



			Instruction newInstruction;
		/*
		for(int i = 0; i < tokens.size(); i++){
			std::cout << tokens.at(i) << std::endl;
		}
		*/
			if(tokens.size() > 3){
				//std::cout << "ERROR AT LINE " << i << std::endl;	
				pr = {EC_TOO_LONG, i};
				instructionStack.clear();
				return pr;
			}
			else if(tokens.size() < 3){
				pr = {EC_TOO_SHORT, i};
				instructionStack.clear();
				return pr;
			}
			else if(tokens.at(0) != "SWAP"){
				pr = {EC_BAD_OPCODE, i};
				instructionStack.clear();
				return pr;
			}
			else if(std::stoi(tokens.at(1)) < 0 || std::stoi(tokens.at(1)) > 4){
				pr = {EC_OUT_OF_BOUNDS, i};
				instructionStack.clear();
				return pr;
			}
			else if(std::stoi(tokens.at(2)) < 0 || std::stoi(tokens.at(2)) > 4){
				pr = {EC_OUT_OF_BOUNDS, i};
				instructionStack.clear();
				return pr;
			}
			else{
				newInstruction.op = SWAP;
				newInstruction.index1 = std::stoi(tokens.at(1));
				newInstruction.index2 = std::stoi(tokens.at(2));
				instructionStack.push_back(newInstruction);
			}
			std::cout << "managed to parse instructions" << std::endl;
		}
	}
	pr = {EC_SUCCESS, 0};
	return pr;
}

void executeInstructions(){
	if(instructionStack.size() > 0){
		Instruction currentInstruction = instructionStack.at(instructionStack.size() - 1);
		instructionStack.pop_back();
		if(currentInstruction.op == SWAP){
			swapBlocks(currentInstruction.index1, currentInstruction.index2);
		} 
	}else{
		executing = false;
	}
}

void renderContainers(Container* rootContainer){
	SDL_Rect outerRect = {rootContainer->getX(), rootContainer->getY(), rootContainer->getW(), rootContainer->getH()};
	//SDL_SetRenderDrawColor(rend, rootContainer->getColor().r, rootContainer->getColor().g, rootContainer->getColor().b, rootContainer->getColor().a);
	SDL_SetRenderDrawColor(rend, 224, 224, 224, 1);
	SDL_RenderFillRect(rend, &outerRect);

	SDL_Rect innerRect = {outerRect.x + 1, outerRect.y + 1, outerRect.w - 2, outerRect.h - 2};
	SDL_SetRenderDrawColor(rend, 50, 50, 50, 1);
	SDL_RenderFillRect(rend, &innerRect);

	if(rootContainer->isHidden()){
		SDL_SetRenderDrawColor(rend, 0, 0, 0, 0.5);
		SDL_RenderFillRect(rend, &outerRect);
	}

	for(int i = 0; i < rootContainer->getContentsSize(); i++){
		renderContainers(rootContainer->getContents(i));
	}
}

void renderText(const char* text, TTF_Font* font, int x, int y, int r, int g, int b){
	//std::cout << inputText << std::endl;
	SDL_Rect dest = { x, y, 0, 0 };
	TTF_SizeText(font, text, &dest.w, &dest.h);	
	SDL_Color col = { r, g, b };
	SDL_Surface* tempSurf = TTF_RenderText_Solid(font, text, col);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(rend, tempSurf);
	SDL_FreeSurface(tempSurf);
	SDL_RenderCopy(rend, texture, NULL, &dest);
	SDL_DestroyTexture(texture);
}

void renderTextBlock(){

	SDL_Rect lineHighlight = {(screenW / 2) + 42, (currentLine * 24) + 24, screenW, 24};
	SDL_SetRenderDrawColor(rend, 145, 145, 145, 1);
	SDL_RenderFillRect(rend, &lineHighlight);

	for(int i = 0; i < maxLines; i ++){
		renderText(inputBlock[i].c_str(), mainFont, (screenW / 2) + 44, (i * 24) + 24, 255, 255, 255);
	}
}

void renderLineNumbers(){
	for(int i = 0; i < maxLines; i++){
		std::string lineNoStr = std::to_string(i);
		if(i < 10){
			renderText(lineNoStr.c_str(), mainFont, (screenW / 2) + 20, (i * 24) + 24, 255, 130, 0);
		}
		else renderText(lineNoStr.c_str(), mainFont, (screenW / 2) + 2, (i * 24) + 24, 255, 130, 0);
	}
}

void renderHeaders(){
	renderText("YOUR SEQUENCE:", headerFont, (screenW / 4) - 125, 20, 255, 255, 255);
	renderText("TARGET SEQUENCE:", headerFont, (screenW / 4) - 150, (screenH / 2) + 20, 255, 255, 255);
}

void renderBlocks(){
	/*
	SDL_Rect dRect = {48, (screenH / 4) - 40, 80, 80};

	for(int i = 0; i < 5; i++){
		block currentBlock = userSequence[i];
		SDL_SetRenderDrawColor(rend, currentBlock.r, currentBlock.g, currentBlock.b, 1);
		SDL_RenderFillRect(rend, &dRect);
		dRect.x += 112;
	}
	*/
	SDL_Rect dRect = {0, 0, 80, 80};

	for(int i = 0; i < 5; i++){
		
		dRect.x = userSequence[i].screenX;
		dRect.y = userSequence[i].screenY;
		SDL_SetRenderDrawColor(rend, userSequence[i].r, userSequence[i].g, userSequence[i].b, 1 );
		SDL_RenderFillRect(rend, &dRect);

		std::string indexString = std::to_string(i);
		
		renderText(indexString.c_str(), mainFont, ((i * 112) + 48) + 28, dRect.y + 94, 255, 255, 255);

		dRect.x = targetSequence[i].screenX;
		dRect.y = targetSequence[i].screenY;
		SDL_SetRenderDrawColor(rend, targetSequence[i].r, targetSequence[i].g, targetSequence[i].b, 1 );
		SDL_RenderFillRect(rend, &dRect);

		//std::string indexString = std::to_string(i);
		renderText(indexString.c_str(), mainFont, (dRect.x + (dRect.w / 2)) - 12, dRect.y + 94, 255, 255, 255);
	}
}

void renderButtons(){
	for(int i = 0; i < buttonList.size(); i++){
		if(buttonList.at(i)->isShown()){
			SDL_Rect dRect = {buttonList.at(i)->getX(), buttonList.at(i)->getY(), buttonList.at(i)->getW(), buttonList.at(i)->getH()};
			SDL_SetRenderDrawColor(rend, 255, 255, 255, 1);
			SDL_RenderFillRect(rend, &dRect);	
		}
	}
}

void renderAll(Container* rootContainer){
	SDL_RenderClear(rend);
	renderContainers(rootContainer);
	renderLineNumbers();
	renderTextBlock();
	renderHeaders();
	renderBlocks();
	renderButtons();
	SDL_RenderPresent(rend);
}

void swapBlocks(int block1, int block2){
	std::swap(userSequence[block1], userSequence[block2]);
	userSequence[block1].target = (block1 * 112) + 48;
	userSequence[block2].target = (block2 * 112) + 48;
	animating = true;
	std::cout << "swapping " << block1 << " & " << block2 << std::endl;
}

void animateBlocks(){
	for(int i = 0 ; i < 5; i++){
		if(userSequence[i].target != 0){
			if(userSequence[i].target < userSequence[i].screenX){
				// if target is less than current X move left
				userSequence[i].screenX -= 1;
			}
			else if(userSequence[i].target > userSequence[i].screenX){
				// if target is greater than current x move right
				userSequence[i].screenX += 1;
			}
			else if(userSequence[i].target == userSequence[i].screenX){
				userSequence[i].target = 0;
				animating = false;
			}
		}
	}
}

void runButtonBehaviour(ButtonType t){
	if (t == RUN){
		std::cout << "RUN button clicked!" << std::endl;
		//executing = true;
		ParseResult pr = parseInstructions();
		if(pr.errorCode == EC_SUCCESS){
			executing = true;
		}
	}
	else if(t == CLEAR){
		std::cout << "CLEAR button clicked!" << std::endl;
		clearAllText();
	}
}

void checkButtons(int mouseX, int mouseY){
	for(int i = 0; i < buttonList.size(); i++){
		if(buttonList.at(i)->clickedWithin(mouseX, mouseY)){
			runButtonBehaviour(buttonList.at(i)->getButtonType());
		}
	}
}

void handleInputs(){
	while(SDL_PollEvent(&evt) != 0 && !executing){
		if(evt.type == SDL_QUIT){
			gRunning = false;
		}
		else if(evt.type == SDL_KEYDOWN){
			if (evt.key.keysym.sym == SDLK_BACKSPACE && inputBlock[currentLine].length() > 0){
				inputBlock[currentLine].pop_back();
			}
			else if(evt.key.keysym.sym == SDLK_BACKSPACE && inputBlock[currentLine].length() < 1 && currentLine > 0){
				currentLine -= 1;
			}
			else if(evt.key.keysym.sym == SDLK_RETURN && currentLine < maxLines - 1){
				currentLine += 1;
			}

			else if(evt.key.keysym.sym == SDLK_UP && currentLine > 0){
				currentLine -= 1;
			}
			else if(evt.key.keysym.sym == SDLK_DOWN && currentLine < maxLines - 1){
				currentLine += 1;
			}
			//		delete last character
			// else if( e.key.keysym.sym == SDLK_c && SDL_GetModState() & KMOD_CTRL )
			//		copy - optional feature
			// else if ( e.key.keysym.sym == SDLK_v && SDL_GetModState() & KMOD_CTRL )
			//		paste also optiona
		}
		else if(evt.type == SDL_TEXTINPUT){
			inputBlock[currentLine] += evt.text.text;
		}

		else if(evt.type == SDL_MOUSEBUTTONDOWN){
			int x, y;
			SDL_GetMouseState(&x, &y);
			checkButtons(x, y);
		}
	}
}



int main(int argv, char* argc[]){


	
	if(initSDL()) gRunning = true;
	executing = false;
	animating = false;

	initSequences();

	initButtons();

	Container* rootContainer = new Container(0, 0, screenW, screenH, {255, 0, 255, 1});

	//Container* testContainer = new Container(0, 0, 200, 200, {255, 0, 0, 1});

	//rootContainer->addContents(testContainer);

	Container* rightContainer = new Container(screenW / 2, 0, screenW/2, screenH, {255, 255, 255, 1});

	Container* rightInsideContainer = new Container((screenW / 2) + 40, 0, screenW / 2, screenH, {255, 255, 255, 1});

	rightContainer->addContents(rightInsideContainer);

	Container* leftTopContainer = new Container(0, 0, screenW / 2, screenH / 2, {255, 255, 255, 1});

	Container* leftBottomContainer = new Container(0, screenH/2, screenW / 2, screenH/ 2, {255, 255, 255, 1});

	
	rootContainer->addContents(leftTopContainer);
	rootContainer->addContents(leftBottomContainer);
	rootContainer->addContents(rightContainer);


	while(gRunning){
		handleInputs();
		if(executing && !animating){
			executeInstructions();
		}
		animateBlocks();
		renderAll(rootContainer);
	}

	SDL_Quit();

	return 0;
}
