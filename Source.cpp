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

/*
struct container{
	int x, y, w, h;
	int r, g, b;
};
*/



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

private:
	std::vector<Container*> contents;
	int contentsSize;
};



bool initSDL(){
	SDL_Init(SDL_INIT_EVERYTHING);

	TTF_Init();
	IMG_Init(IMG_INIT_PNG);
	SDL_ShowCursor(SDL_DISABLE);
	window = SDL_CreateWindow("Technology Enhanced Learning Environment Prototype", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenW, screenH, SDL_WINDOW_SHOWN);
	if (window != NULL){
		printf("Window Initialized\n");
		rend = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED);
		if (rend != NULL){
			printf("Renderer Initialized\n");
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

void renderAll(){
	SDL_RenderClear(rend);

	SDL_Rect r{ 0, 0, screenW, screenH};
	SDL_SetRenderDrawColor(rend,255, 0, 0, 1);
	SDL_RenderFillRect(rend, &r);

	SDL_RenderPresent(rend);
}



void renderContainers(Container* rootContainer){
	SDL_Rect dRect = {rootContainer->getX(), rootContainer->getY(), rootContainer->getW(), rootContainer->getH()};
	SDL_SetRenderDrawColor(rend, rootContainer->getColor().r, rootContainer->getColor().g, rootContainer->getColor().b, rootContainer->getColor().a);
	SDL_RenderFillRect(rend, &dRect);

	for(int i = 0; i < rootContainer->getContentsSize(); i++){
		renderContainers(rootContainer->getContents(i));
	}
}

int main(int argv, char* argc[]){
	initSDL();

	Container* rootContainer = new Container(0, 0, screenW, screenH, {255, 0, 255, 1});

	Container* testContainer = new Container(0, 0, 200, 200, {255, 0, 0, 1});

	rootContainer->addContents(testContainer);

	//renderAll();

	SDL_RenderClear(rend);
	renderContainers(rootContainer);
	SDL_RenderPresent(rend);
	SDL_Delay(2000);

	SDL_Quit();

	return 0;
}
